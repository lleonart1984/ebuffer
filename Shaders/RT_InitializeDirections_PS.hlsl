cbuffer InverseProjection {
	row_major matrix InverseProj;
};

float3 main(float4 input : SV_POSITION, float2 C : TEXCOORD) : SV_TARGET
{
	float3 NPC = float3((C.x * 2 - 1), 1 - C.y * 2, -0);
	float3 dir = mul(float4(NPC, 1), InverseProj).xyz;
	return !any(dir) ? 0 : normalize(dir);
}