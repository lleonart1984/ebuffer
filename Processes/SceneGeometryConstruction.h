#pragma once
#include "..\ca4G.h"
#include "..\ca4SD.h"
#include "..\ca4GDSL.h"

struct SGC_CB {
	float4x4 Transform;
	int Index;
	float3 rem;
};

class SGC_VS : public VertexShaderBinding {
protected:
	void Load()
	{
		LoadCode("Shaders\\SGC_VS.cso");

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);

		LoadInputLayout(layout, numElements);
	}
	void OnGlobal() {
		CB(0, Locals);
	}
public:
	Buffer* Locals;
};

class SGC_GS : public GeometryShaderBinding {
protected:
	void Load()
	{
		LoadCode("Shaders\\SGC_GS.cso");
	}
};

class SGC_PS : public PixelShaderBinding {
protected:
	void Load()
	{
		LoadCode("Shaders\\SGC_PS.cso");
	}
	void OnGlobal() {
		UAV(0, Vertexes);
		UAV(1, Malloc);
	}
public:
	Buffer* Vertexes;
	Buffer* Malloc;
};

struct SGC_VERTEX {
	float3 P;
	float3 N;
	float2 C;
	int MaterialIndex;
	int TriangleIndex;
};

class SceneGeometryConstructionProcess : public SceneProcess<NoDescription>
{
private:
	SGC_VS *vs;
	SGC_GS *gs;
	SGC_PS *ps;

protected:
	void Initialize() {
		vs = load Shader<SGC_VS>();
		gs = load Shader<SGC_GS>();
		ps = load Shader<SGC_PS>();
		vs->Locals = create ConstantBuffer<SGC_CB>();
		ps->Malloc = create StructuredBuffer<int>(1);
	}
	void Execute() {
		set Pipeline(vs, gs, ps);
		set Viewport(1, 1);
		set Fill(solid);
		set Cull(none);
		set NoDepthTest();
		clear UAV(ps->Malloc);

		static SGC_CB VS_CB;

		for (int i = 0; i < scene->Length(); i++)
		{
			Visual<VERTEX, MATERIAL>* visual = scene->getVisual(i);
			if (!DiscardLights || !visual->isEmissive()) {
				VS_CB.Transform = mul(visual->getWorldTransform(), In_ViewMatrix);
				VS_CB.Index = visual->getMaterialIndex();
				vs->Locals->Update(VS_CB);
				visual->Draw();
			}
		}
	}
	void Destroy() {
		delete vs->Locals;
		delete ps->Malloc;
		delete ps->Vertexes;
		delete vs;
		delete gs;
		delete ps;
	}
public:
	SceneGeometryConstructionProcess(DeviceManager* manager, NoDescription description) :SceneProcess<NoDescription>(manager, description) {}
	// Output
	Buffer* Out_Vertexes;
	Buffer* Out_Materials;
	Texture2D** Out_Textures;
	int TextureCount;
	int VertexesCount;
	// Input
	void SetScene(Scene<VERTEX, MATERIAL>* scene) {
		this->SceneProcess::SetScene(scene);

		VertexesCount = 0;
		for (int i = 0; i < scene->Length(); i++)
		{
			Visual<VERTEX, MATERIAL> *visual = scene->getVisual(i);
			VertexesCount += visual->getPrimitiveCount() * 3;
		}

		MATERIAL* materials = new MATERIAL[scene->MaterialCount()];
		for (int i = 0; i < scene->MaterialCount(); i++)
			materials[i] = *scene->getMaterial(i);
		Out_Materials = create StructuredBuffer<MATERIAL>(materials, scene->MaterialCount());

		Out_Textures = new Texture2D*[scene->TextureCount()];
		for (int i = 0; i < scene->TextureCount(); i++)
			Out_Textures[i] = scene->getTexture(i);
		TextureCount = scene->TextureCount();

		ps->Vertexes = Out_Vertexes = create StructuredBuffer<SGC_VERTEX>(VertexesCount);
	}

	float4x4 In_ViewMatrix;
	bool DiscardLights;
};
