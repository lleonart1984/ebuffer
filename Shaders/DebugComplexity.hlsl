Texture2D<int> tex : register (t0);

cbuffer Trasforming : register(b0) {
	float scale;
	float translate;
};

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

float3 main(float4 proj : SV_POSITION, float2 C : TEXCOORD) : SV_TARGET
{
	int width, height;
	tex.GetDimensions(width, height);
	return GetColor(tex[uint2(width*C.x, height*C.y)] * scale + translate);
}