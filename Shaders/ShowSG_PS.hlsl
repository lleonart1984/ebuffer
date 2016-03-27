struct PS_IN
{
	float4 Proj : SV_POSITION;
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
	int MaterialIndex : MATERIALINDEX;
	int TriangleIndex : TRIANGLEINDEX;
};

struct Material
{
	float3 Diffuse;
	float3 Specular;
	float SpecularPower;
	float3 Emissive;
	int Opaque;
	float ReflectionIndex;
	int TextureIndex;
};

cbuffer Lighting : register (b0)
{
	float3 LightPosition;
	float3 LightIntensity;
}

SamplerState samp						: register(s0);

StructuredBuffer<Material> Materials	: register(t0);
Texture2D<float3> Textures[32]			: register(t1);

float3 main(PS_IN input) : SV_TARGET
{
	Material m = Materials[input.MaterialIndex];

	float3 sampledColor = float3(1,1,1);
	// Texture sampling
	{
		switch (m.TextureIndex) {
		case 0: sampledColor = Textures[0].Sample(samp, input.C); break;
		case 1: sampledColor = Textures[1].Sample(samp, input.C); break;
		case 2: sampledColor = Textures[2].Sample(samp, input.C); break;
		case 3: sampledColor = Textures[3].Sample(samp, input.C); break;
		case 4: sampledColor = Textures[4].Sample(samp, input.C); break;
		case 5: sampledColor = Textures[5].Sample(samp, input.C); break;
		case 6: sampledColor = Textures[6].Sample(samp, input.C); break;
		case 7: sampledColor = Textures[7].Sample(samp, input.C); break;
		case 8: sampledColor = Textures[8].Sample(samp, input.C); break;
		case 9: sampledColor = Textures[9].Sample(samp, input.C); break;
		case 10: sampledColor = Textures[10].Sample(samp, input.C); break;
		case 11: sampledColor = Textures[11].Sample(samp, input.C); break;
		case 12: sampledColor = Textures[12].Sample(samp, input.C); break;
		case 13: sampledColor = Textures[13].Sample(samp, input.C); break;
		case 14: sampledColor = Textures[14].Sample(samp, input.C); break;
		case 15: sampledColor = Textures[15].Sample(samp, input.C); break;
		case 16: sampledColor = Textures[16].Sample(samp, input.C); break;
		case 17: sampledColor = Textures[17].Sample(samp, input.C); break;
		case 18: sampledColor = Textures[18].Sample(samp, input.C); break;
		case 19: sampledColor = Textures[19].Sample(samp, input.C); break;
		case 20: sampledColor = Textures[20].Sample(samp, input.C); break;
		case 21: sampledColor = Textures[21].Sample(samp, input.C); break;
		case 22: sampledColor = Textures[22].Sample(samp, input.C); break;
		case 23: sampledColor = Textures[23].Sample(samp, input.C); break;
		case 24: sampledColor = Textures[24].Sample(samp, input.C); break;
		case 25: sampledColor = Textures[25].Sample(samp, input.C); break;
		case 26: sampledColor = Textures[26].Sample(samp, input.C); break;
		case 27: sampledColor = Textures[27].Sample(samp, input.C); break;
		case 28: sampledColor = Textures[28].Sample(samp, input.C); break;
		}
	}

	float3 L = LightPosition - input.P;
	float D = length(L);
	L /= D;

	float3 V = normalize(-input.P);

	float3 H = normalize(V + L);

	return ((m.Diffuse * saturate(dot(input.N, L)) + 0.3) * sampledColor + m.Specular * pow(saturate(dot(input.N, H)), m.SpecularPower))*LightIntensity / (D * D);
}