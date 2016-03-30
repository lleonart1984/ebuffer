// Data for each fragment
struct Fragment {
	int Index;
	float MinDepth;
	float MaxDepth;
};
struct VertexData {
	float3 Position;
	float3 Normal;
	float2 Coordinates;
	int MaterialIndex;
	int TriangleIndex;
};
struct Range {
	float Minim;
	float Maxim;
};
struct OccupiedRange {
	int Start;
	int Count;
};

RWTexture2D<int> complexity : register (u2);

// Ray information input
Texture2D<float3> currentRayOrigin				: register (t0);
Texture2D<float3> currentRayDirection			: register (t1);

// Scene Geometry
StructuredBuffer<VertexData> triangles			: register (t2);
// E-Buffer
StructuredBuffer<Fragment> fragments			: register (t3);
StructuredBuffer<int> indices					: register (t4);
Texture2D<int> eLengthBuffer					: register (t5);
StructuredBuffer<OccupiedRange> occupiedRanges	: register (t6);
StructuredBuffer<Range> ranges					: register (t7);
Texture2D<int> startEB[12]						: register (t8);

// This constant buffer gets the information of the face
cbuffer EBufferInfo : register(b0)
{
	// Size in pixels of generated cube texture
	int CubeLength;
}

cbuffer RaymarchingInfo : register (b1) {

	int UseAdaptiveSteps;

	int CountSteps;

	int CountHits;

	int LODVariations;
}

int GetEmptySpaceBlockPosition(uint2 coord, float currentDepth)
{
	int count = eLengthBuffer[coord];
	int start = startEB[0][coord];

	int startIndex = start;
	int endIndex = start + count - 1;

	/*if (ranges[startIndex].Maxim > currentDepth)
	return 0;

	if (ranges[endIndex].Minim < currentDepth)
	return count - 1;
	*/
	for (int i = startIndex; i <= endIndex; i++)
		if (ranges[i].Maxim > currentDepth && ranges[i].Minim < currentDepth)
			return i - startIndex;

	return 0;
}

bool GetEmptySpaceBlock(int rangeIndex, uint2 coord, float currentDepth, out Range range)
{
	uint2 mipCoord = coord;

	int start = 0;

	[forcecase]
	switch (rangeIndex) {
	case 0: start = startEB[0][mipCoord]; break;
	case 1: start = startEB[1][mipCoord]; break;
	case 2: start = startEB[2][mipCoord]; break;
	case 3: start = startEB[3][mipCoord]; break;
	case 4: start = startEB[4][mipCoord]; break;
	case 5: start = startEB[5][mipCoord]; break;
	case 6: start = startEB[6][mipCoord]; break;
	case 7: start = startEB[7][mipCoord]; break;
	case 8: start = startEB[8][mipCoord]; break;
	case 9: start = startEB[9][mipCoord]; break;
	case 10: start = startEB[10][mipCoord]; break;
	case 11: start = startEB[11][mipCoord]; break;
	}

	uint2 refCoord = mipCoord << rangeIndex;

	int position = GetEmptySpaceBlockPosition(refCoord, currentDepth);

	range = ranges[start + position];

	return range.Minim < currentDepth && range.Maxim > currentDepth;
}


// Determines blocks in a frustum given screen coordinates and minimum and maximum depths.
// Return true if an occupied block is found, in that case, index1 and index2 are the interval of blocks involved.
// Return false if no occupied blocks are found
bool DetectBlocks(uint2 coord, float minDepth, float maxDepth, out uint index1, out uint index2)
{
	uint startIndex = startEB[0][coord];
	uint endIndex = startIndex + eLengthBuffer[coord] - 2;

	index1 = startIndex;
	index2 = endIndex;

	if (eLengthBuffer[coord] <= 1 || maxDepth < ranges[startIndex].Maxim || minDepth > ranges[endIndex + 1].Minim)
		return false;

	while (ranges[index1 + 1].Minim < minDepth)
		index1++;
	while (ranges[index2].Maxim > maxDepth)
		index2--;

	return index1 <= index2;
}

// Gets axises of cube face position lays on.
// Use matrix { dx, dy, dz } to transform from local space to world space, transpose to otherwise.
void GetFrustumAxis(int faceIndex, out float3 dx, out float3 dy, out float3 dz)
{
	int axis = faceIndex / 2;

	dx = float3 (axis == 0 ? 0 : 1, 0, axis == 0 ? 1 : 0);
	dy = float3 (0, axis == 1 ? 0 : 1, axis == 1 ? 1 : 0);
	dz = float3 (axis == 0 ? 1 : 0, axis == 1 ? 1 : 0, axis == 2 ? 1 : 0);
	dz *= faceIndex % 2 == 1 ? -1 : 1;
}

