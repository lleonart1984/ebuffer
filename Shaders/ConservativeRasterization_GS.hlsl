// This constant buffer gets the information of the face
cbuffer FaceInfo : register (b0)
{
	row_major matrix FaceTransform;
	// face being generated
	int ActiveFace;
	// Size in pixels of generated cube texture
	int CubeLength;
}


struct ProjectedTransformedVertex
{
	float4 proj : SV_POSITION;
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
	int MaterialIndex : MATERIALINDEX;
	int TriangleIndex : TRIANGLEINDEX;
};

struct GS_OUT
{
	float4 proj : SV_POSITION;
	int TriangleIndex : TRIANGLEINDEX;
};

void updateCorners(float4 proj, inout float2 minim, inout float2 maxim)
{
	float2 screen = proj.xy / proj.w;
	minim = min(minim, screen);
	maxim = max(maxim, screen);
}

[maxvertexcount(4)]
void main(triangle ProjectedTransformedVertex vertexes[3],
	inout TriangleStream<GS_OUT> output)
{
	float4 projected[3];
	projected[0] = vertexes[0].proj;
	projected[1] = vertexes[1].proj;
	projected[2] = vertexes[2].proj;

	float2 minim = float2(1, 1);
	float2 maxim = float2(-1, -1);

	for (int i = 0; i<3; i++)
	{
		if (projected[i].z >= 0)
		{
			updateCorners(projected[i], minim, maxim);
			for (int j = 0; j<3; j++)
				if (projected[j].z < 0) // Perform Clipping if necessary
				{
					float alpha = -projected[i].z / (projected[j].z - projected[i].z);
					updateCorners(lerp(projected[i], projected[j], alpha), minim, maxim);
				}
		}
	}

	if ((maxim.y - minim.y) + (maxim.x - minim.x) <= 0.001)
		return;

	if (all(maxim - int2(-1, -1)) && all(minim - int2(1, 1))) // some posible point inside screen
	{
		minim -= float2(1, 1) / CubeLength;
		maxim += float2(1, 1) / CubeLength;

		GS_OUT clone = (GS_OUT)0;
		clone.TriangleIndex = vertexes[0].TriangleIndex;

		clone.proj = float4(minim.x, minim.y, 0.5, 1);
		output.Append(clone);

		clone.proj = float4(maxim.x, minim.y, 0.5, 1);
		output.Append(clone);

		clone.proj = float4(minim.x, maxim.y, 0.5, 1);
		output.Append(clone);

		clone.proj = float4(maxim.x, maxim.y, 0.5, 1);
		output.Append(clone);

		//output.RestartStrip();
	}
}
