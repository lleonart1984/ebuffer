#pragma once
#include "..\ca4G.h"
#include "..\ca4SD.h"
#include "..\ca4GDSL.h"
#include "SceneGeometryConstruction.h"
#include "LLABufferProcess.h"


class ShowAB_PS : public PixelShaderBinding {
public :
	Texture2D* RenderTarget;

	Texture2D* CountBuffer;
	Texture2D* StartBuffer;
	Buffer* Indices;
	Buffer* Fragments;

	Buffer* FaceInfoCB;
	Buffer* DebugCB;
protected:
	void Load()
	{
		LoadCode("Shaders\\ShowAB_PS.cso");
	}
	void OnGlobal() {
		RT(RenderTarget);
		CB(0, FaceInfoCB);
		CB(1, DebugCB);
		SRV(0, CountBuffer);
		SRV(1, StartBuffer);
		SRV(2, Indices);
		SRV(3, Fragments);
	}
};



class DebugABProcess : public DrawSceneProcess, public DebugableProcess {
private:
	ABufferConstructionProcess *constructingABuffer;
	ShowAB_PS *ps;
protected:
	void Initialize() {
		DrawSceneProcess::Initialize();

		load Process(constructingABuffer, ABufferDescription{ 128 });
		load Shader(ps);

		ps->CountBuffer = constructingABuffer->CountBuffer;
		ps->Fragments = constructingABuffer->Fragments;
		ps->Indices = constructingABuffer->Indices;
		ps->StartBuffer = constructingABuffer->StartBuffer;

		ps->FaceInfoCB = create ConstantBuffer<FaceInfo>();
		ps->DebugCB = create ConstantBuffer<DebugInfo>();
	}
	void Execute() {
		float4x4 view;
		float4x4 proj;
		scene->getCamera()->GetMatrices(RenderTarget->getWidth(), RenderTarget->getHeight(), view, proj);

		constructingABuffer->ViewMatrix = view;
		run(constructingABuffer);

		clear RTV(RenderTarget, scene->getBackColor());
		ps->RenderTarget = RenderTarget;
		FaceInfo faceInfo;
		faceInfo.ActiveFace = 5; // Negative Z
		faceInfo.CubeLength = constructingABuffer->Description.CubeLength;
		ps->FaceInfoCB->Update(faceInfo);
		ps->DebugCB->Update(Debugging);
		perform(ps, RenderTarget->getHeight(), RenderTarget->getHeight());
	}
public:
	DebugABProcess(DeviceManager *manager, ScreenDescription description) :DrawSceneProcess(manager, description) {}
	void SetScene(SScene* scene) {
		DrawSceneProcess::SetScene(scene);
		constructingABuffer->SetScene(scene);
	}
};