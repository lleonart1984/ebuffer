#pragma once
#include "..\ca4G.h"
#include "..\ca4SD.h"
#include "..\ca4GDSL.h"
#include "LLABufferProcess.h"


struct OccupiedRange {
	int Start;
	int Count;
};

struct Range {
	float MinDepth;
	float MaxDepth;
};

class EB_Level0_PS : public PixelShaderBinding {
public:
	// UAVs
	Buffer* OccupiedBlocks;
	Buffer* EmptyBlocks;
	Texture2D* EBufferLengths;
	// SRVs
	Buffer* Fragments;
	Buffer* Indices;
	Texture2D* StartBuffer;
	Texture2D* CountBuffer;
protected:
	void Load() {
		LoadCode("Shaders\\EB_Level0_PS.cso");
	}
	void OnGlobal() {
		UAV(0, OccupiedBlocks);
		UAV(1, EmptyBlocks);
		UAV(2, EBufferLengths);

		SRV(0, Fragments);
		SRV(1, Indices);
		SRV(2, StartBuffer);
		SRV(3, CountBuffer);
	}
};

class EB_LevelK_PS : public PixelShaderBinding {
public:
	// UAVs
	Buffer* EmptyBlocks;
	Texture2D* StartEB; // Start indices for new empty blocks arrays for this hierarchy level
	Buffer* Malloc;
	// SRVs
	Texture2D* PrevStartBuffer;
	Texture2D* EBufferLengths;
	// Constant Buffers
	Buffer* CombiningInfo;
protected:
	void Load() {
		LoadCode("Shaders\\EB_LevelK_PS.cso");
	}
	void OnGlobal() {
		CB(0, CombiningInfo);
		UAV(0, EmptyBlocks);
		UAV(1, StartEB);
		UAV(2, Malloc);
	}

	void OnLocal() {
		SRV(0, PrevStartBuffer);
		SRV(1, EBufferLengths);
	}
};

struct EBufferDescription {
	int Power;

	int getResolution() const { return 1 << Power; }
};

struct CombiningProcessCB {
	int Level;
	float3 rem;
};

class EBufferConstructionProcess : public SceneProcess<EBufferDescription> {
public:
	// Public Buffers
	Texture2D** StartBuffer;
	int NumberOfLevels;
	Texture2D* EBufferLengths;
	Buffer* EmptyBlocks;
	Buffer* OccupiedBlocks;
	float4x4 ViewMatrix;
	Texture2D* OutputStartBuffer;

	void SetScene(SScene *scene) {
		SceneProcess<EBufferDescription>::SetScene(scene);
		constructingABuffer->SetScene(scene);
	}
	EBufferConstructionProcess(DeviceManager* manager, EBufferDescription description) :SceneProcess<EBufferDescription>(manager, description) {
	}

	inline ABufferConstructionProcess* ABuffer() const {
		return constructingABuffer;
	}
protected:
	void Initialize() {
		load Process(constructingABuffer, ABufferDescription{ Description.getResolution() });
		load Shader(buildingLevel0);
		load Shader(buildingLevelK);
		
		buildingLevelK->CombiningInfo = create ConstantBuffer<CombiningProcessCB>();

		buildingLevel0->CountBuffer = constructingABuffer->CountBuffer;
		buildingLevel0->Fragments = constructingABuffer->Fragments;
		buildingLevel0->Indices = constructingABuffer->Indices;
		buildingLevel0->StartBuffer = constructingABuffer->StartBuffer;
		EBufferLengths = buildingLevel0->EBufferLengths = create Texture<int>(Description.getResolution() * 6, Description.getResolution());
		EmptyBlocks = buildingLevel0->EmptyBlocks = create StructuredBuffer<Range>(MAX_NUMBER_OF_FRAGMENTS);
		OccupiedBlocks = buildingLevel0->OccupiedBlocks = create StructuredBuffer<OccupiedRange>(MAX_NUMBER_OF_FRAGMENTS);
		
		NumberOfLevels = Description.Power + 1;
		StartBuffer = new Texture2D*[NumberOfLevels];

		OutputStartBuffer = create Texture<int>(Description.getResolution() * 6, Description.getResolution());

		StartBuffer[0] = constructingABuffer->StartBuffer;
		int Size = Description.getResolution() / 2;
		for (int i = 1; i <= Description.Power; i++)
		{
			StartBuffer[i] = create Texture<int>(Size * 6, Size);
			Size /= 2;
		}
		buildingLevelK->EBufferLengths = buildingLevel0->EBufferLengths;
		buildingLevelK->EmptyBlocks = buildingLevel0->EmptyBlocks;
		buildingLevelK->Malloc = constructingABuffer->Malloc;
	}
	void Execute() {
		constructingABuffer->ViewMatrix = ViewMatrix;
		run(constructingABuffer);

		// Create Level 0
		perform(buildingLevel0, Description.getResolution() * 6, Description.getResolution());

		int width = Description.getResolution() * 6;
		int height = Description.getResolution();

		OutputStartBuffer->UpdateSubresource(0, StartBuffer[0]);

		CombiningProcessCB Combining;
		// Create Level K From K-1
		for (int K = 1; K <= Description.Power; K++)
		{
			Combining.Level = K;
			buildingLevelK->CombiningInfo->Update(Combining);

			buildingLevelK->PrevStartBuffer = StartBuffer[K - 1];
			buildingLevelK->StartEB = StartBuffer[K];

			width /= 2;
			height /= 2;

			perform(buildingLevelK, width, height);

			OutputStartBuffer->UpdateSubresource(K, StartBuffer[K]);
		}
	}
private:
	// Subprocesses
	ABufferConstructionProcess *constructingABuffer;
	// Shaders
	EB_Level0_PS *buildingLevel0;
	EB_LevelK_PS *buildingLevelK;
};