float2 Project(float3 P)
{
	return P.xy / P.z;
}

uint2 ToScreenPixel(float2 P, int faceIndex)
{
	uint px = uint(min(max(0, (P.x + 1)) * 0.5 * CubeLength, CubeLength - 1) + faceIndex * CubeLength);
	uint py = uint(min(max(0, (1 - P.y)) * 0.5 * CubeLength, CubeLength - 1));
	return uint2(px, py);
}

float2 ToPixelF(float2 P)
{
	return float2((P.x + 1) * 0.5 * CubeLength, (1 - P.y) * 0.5 * CubeLength);
}

void GetPixelFromView(float3 P, int faceIndex, float2 Ds, out uint2 px, out float2 Ps, out float2 PsN) {
	Ps = Project(P);
	PsN = float2 ((Ps.x + 1)*0.5 + faceIndex, (1 - Ps.y)*0.5) * CubeLength;
	px = ToScreenPixel(Ps, faceIndex);
}

bool IsInsideFrustum(float2 Ps, uint2 px)
{
	float2 screenPixelCenter = float2(px.x % CubeLength + 0.5, px.y + 0.5);
	float2 projectedSPC = float2((screenPixelCenter.x / CubeLength) * 2 - 1, 1 - (screenPixelCenter.y / CubeLength) * 2);
	float halfPixel = (1.0 + 0.1) / (CubeLength);

	return (Ps.x >= projectedSPC.x - halfPixel && Ps.x <= projectedSPC.x + halfPixel &&
		Ps.y >= projectedSPC.y - halfPixel && Ps.y <= projectedSPC.y + halfPixel);
}

bool HFS(float3 pos, float3 dir, float3 B, float3 C, inout float t, inout float3 P)
{
	float3 BA = B;
	float3 CA = C;
	float3 BAxCA = cross(BA, CA);

	float3 n = (BAxCA);

	float nDOTdir = dot(n, dir);

	float newt = (-dot(n, pos)) / nDOTdir;

	if (newt <= 0 || newt > t)
		return false;

	//newt = min (newt, t);

	P = pos + dir * newt;
	t = newt;

	return true;
}

bool Intersect(float3 O, float3 D, float3 V1, float3 V2, float3 V3, float minT,
	out float t, out float3 Pout, out float3 coords)
{
	float3 e1, e2;
	float3 P, Q, T;
	float det, inv_det, u, v;

	e1 = V2 - V1;
	e2 = V3 - V1;

	P = cross(D, e2);
	det = dot(e1, P);

	if (det == 0) return false;

	inv_det = 1 / det;

	T = O - V1;

	Q = cross(T, e1);

	float3 uvt = float3 (dot(T, P), dot(D, Q), dot(e2, Q)) * inv_det;

	if (any(uvt < 0) || uvt.x + uvt.y > 1 || uvt.z >= minT) return false;

	Pout = O + D*uvt.z;
	coords = float3 (1 - uvt.x - uvt.y, uvt.x, uvt.y);
	t = uvt.z;
	return true;
}

void UpLOD(inout float2 PsN, inout int rangeIndex, inout uint frustumSize, inout uint2 px)
{
	rangeIndex--;
	frustumSize >>= 1;
	PsN *= 2;
	px <<= 1;
	px |= uint2 (PsN.x >= px.x + 1 ? 1 : 0, PsN.y >= px.y + 1 ? 1 : 0);
}

void DownLOD(inout float2 PsN, inout int rangeIndex, inout uint frustumSize, inout uint2 px)
{
	rangeIndex++;
	frustumSize <<= 1;
	PsN /= 2;
	px >>= 1;
}

bool InsideFrustumView(uint2 px, int frustumSize, int faceIndex)
{
	return px.x * frustumSize >= faceIndex * CubeLength && px.x  * frustumSize< (faceIndex + 1)*CubeLength && px.y *
		frustumSize >= 0 && px.y  * frustumSize< CubeLength;
}

