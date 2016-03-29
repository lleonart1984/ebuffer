// This constant buffer gets the information of the face
cbuffer FaceInfo : register (b0)
{
	row_major matrix FaceTransform;
	// face being generated
	int ActiveFace;
	// Size in pixels of generated cube texture
	int CubeLength;
}

cbuffer DebugInfo : register(b1)
{
	int FaceIndex;

	int LayerIndex;
}

struct Fragment {
	int Index;
	float MinDepth;
	float MaxDepth;
};

Texture2D<int> countBuffer : register (t0);
Texture2D<int> startBuffer : register (t1);
StructuredBuffer<int> Indices : register(t2);
StructuredBuffer<Fragment> Fragments : register(t3);

uint2 FromScreen(float2 c)
{
	uint px = c.x * CubeLength;
	uint py = c.y * CubeLength;
	return uint2 (px + CubeLength * FaceIndex, py);
}

float3 GetColor(int complexity) {

	if (complexity == 0)
		return float3(1, 0, 1);

	//return float3(1,1,1);

	int level = min(4, log(complexity) / log(10));

	switch (level)
	{
	case 0:
		return lerp(float3(0, 0, 0.5), float3 (0, 0.5, 0.5), 0.1 * complexity);
	case 1:
		return lerp(float3(0, 0.5, 0.5), float3 (0.5, 0.5, 0), 0.01 * complexity);
	case 2:
		return lerp(float3(0.5, 0.5, 0), float3(0.5, 0, 0), 0.001 * complexity);
	case 3:
		return lerp(float3(0.5, 0, 0), float3(1, 0, 0), 0.0001 * complexity);
	default:
		return float3(0, 0, 1);
	}
}

float4 main(float4 P : SV_POSITION, float2 C : TEXCOORD) : SV_TARGET
{
	uint2 coord = FromScreen(C);

	int count = countBuffer[coord];
	if (count <= LayerIndex)
		return float4(1, 0, 0.5,1);
	return float4(Fragments[Indices[startBuffer[coord] + LayerIndex]].MinDepth * float3(1, 1, 1)*0.1, 1);

	int complexity = countBuffer[coord];

	return float4(GetColor(complexity), 1);
}