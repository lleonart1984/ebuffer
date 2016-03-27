struct VS_IN
{
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
	int MaterialIndex : MATERIALINDEX;
	int TriangleIndex : TRIANGLEINDEX;
};

struct VS_OUT
{
	float4 Proj : SV_POSITION;
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
	int MaterialIndex : MATERIALINDEX;
	int TriangleIndex : TRIANGLEINDEX;
};

cbuffer globaltransforms : register(b0)
{
	row_major matrix    projection : packoffset (c0);
}

VS_OUT main(VS_IN input)
{
	VS_OUT output = (VS_OUT)0;

	output.Proj = mul(float4(input.P, 1), projection);
	output.P = input.P;
	output.N = input.N;
	output.C = float2(input.C.x, 1-input.C.y);
	output.MaterialIndex = input.MaterialIndex;
	output.TriangleIndex = input.TriangleIndex;

	return output;
}