void AdvanceRayMarching(inout int counter, float3 P, float3 H, int faceIndex, float3 D, inout int rangeIndex, inout uint

	frustumSize, out bool hit, out int index, out float3 rayHitCoords)
{
	float znear = 0.0015;

	float3 rayOrigin = P;
	float3 rayDirection = D;

	float3 absP = abs(P);
	float depth = max(absP.x, max(absP.y, absP.z));
	if (depth <= znear) // if nearest of near plane hit directly versus observer box
	{
		// implement here hit versus box
		P = P + D * znear * 5;
	}

	hit = false;
	index = -1;
	rayHitCoords = float3(0, 0, 0);

	if (depth > 1000) // too far away, discard ray.
		return;

	int index1 = 0;
	int index2 = 0;
	float depth1 = 0;
	float depth2 = 0;
	uint2 pixel = uint2(0, 0);

	float3 axisx, axisy, axisz;

	GetFrustumAxis(faceIndex, axisx, axisy, axisz);

	float3x3 fromLocalToView = { axisx, axisy, axisz };
	float3x3 fromViewToLocal = transpose(fromLocalToView);

	// normalize P and D.
	P = mul(P, fromViewToLocal);
	H = mul(H, fromViewToLocal);
	D = mul(D, fromViewToLocal);

	//if (D.z < 0) // Clip H versus z-near 0.1
	//    H = lerp(P, H, (P.z - znear * 0.9f) / (P.z - H.z));

	float3 firstP = P;

	float distH = length(H - P);

	if (distH <= 0)
		return;

	float pixelSize = 2.0 / CubeLength;

	float2 Ds = Project(H) - Project(P);
	float lengthDs = length(Ds);

	if (lengthDs == 0)
		lengthDs = 1;

	Ds /= lengthDs;

	uint2 px;
	float2 Ps;
	float2 PsN;
	GetPixelFromView(P, faceIndex, Ds, px, Ps, PsN);
	px /= frustumSize;
	PsN /= frustumSize;

	float2 fact = float2(Ds.x < 0 ? -0.5 : 0.5, Ds.y < 0 ? -0.5 : 0.5);

	float2 XCorner1 = fact.x * float2(1, -1) * pixelSize;
	float2 XCorner2 = fact.x * float2(1, 1) * pixelSize;
	float2 YCorner1 = fact.y * float2(-1, 1) * pixelSize;
	float2 YCorner2 = fact.y * float2(1, 1) * pixelSize;

	float globalStep = length(H - P) * sign(dot(D, H - P)); // start step with all length to screen side

	for (int k = 0; k<1024; k++)
	{
		Range range = (Range)0;

		if (rangeIndex > 0 && !GetEmptySpaceBlock(rangeIndex, px, P.z, range))
		{
			//UpTo0LOD (PsN, rangeIndex, frustumSize, px);
			while (rangeIndex > 0)
				UpLOD(PsN, rangeIndex, frustumSize, px);
		}

		float2 pc = (px + float2(0.5, 0.5)) * frustumSize / CubeLength;
		float2 currentPixelCenter = float2((pc.x % 1) * 2 - 1, 1 - 2 * pc.y);
		float3 newP = H;
		float step = globalStep;

		bool hsideX = HFS(firstP, D, float3(currentPixelCenter + XCorner1 * frustumSize, 1), float3(currentPixelCenter + XCorner2 * frustumSize, 1), step, newP);
		bool hsideY = HFS(firstP, D, float3(currentPixelCenter + YCorner1 * frustumSize, 1), float3(currentPixelCenter + YCorner2 * frustumSize, 1), step, newP);

		bool hside = hsideX || hsideY;

		depth1 = min(P.z, newP.z);
		depth2 = max(P.z, newP.z);

		bool clipped = false;
		bool occupied = false;

		if (rangeIndex == 0)
		{
			occupied = DetectBlocks(px, depth1, depth2, index1, index2);
			range.Minim = 0;
			range.Maxim = 1000;

			if (occupied)
			{
				float t = 100000;
				float3 hitP = rayOrigin + rayDirection * t;

				int minIndex = min(index1, index2);
				int maxIndex = max(index1, index2);
				float minimDepth = depth1;
				float maximDepth = depth2;

				int startIndex = occupiedRanges[minIndex].Start;
				int endIndex = occupiedRanges[maxIndex].Start + occupiedRanges[maxIndex].Count - 1;

				for (int j = startIndex; j <= endIndex; j++)
				{
					counter += CountHits;

					Fragment frag = fragments[indices[j]];
					int hI = frag.Index;
					float minDepth = frag.MinDepth;
					float maxDepth = frag.MaxDepth;

					float tt;
					float3 PP;
					float3 coords;

					if (minDepth > maximDepth)
						break;

					if (
						minimDepth <= maxDepth && minDepth <= maximDepth &&
						Intersect(rayOrigin, rayDirection,
							triangles[3 * hI].Position,
							triangles[3 * hI + 1].Position,
							triangles[3 * hI + 2].Position,
							t,
							tt, PP, coords))
					{
						index = hI;
						t = tt;
						hitP = PP;
						rayHitCoords = coords;
					}
				}
			}
		}

		clipped = newP.z < range.Minim | newP.z > range.Maxim;

		float alpha = ((newP.z > P.z ? range.Maxim : range.Minim) - P.z) / (newP.z - P.z);

		newP = clipped ? lerp(P, newP, min(1, alpha)) : newP;

		hit = index != -1;

		int pxYInc = clipped ? 0 : !hsideY ? 0 : fact.y < 0 ? 1 : -1;
		int pxXInc = clipped ? 0 : hsideY ? 0 : fact.x > 0 ? 1 : -1;
		px += int2(pxXInc, pxYInc);

		P = newP;
		Ps = Project(P) + Ds*0.001;// Project(P+ D * 0.001f);
		PsN = uint2(float2((Ps.x + 1) * 0.5f + faceIndex, (1 - Ps.y) * 0.5f) * CubeLength) / (float)(frustumSize);

		int increment = clipped ? -1 : (UseAdaptiveSteps == 0 ? 0 : frustumSize < CubeLength / 2 ? 2 : 0);
		float frustumFactor = clipped ? 0.5 : UseAdaptiveSteps == 0 ? 1 : frustumSize < CubeLength / 2 ? 4 : 1;

		rangeIndex += increment;
		frustumSize *= frustumFactor;
		PsN /= frustumFactor;
		px /= frustumFactor;
		px += (clipped) ? uint2 (PsN.x >= px.x + 1 ? 1 : 0, PsN.y >= px.y + 1 ? 1 : 0) : uint2(0, 0);
		counter += CountSteps;

		[branch]
		if (hit || (globalStep - distance(firstP, newP)) <= 0.001 || P.z > 100 || P.z <= znear)
			return;
	}
}

