
struct PS_IN
{
	float4 Proj : SV_POSITION;
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
	nointerpolation int MaterialIndex : MATERIALINDEX;
	nointerpolation int TriangleIndex : TRIANGLEINDEX;
};

struct TransformedVertex
{
	float3 P;
	float3 N;
	float2 C;
	int MaterialIndex;
	int TriangleIndex;
};

StructuredBuffer<TransformedVertex> Vertexes : register (t0);

void main(PS_IN input, out float3 hitCoord : SV_TARGET0, out int hitIndex : SV_TARGET1)
{
	float3 P = input.P;
	int triangleIndex = input.TriangleIndex;

	hitIndex = triangleIndex;

	float3 P0 = Vertexes[triangleIndex * 3].P;
	float3 P1 = Vertexes[triangleIndex * 3 + 1].P;
	float3 P2 = Vertexes[triangleIndex * 3 + 2].P;

	float3 N = cross(P1 - P0, P2 - P0);

	float a = abs(dot(cross(N, P2 - P1), P - P1));
	float b = abs(dot(cross(N, P2 - P0), P - P0));
	float c = abs(dot(cross(N, P1 - P0), P - P0));

	float l = a + b + c;

	hitCoord = l == 0 ? 0 : float3(a, b, c) / l;
}