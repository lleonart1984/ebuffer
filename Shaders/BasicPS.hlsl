struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD0;
};

cbuffer Lighting : register (b0)
{
	float3 LightPosition;
	float3 LightIntensity;
}

cbuffer Material : register (b1)
{
	float3 diffuse;
	float3 specular;
	float power;
	int TextureIndex;
}

Texture2D<float3> Diffuse : register (t0);

SamplerState samp : register (s0);

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 main(PS_INPUT input) : SV_Target
{
	float3 tex = TextureIndex >= 0 ? Diffuse.Sample(samp, float2(input.C.x, 1 - input.C.y)) : float3(1,1,1);

	float3 L = LightPosition - input.P;
	float D = length(L);
	L /= D;

	float3 V = normalize(-input.P);

	float3 H = normalize(V + L);

	float3 lighting = ((diffuse * saturate(dot(input.N, L))) * tex + specular * pow(saturate(dot(input.N, H)), power))*LightIntensity / (D * D);

	return float4(lighting,1);
	//return float4(input.C, 0, 1);
	//return float4(input.Pos.x/512,input.Pos.y/512, 0,1);// input.Pos;
}