int GetFace(float3 v)
{
	float3 absv = abs(v);
	float maxim = max(absv.x, max(absv.y, absv.z));
	int3 equals = v == maxim;
	int3 negEquals = v == -maxim;

	return dot(int3(0, 2, 4), equals) + dot(int3(1, 3, 5), negEquals);
}

void main(float4 proj : SV_POSITION, out int rayHit : SV_TARGET0, out float3 rayHitCoords : SV_TARGET1)
{
	rayHit = -1;
	rayHitCoords = float3(0, 0, 0);

	uint2 coord = proj.xy;

	// Read the data
	float3 dir = currentRayDirection[coord];

	if (!any(dir))
		return;

	dir = normalize(dir);

	float3 pos = currentRayOrigin[coord].xyz;

	float3 origin = pos;

	if (dot(pos, pos) > 10000) // too far position
		return;

	complexity[coord]++;

	bool cont;
	bool hit;

	float3 fHits[14];
	int fHitsCount = 0;

	float3 fCorners[24] = {
		float3(-1,1,1), float3(1,1,1),
		float3(1,1,1), float3(1,-1,1),
		float3(1,-1,1), float3(-1,-1,1),
		float3(-1,-1,1), float3(-1,1,1),

		float3(-1,1,-1), float3(1,1,-1),
		float3(1,1,-1), float3(1,-1,-1),
		float3(1,-1,-1), float3(-1,-1,-1),
		float3(-1,-1,-1), float3(-1,1,-1),

		float3(-1,1,1), float3(-1,1,-1),
		float3(-1,-1,1), float3(-1,-1,-1),
		float3(1,1,1), float3(1,1,-1),
		float3(1,-1,1), float3(1,-1,-1)
	};

	float t;
	float3 fHit;
	float3 coords;

	fHits[fHitsCount++] = pos;

	for (int i = 0; i<12; i++)
		if (Intersect(origin, dir, float3(0, 0, 0), fCorners[i * 2 + 0] * 100, fCorners[i * 2 + 1] * 100, 1000, t, fHit,
			coords))
			fHits[fHitsCount++] = fHit;

	fHits[fHitsCount++] = origin + dir * 100; // final position at infinity

	for (int i = 0; i<fHitsCount - 1; i++)
		for (int j = i + 1; j<fHitsCount; j++)
			if (length(fHits[j] - origin) < length(fHits[i] - origin))
			{
				float3 temp = fHits[j];
				fHits[j] = fHits[i];
				fHits[i] = temp;
			}

	int faceIndices[14];

	for (int i = 0; i < fHitsCount - 1; i++)
		faceIndices[i] = GetFace(lerp(fHits[i], fHits[i + 1], 0.5));

	int counter = 0;

	int rangeIndex = 0;
	uint frustumSize = 1;

	for (int i = 0; i<fHitsCount - 1; i++)
	{
		AdvanceRayMarching(counter, fHits[i], fHits[i + 1], faceIndices[i], dir, rangeIndex, frustumSize, hit,

			rayHit, rayHitCoords);

		if (CountSteps)
			counter++;

		if (rayHit != -1)
			break;
	}

	complexity[coord] += counter;
}