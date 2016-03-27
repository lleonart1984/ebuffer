struct GS_VERTEX_IN
{
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
};

struct GS_IN
{
	GS_VERTEX_IN V;
	nointerpolation int    M : MATERIALINDEX;
};

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

[maxvertexcount(1)]
void main(triangle GS_IN vertexes[3],
	inout PointStream<GS_OUT> triangleOut)
{
	GS_OUT Out = (GS_OUT)0;

	Out.proj = float4(0, 0, 0.5, 1);

	Out.V1 = (GS_V1_OUT)vertexes[0].V;
	Out.V2 = (GS_V2_OUT)vertexes[1].V;
	Out.V3 = (GS_V3_OUT)vertexes[2].V;

	Out.M = vertexes[0].M;

	triangleOut.Append(Out);
}
