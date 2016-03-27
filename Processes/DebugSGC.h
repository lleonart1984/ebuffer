#pragma once
#include "..\ca4G.h"
#include "..\ca4SD.h"
#include "..\ca4GDSL.h"
#include "SceneGeometryConstruction.h"

struct ShowSG_Global {
	float4x4 Projection;
};

class ShowSG_VS : public VertexShaderBinding {
protected:
	void Load() {
		LoadCode("Shaders\\ShowSG_VS.cso");

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "MATERIALINDEX", 0, DXGI_FORMAT_R8_SINT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TRIANGLEINDEX", 0, DXGI_FORMAT_R8_SINT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);

		LoadInputLayout(layout, numElements);
	}
	void OnGlobal() {
		CB(0, Globals);
	}
public:
	Buffer* Globals;
};

struct ShowSG_Lighting {
	float3 Position;
	float rem0;
	float3 Intensity;
	float rem1;
};

class ShowSG_PS : public PixelShaderBinding {
protected:
	void Load()
	{
		LoadCode("Shaders\\ShowSG_PS.cso");
	}
	void OnGlobal() {
		RT(RenderTarget);
		DB(DepthBuffer);
		CB(0, Lighting);
		SMP(0, Sampler);
		SRV(0, Materials);
		SRV(1, (Resource**)Textures, TextureCount);
	}
public:
	Texture2D* RenderTarget;
	Texture2D* DepthBuffer;
	Buffer* Lighting;
	Buffer* Materials;
	Texture2D** Textures;
	int TextureCount;
	Sampler* Sampler;
};

class DebugSGCProcess : public DrawSceneProcess {
private:
	ShowSG_VS *vs;
	ShowSG_PS *ps;
	SceneGeometryConstructionProcess *process;
	Sampler* sampler;
protected:
	void Initialize() {
		DrawSceneProcess::Initialize();

		process = load Process<SceneGeometryConstructionProcess, NoDescription>(NoDescription());
		vs = load Shader<ShowSG_VS>();
		ps = load Shader<ShowSG_PS>();

		vs->Globals = create ConstantBuffer<ShowSG_Global>();
		ps->Lighting = create ConstantBuffer<ShowSG_Lighting>();
		ps->Sampler = create BilinearSampler();
	}
	void Execute() {
		float4x4 view;
		float4x4 proj;
		scene->getCamera()->GetMatrices(RenderTarget->getWidth(), RenderTarget->getHeight(), view, proj);

		// execute Scene Geometry Construction sub-process
		process->In_ViewMatrix = view;
		run(process);

		ShowSG_Lighting l;
		l.Position = scene->getLight()->Position;
		l.Intensity = scene->getLight()->Intensity;
		ps->Lighting->Update(l);

		ShowSG_Global g;
		g.Projection = proj;
		vs->Globals->Update(g);

		ps->Materials = process->Out_Materials;
		ps->Textures = process->Out_Textures;
		ps->TextureCount = process->TextureCount;

		ps->RenderTarget = RenderTarget;
		ps->DepthBuffer = DepthBuffer;
		set Pipeline(vs, ps);
		set Viewport(RenderTarget->getWidth(), RenderTarget->getHeight());
		set Fill(solid);
		set Cull(none);
		set DepthTest();

		clear Depth(DepthBuffer);
		clear RTV(RenderTarget, scene->getBackColor());

		draw Primitive(process->Out_Vertexes, 0, process->VertexesCount);
	}
public:
	DebugSGCProcess(DeviceManager* manager, ScreenDescription description) :DrawSceneProcess(manager, description) {}
	void SetScene(SScene *scene) {
		this->DrawSceneProcess::SetScene(scene);
		process->SetScene(scene);
	}
};
