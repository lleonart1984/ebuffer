Texture2D<float4> tex : register (t0);

SamplerState Samp
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};

cbuffer Transforming : register (b0) {
	row_major matrix Transform;
}

float4 main(float4 proj : SV_POSITION, float2 C : TEXCOORD) : SV_TARGET
{
	return mul (tex.Sample(Samp, C), Transform);
}