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

	int Level;
}

struct Range
{
	float MinDepth;
	float MaxDepth;
};

StructuredBuffer<Range> ranges : register (t0);
Texture2D<int> countBuffer : register (t1);
Texture2D<int> startEB[12] : register (t2);

// Gets an index from a projected position in cube face.
uint2 FromScreen(float2 proj)
{
	uint px = proj.x;
	uint py = proj.y;
	return uint2(px + CubeLength * FaceIndex, py);
}

float4 main(float4 proj : SV_POSITION, float2 C : TEXCOORD) : SV_TARGET
{
	uint2 coord = FromScreen(C * CubeLength);

	int startIndex;

	switch (Level)
	{
	case 0: startIndex = startEB[0][coord >> Level]; break;
	case 1: startIndex = startEB[1][coord >> Level]; break;
	case 2: startIndex = startEB[2][coord >> Level]; break;
	case 3: startIndex = startEB[3][coord >> Level]; break;
	case 4: startIndex = startEB[4][coord >> Level]; break;
	case 5: startIndex = startEB[5][coord >> Level]; break;
	case 6: startIndex = startEB[6][coord >> Level]; break;
	case 7: startIndex = startEB[7][coord >> Level]; break;
	case 8: startIndex = startEB[8][coord >> Level]; break;
	case 9: startIndex = startEB[9][coord >> Level]; break;
	case 10: startIndex = startEB[10][coord >> Level]; break;
	case 11: startIndex = startEB[11][coord >> Level]; break;
	}

	if (LayerIndex >= countBuffer[coord >> Level << Level])
		return float4 (0,0,1,1);

	Range range = ranges[startIndex + LayerIndex];

	if (range.MaxDepth < range.MinDepth)
		return float4(1,0,1,1);

	return float4(6 * (range.MaxDepth - range.MinDepth) * float3(0.05,0.05,0.05),1);
}

