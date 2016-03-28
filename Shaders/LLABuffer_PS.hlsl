// Data for each fragment
struct Fragment {
	int Index;
	float MinDepth;
	float MaxDepth;
};

struct TransformedVertex {
	float3 P;
	float3 N;
	float2 C;
	int MaterialIndex;
	int TriangleIndex;
};

// This constant buffer gets the information of the face
cbuffer FaceInfo : register (b0)
{
	row_major matrix FaceTransform;
	// face being generated
	int ActiveFace;
	// Size in pixels of generated cube texture
	int CubeLength;
}

// Linked lists of fragments for each pixel
RWStructuredBuffer<Fragment> fragments : register(u0);
// References to next fragment node index for each node in linked list
RWStructuredBuffer<int> nextBuffer: register(u1);
// count of each list
RWTexture2D<int> countBuffer : register (u2);
// first node of each list
RWTexture2D<int> firstBuffer : register (u3);
// buffer for fragment allocation
RWStructuredBuffer<int> malloc : register(u4);

// Scene geometry
StructuredBuffer<TransformedVertex> triangles : register(t0);

// Gets an index from a projected position in cube face.
uint2 FromScreen(float2 proj)
{
	uint px = proj.x;
	uint py = proj.y;
	return uint2(px + CubeLength * ActiveFace, py);
}

struct PS_IN
{
	float4 proj : SV_POSITION;
	nointerpolation int TriangleIndex : TRIANGLEINDEX;
};


void Crop(inout float3 p1, inout float3 p2, float3 sidea, float3 sideb)
{
	if (!any(p1)) return; // no segment

	float3 n = (cross(sidea, sideb));

	float dotP1 = dot(p1, n);
	float dotP2 = dot(p2, n);

	if (dotP1 <= 0 && dotP2 <= 0)
	{
		p1 = p2 = 0;
		return;
	}

	if (dotP1 >= 0 && dotP2 >= 0)
		return;

	float3 inter = lerp(p1, p2, dotP1 / (dotP1 - dotP2));

	if (dotP2 < 0)
		p2 = inter;
	else
		p1 = inter;
}

void UpdateMinMax(float3 p1, float3 p2, float3 sidea, float3 sideb, float3 sidec, float3 sided, inout float minim, inout float maxim)
{
	Crop(p1, p2, sidea, sideb);
	Crop(p1, p2, sideb, sidec);
	Crop(p1, p2, sidec, sided);
	Crop(p1, p2, sided, sidea);

	if (!any(p1)) return;

	minim = min(minim, min(-p1.z, -p2.z));
	maxim = max(maxim, max(-p1.z, -p2.z));
}

// A, B, C are the triangle points, in COUNTER-CLOCKWISE order
void Intersect(float3 dir, float3 A, float3 B, float3 C,
	inout float minim, inout float maxim)
{
	float3 BA = B - A;
	float3 CA = C - A;
	float3 BAxCA = cross(BA, CA);

	float3 n = (BAxCA);
	float d = dot(n, A);

	float nDOTdir = dot(n, dir);

	if (nDOTdir == 0)
		return;

	float t = d / nDOTdir;

	if (t < 0)
		return;

	float3 P = dir * t;

	float3 PA = P - A;
	float3 PB = P - B;
	float3 PC = P - C;

	float3 BAxPA = cross(BA, PA);
	float3 CBxPB = cross(C - B, PB);
	float3 ACxPC = cross(A - C, PC);

	//float den = dot(BAxCA, n);

	float a = dot(CBxPB, n);
	float b = dot(ACxPC, n);
	float c = dot(BAxPA, n);

	if (a >= 0 && b >= 0 && c >= 0)
	{
		float depth = -P.z;

		minim = min(depth, minim);
		maxim = max(depth, maxim);
	}
}

void main(PS_IN input)
{
	// Get 2D coordinates of the a-buffer entry
	uint2 coord = FromScreen(input.proj.xy);

	int triangleIndex = input.TriangleIndex;

	// half pixel size in projection coordinates
	float halfPixelSize = 1.0 / CubeLength;

	// current pixel center in projection coordinates
	float3 p = float3(input.proj.xy / CubeLength * 2 - float2(1, 1), -1);
	p.y *= -1;

	// current pixel corners
	float3 p00 = p + float3(-halfPixelSize, halfPixelSize, 0);
	float3 p01 = p + float3(halfPixelSize, halfPixelSize, 0);
	float3 p10 = p + float3(-halfPixelSize, -halfPixelSize, 0);
	float3 p11 = p + float3(halfPixelSize, -halfPixelSize, 0);

	// current triangle vertexes transformed to current face coordinates
	float3 a = mul(float4(triangles[triangleIndex * 3 + 0].P, 1), (FaceTransform)).xyz;
	float3 b = mul(float4(triangles[triangleIndex * 3 + 1].P, 1), (FaceTransform)).xyz;
	float3 c = mul(float4(triangles[triangleIndex * 3 + 2].P, 1), (FaceTransform)).xyz;

	// minimum and maximum depths
	float minDepth = 1000000;
	float maxDepth = 0;

	//// check triangle sides vs. pixel sides
	UpdateMinMax(a, b, p00, p01, p11, p10, minDepth, maxDepth);
	UpdateMinMax(b, c, p00, p01, p11, p10, minDepth, maxDepth);
	UpdateMinMax(c, a, p00, p01, p11, p10, minDepth, maxDepth);

	// check pixel corners vs. triangle
	Intersect(p00, a, b, c, minDepth, maxDepth);
	Intersect(p01, a, b, c, minDepth, maxDepth);
	Intersect(p11, a, b, c, minDepth, maxDepth);
	Intersect(p10, a, b, c, minDepth, maxDepth);
	
	if (minDepth <= maxDepth)
	{
		InterlockedAdd(countBuffer[coord], 1);
	
		Fragment f = (Fragment)0;
		f.Index = input.TriangleIndex;
		f.MinDepth = minDepth;
		f.MaxDepth = maxDepth;

		int index;
		InterlockedAdd(malloc[0], 1, index);

		InterlockedExchange(firstBuffer[coord], index, nextBuffer[index]);

		fragments[index] = f;
	}
}