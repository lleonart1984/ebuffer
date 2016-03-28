#pragma once
#include "..\ca4G.h"
#include "..\ca4SD.h"
#include "..\ca4GDSL.h"
#include "SceneGeometryConstruction.h"



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
		SRV(0, Vertexes);
		SRV(1, Materials);
		SRV(2, (Resource**)Textures, TextureCount);
	}
public:
	Texture2D* RenderTarget;
	Texture2D* DepthBuffer;
	Buffer* Lighting;
	Buffer* Vertexes;
	Buffer* Materials;
	Texture2D** Textures;
	int TextureCount;
	Sampler* Sampler;
};

class DebugSGCProcess : public DrawSceneProcess {
private:
	SG_Projection_VS *vs;
	ShowSG_PS *ps;
	SceneGeometryConstructionProcess *process;
	Sampler* sampler;
protected:
	void Initialize() {
		DrawSceneProcess::Initialize();

		load Process(process);
		load Shader(vs);
		load Shader(ps);

		vs->Globals = create ConstantBuffer<SG_Projection_Global>();
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

		SG_Projection_Global g;
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
		ps->Vertexes = process->Out_Vertexes;
	}
};
