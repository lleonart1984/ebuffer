#pragma once
#include "..\ca4G.h"
#include "..\ca4SD.h"
#include "..\ca4GDSL.h"
#include "EBufferConstructionProcess.h"

class ShowEB_PS : public PixelShaderBinding {
public:
	Texture2D* RenderTarget;

	Buffer* EmptyBlocks;
	Texture2D* CountBuffer;
	Texture2D** StartBuffer;
	int NumberOfLevels;
	Buffer* FaceInfoCB;
	Buffer* DebugCB;
protected:
	void Load()
	{
		LoadCode("Shaders\\ShowEB_PS.cso");
	}
	void OnGlobal() {
		RT(RenderTarget);
		CB(0, FaceInfoCB);
		CB(1, DebugCB);
		SRV(0, EmptyBlocks);
		SRV(1, CountBuffer);
		SRV(2, (Resource**)StartBuffer, NumberOfLevels);
	}
};

class DebugEBProcess : public DrawSceneProcess, public DebugableProcess {
private:
	EBufferConstructionProcess *constructingEBuffer;
	ShowEB_PS *ps;
protected:
	void Initialize() {
		DrawSceneProcess::Initialize();

		load Process(constructingEBuffer, EBufferDescription{ 7 });
		load Shader(ps);

		ps->CountBuffer = constructingEBuffer->EBufferLengths;
		ps->EmptyBlocks = constructingEBuffer->EmptyBlocks;
		ps->NumberOfLevels = constructingEBuffer->NumberOfLevels;
		ps->StartBuffer = constructingEBuffer->StartBuffer;
		
		ps->FaceInfoCB = create ConstantBuffer<FaceInfo>();
		ps->DebugCB = create ConstantBuffer<DebugInfo>();
	}
	void Execute() {
		float4x4 view;
		float4x4 proj;
		scene->getCamera()->GetMatrices(RenderTarget->getWidth(), RenderTarget->getHeight(), view, proj);

		constructingEBuffer->ViewMatrix = view;
		run(constructingEBuffer);

		clear RTV(RenderTarget, scene->getBackColor());
		ps->RenderTarget = RenderTarget;
		FaceInfo faceInfo;
		faceInfo.CubeLength = constructingEBuffer->Description.getResolution();
		ps->FaceInfoCB->Update(faceInfo);
		ps->DebugCB->Update(Debugging);
		show(ps, RenderTarget->getHeight(), RenderTarget->getHeight());
	}
public:
	DebugEBProcess(DeviceManager *manager, ScreenDescription description) :DrawSceneProcess(manager, description) {}
	void SetScene(SScene* scene) {
		DrawSceneProcess::SetScene(scene);
		constructingEBuffer->SetScene(scene);
	}
};