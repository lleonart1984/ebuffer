cbuffer settingMaterialIndex : register(b0)
{
	row_major matrix    worldView : packoffset (c0);
	int MaterialIndex : packoffset (c4);
}

struct VS_IN
{
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
};

struct VS_OUT
{
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
	nointerpolation int	   M : MATERIALINDEX;
};

VS_OUT main(VS_IN input)
{
	VS_OUT output = (VS_OUT)0;
	output.P = mul(float4(input.P, 1), worldView).xyz;
	output.N = mul(float4(input.N, 0), worldView).xyz;
	output.C = input.C;
	output.M = MaterialIndex;
	return output;
}
