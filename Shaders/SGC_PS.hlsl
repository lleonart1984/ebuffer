struct GS_V1_OUT {
	float3 P : POSITION0;
	float3 N : NORMAL0;
	float2 C : TEXCOORD0;
};

struct GS_V2_OUT {
	float3 P : POSITION1;
	float3 N : NORMAL1;
	float2 C : TEXCOORD1;
};

struct GS_V3_OUT {
	float3 P : POSITION2;
	float3 N : NORMAL2;
	float2 C : TEXCOORD2;
};

struct GS_OUT
{
	float4 proj : SV_POSITION;
	GS_V1_OUT V1;
	GS_V2_OUT V2;
	GS_V3_OUT V3;
	nointerpolation int M : MATERIALINDEX;
};

struct VertexData
{
	float3 P;
	float3 N;
	float2 C;
};

struct TransformedVertex
{
	VertexData V;
	int MaterialIndex;
	int TriangleIndex;
};

RWStructuredBuffer<TransformedVertex> vertexes : register (u0);
RWStructuredBuffer<int> Malloc : register(u1);

void main(GS_OUT In)
{
	int pos;
	InterlockedAdd(Malloc[0], 3, pos);

	int MI = In.M;
	TransformedVertex temp = (TransformedVertex)0;
	temp.MaterialIndex = MI;
	temp.TriangleIndex = pos/3;
	temp.V = In.V1;
	vertexes[pos++] = temp;
	temp.V = In.V2;
	vertexes[pos++] = temp;
	temp.V = In.V3;
	vertexes[pos] = temp;
}