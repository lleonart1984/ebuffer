struct VertexData {
	float3 P;
	float3 N;
	float2 C;
	int MaterialIndex;
	int TriangleIndex;
};

struct Material
{
	float3 Diffuse;
	float3 Specular;
	float SpecularSharpness;
	int TextureIndex;
	float3 Emissive;
	float4 Roulette; // x-diffuse, y-mirror, z-fresnell, w-reflection index
};

struct PS_IN
{
	float4 Projection : SV_POSITION;
};

cbuffer Lighting : register(b0)
{
	float3 LightPosition;

	float3 LightIntensity;
}


// Incoming Ray informations
Texture2D<float3>				  positions : register (t0);
Texture2D<float3>				 directions : register (t1);
Texture2D<float3>				    factors : register (t2);
Texture2D<int>						   hits : register (t3);
Texture2D<float3>				  hitCoords : register (t4);

// List of triangles's vertices of Scene Geometry
StructuredBuffer<VertexData>      triangles : register (t5);
// Materials and Textures of the scene
StructuredBuffer<Material>		  materials : register (t6);
Texture2D<float4>			   Textures[32] : register (t7);

sampler Sampler : register (s0);

float4 Sampling(int index, float2 coord)
{
	switch (index)
	{
	case -1: return float4(1, 1, 1, 1);
	case  0: return Textures[0].Sample(Sampler, coord);
	case  1: return Textures[1].Sample(Sampler, coord);
	case  2: return Textures[2].Sample(Sampler, coord);
	case  3: return Textures[3].Sample(Sampler, coord);
	case  4: return Textures[4].Sample(Sampler, coord);
	case  5: return Textures[5].Sample(Sampler, coord);
	case  6: return Textures[6].Sample(Sampler, coord);
	case  7: return Textures[7].Sample(Sampler, coord);
	case  8: return Textures[8].Sample(Sampler, coord);
	case  9: return Textures[9].Sample(Sampler, coord);
	case 10: return Textures[10].Sample(Sampler, coord);
	case 11: return Textures[11].Sample(Sampler, coord);
	case 12: return Textures[12].Sample(Sampler, coord);
	case 13: return Textures[13].Sample(Sampler, coord);
	case 14: return Textures[14].Sample(Sampler, coord);
	case 15: return Textures[15].Sample(Sampler, coord);
	case 16: return Textures[16].Sample(Sampler, coord);
	case 17: return Textures[17].Sample(Sampler, coord);
	case 18: return Textures[18].Sample(Sampler, coord);
	case 19: return Textures[19].Sample(Sampler, coord);
	case 20: return Textures[20].Sample(Sampler, coord);
	case 21: return Textures[21].Sample(Sampler, coord);
	case 22: return Textures[22].Sample(Sampler, coord);
	case 23: return Textures[23].Sample(Sampler, coord);
	case 24: return Textures[24].Sample(Sampler, coord);
	case 25: return Textures[25].Sample(Sampler, coord);
	case 26: return Textures[26].Sample(Sampler, coord);
	case 27: return Textures[27].Sample(Sampler, coord);
	case 28: return Textures[28].Sample(Sampler, coord);
	case 29: return Textures[29].Sample(Sampler, coord);
	case 30: return Textures[30].Sample(Sampler, coord);
	case 31: return Textures[31].Sample(Sampler, coord);
	}

	return float4 (1, 1, 1, 1);
}

void ComputeFresnel(float3 dir, float3 faceNormal, float ratio, out float reflection, out float refraction)
{
	float f = ((1.0 - ratio) * (1.0 - ratio)) / ((1.0 + ratio) * (1.0 + ratio));

	float Ratio = f + (1.0 - f) * pow((1.0 + dot(dir, faceNormal)), 5);

	reflection = Ratio;
	refraction = 1 - Ratio;
}

static float pi = 3.1415926;

void main(PS_IN input,
	// Öutgoing ray determination
	out float3 accumulation : SV_TARGET0,
	out float3 newPosition : SV_TARGET1,
	out float3 newDirection : SV_TARGET2,
	out float3 newFactor : SV_TARGET3)
{
	uint2 coord = input.Projection.xy;

	// Read the data
	float3 dir = directions[coord];
	float3 pos = positions[coord];
	float3 fac = factors[coord];

	// copy values just in case ray will vanish...
	newFactor = float3(0, 0, 0);
	newPosition = pos;
	newDirection = float3(0, 0, 0);

	if (!any(fac)) // opaque 
	{
		accumulation = float3(0, 0, 0);
		return;
	}
	if (!any(dir))
	{
		accumulation = float3(0, 0, 0);
		return;
	}
	if (dot(pos, pos) > 10000) // too far position
	{
		accumulation = float3(0, 1, 1);
		return;
	}
	int hit = hits[coord];

	if (hit < 0) // vanishes
	{
		accumulation = float3(1, 1, 0);
		return;
	}
	VertexData V1 = triangles[hit * 3];
	VertexData V2 = triangles[hit * 3 + 1];
	VertexData V3 = triangles[hit * 3 + 2];

	float3 hitCoord = hitCoords[coord]; // use baricentric coordinates

	float3 position = mul(hitCoord, float3x3(V1.P, V2.P, V3.P));
	float3 normal = normalize(mul(hitCoord, float3x3(V1.N, V2.N, V3.N)));
	float2 coordinates = mul(hitCoord, float3x2(V1.C, V2.C, V3.C));

	Material material = materials[V1.MaterialIndex];

	float3 diffuse = material.Diffuse * Sampling(material.TextureIndex, float2(coordinates.x, 1 - coordinates.y)).xyz;
	float3 specular = material.Specular;
	bool isOpaque = material.Roulette.z == 0;

	float3 observer = -1 * dir;
	bool isOutside = dot(observer, normal) > 0;
	float3 facedNormal = isOutside ? normal : -1 * normal;

	float3 L = LightPosition - position;
	float D = length(L);
	L /= D;

	float reflection = 1, refraction = 0;
	float3 reflectingDirection = normalize(2 * facedNormal * dot(facedNormal, observer) - observer);
	float3 refractingDirection = float3(0, 0, 0);
	if (!isOpaque) // Compute fresnell's
	{
		float ratio = isOutside ? 1 / material.Roulette.w : material.Roulette.w;

		ComputeFresnel(dir, facedNormal, ratio, reflection, refraction);

		refractingDirection = refract(dir, facedNormal, ratio);

		if (!any(refractingDirection)) // internal reflection
		{
			reflection = refraction;
			refraction = 0;
		}
	}

	// normalize fresnell ratios with roulette
	reflection = reflection * material.Roulette.z + material.Roulette.y;
	refraction = refraction * material.Roulette.z;

	bool rayWillReflect = reflection >= refraction;

	float f = (rayWillReflect ? reflection : refraction);

	float3 brdf = (diffuse + specular*pow(saturate(dot(L, reflectingDirection)), material.SpecularSharpness)) * material.Roulette.x; // brdf for local lighting

	float3 localLighting = material.Emissive;

	localLighting += brdf * saturate(dot(facedNormal, L)) * LightIntensity / (D*D);

	accumulation = fac * localLighting;
	newFactor = fac * material.Specular * f;
	if (any(newFactor)) {
		newDirection = rayWillReflect ? reflectingDirection : refractingDirection;
		newPosition = position + facedNormal * (rayWillReflect ? 1 : -1)*0.005; // push away from surface to prevent self-intersection
	}
	else
	{ // Opaque ray
		newDirection = float3(0, 0, 0);
		newPosition = position;
	}
}