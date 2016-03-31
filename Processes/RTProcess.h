#pragma once
#include "..\ca4G.h"
#include "..\ca4SD.h"
#include "..\ca4GDSL.h"
#include "SceneGeometryConstruction.h"
#include "EBufferConstructionProcess.h"
#include "GDebug.h"

#define RT_EBUFFER_POWER 6

class RT_InitialRays_PS : public PixelShaderBinding {
public:
	// SRVs
	Buffer* SceneGeometry;
	// RTs
	Texture2D* RayDirections;
	Texture2D* Hits;
	Texture2D* HitCoords;
	// DB
	Texture2D* DepthBuffer;
protected:
	void Load() {
		LoadCode("Shaders\\RT_InitialRays_PS.cso");
	}
	void OnGlobal()
	{
		SRV(0, SceneGeometry);
		RT(0, RayDirections);
		RT(1, HitCoords);
		RT(2, Hits);
		DB(DepthBuffer);
	}
};

struct RTLightingCB {
	float3 Position;
	float rem0;
	float3 Intensity;
	float rem1;
};

class RT_Bounce_PS : public PixelShaderBinding {
public:
	// SRVs
	Texture2D *RayOrigins;
	Texture2D *RayDirections;
	Texture2D *RayFactors;
	Texture2D *Hits;
	Texture2D *HitCoords;

	Buffer *SceneGeometry;
	Buffer *Materials;
	Texture2D** Textures;
	int TextureCount;

	// Samplers
	Sampler *Sampling;

	// RTs
	Texture2D *Accumulation;
	Texture2D *NewRayOrigins;
	Texture2D *NewRayDirections;
	Texture2D *NewRayFactors;

	// CBs
	Buffer *Lighting;
protected:
	void Load() {
		LoadCode("Shaders\\RT_Bounce_PS.cso");
	}
	void OnGlobal() {
		SRV(0, RayOrigins);
		SRV(1, RayDirections);
		SRV(2, RayFactors);
		SRV(3, Hits);
		SRV(4, HitCoords);
		SRV(5, SceneGeometry);
		SRV(6, Materials);
		SRV(7, (Resource**)Textures, TextureCount);
		SMP(0, Sampling);
		RT(0, Accumulation);
		RT(1, NewRayOrigins);
		RT(2, NewRayDirections);
		RT(3, NewRayFactors);
		CB(0, Lighting);
	}
};

struct EBufferInfo {
	int Resolution;
	float3 rem;
};

struct RaytraversalDebug {
	int UseAdaptiveSteps;

	int CountSteps;

	int CountHits;

	int LODVariations;
};

class RT_Traversal_PS : public PixelShaderBinding {
public:
	// UAV
	Texture2D *Complexity;

	// RT
	Texture2D *Hits;
	Texture2D *HitCoords;

	// SRVs
	Texture2D *RayOrigins;
	Texture2D *RayDirections;
	//// Scene Geometry SRVs
	Buffer *SceneGeometry;
	//// EBuffer SRVs
	Buffer *Fragments;
	Buffer *Indices;
	Texture2D *EBufferLengths;
	Buffer *OccupiedBlocks;
	Buffer *EmptyBlocks;
	Texture2D** StartBuffers;
	int StartBuffersLength;

	// CBs
	Buffer* EBufferInfoCB;
	Buffer* RaytraversalDebugCB;
protected:
	void Load() {
		LoadCode("Shaders\\RT_Traversal_PS.cso");
	}
	void OnGlobal() {
		UAV(2, Complexity);

		RT(0, Hits);
		RT(1, HitCoords);

		SRV(0, RayOrigins);
		SRV(1, RayDirections);
		SRV(2, SceneGeometry);
		SRV(3, Fragments);
		SRV(4, Indices);
		SRV(5, EBufferLengths);
		SRV(6, OccupiedBlocks);
		SRV(7, EmptyBlocks);
		SRV(8, (Resource**)StartBuffers, StartBuffersLength);

		CB(0, EBufferInfoCB);
		CB(1, RaytraversalDebugCB);
	}

};

class RTProcess : public DrawSceneProcess {
private:
	EBufferConstructionProcess *constructingEBuffer;
	SG_Projection_VS *projectingScene;
	RT_InitialRays_PS *initializingRays;
	RT_Bounce_PS *bouncing;
	RT_Traversal_PS *traversing;

	Texture2D *rayOrigins;
	Texture2D *rayDirections;
	Texture2D *rayFactors;

	Texture2D *tmpRayOrigins;
	Texture2D *tmpRayDirections;
	Texture2D *tmpRayFactors;

	Texture2D *hits;
	Texture2D *hitCoords;

	// Debuging
	DebugTexture* debugingTexture;
	DebugComplexity* debugingComplexity;
protected:
	void DebugTexture(Texture2D* texture, float4x4 transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }) {
		debugingTexture->RenderTarget = RenderTarget;
		debugingTexture->Texture = texture;
		debugingTexture->Transforms->Update(transform);
		perform(debugingTexture, RenderTarget->getWidth(), RenderTarget->getHeight());
	}
	void DebugComplexity(Texture2D* texture, float scale = 1, float translate = 0) {
		debugingComplexity->RenderTarget = RenderTarget;
		debugingComplexity->Texture = texture;
		DEBUGCOMPLEXITYCB cb;
		cb.scale = scale;
		cb.translate = translate;
		debugingComplexity->Transforms->Update(cb);
		perform(debugingComplexity, RenderTarget->getWidth(), RenderTarget->getHeight());
	}
	void Initialize() {
		// Creates DepthBuffer
		DrawSceneProcess::Initialize();

		load Process(constructingEBuffer, EBufferDescription{ RT_EBUFFER_POWER });
		load Shader(projectingScene);
		load Shader(initializingRays);
		load Shader(bouncing);
		load Shader(traversing);

		load Shader(debugingTexture);
		load Shader(debugingComplexity);

		debugingTexture->Transforms = create ConstantBuffer<DEBUGTEXTURECB>();
		debugingComplexity->Transforms = create ConstantBuffer<DEBUGCOMPLEXITYCB>();

		// CBs
		projectingScene->Globals = create ConstantBuffer<SG_Projection_Global>();
		bouncing->Lighting = create ConstantBuffer<RTLightingCB>();
		traversing->EBufferInfoCB = create ConstantBuffer<EBufferInfo>();
		traversing->RaytraversalDebugCB = create ConstantBuffer<RaytraversalDebug>();

		// Samplers
		bouncing->Sampling = create BilinearSampler();

		// Buffers
		rayOrigins = create Texture<float4>(Description.Width, Description.Height);
		rayDirections = create Texture<float4>(Description.Width, Description.Height);
		rayFactors = create Texture<float4>(Description.Width, Description.Height);
		tmpRayOrigins = create Texture<float4>(Description.Width, Description.Height);
		tmpRayDirections = create Texture<float4>(Description.Width, Description.Height);
		tmpRayFactors = create Texture<float4>(Description.Width, Description.Height);
		hits = create Texture<int>(Description.Width, Description.Height);
		hitCoords = create Texture<float4>(Description.Width, Description.Height);

		traversing->Complexity = create Texture<int>(Description.Width, Description.Height);
		traversing->HitCoords = hitCoords;
		traversing->Hits = hits;

		// EBuffer data binding
		traversing->EBufferLengths = constructingEBuffer->EBufferLengths;
		traversing->EmptyBlocks = constructingEBuffer->EmptyBlocks;
		traversing->OccupiedBlocks = constructingEBuffer->OccupiedBlocks;
		traversing->StartBuffers = constructingEBuffer->StartBuffer;
		traversing->StartBuffersLength = constructingEBuffer->NumberOfLevels;
		// ABuffer data binding
		traversing->Fragments = constructingEBuffer->ABuffer()->Fragments;
		traversing->Indices = constructingEBuffer->ABuffer()->Indices;

		traversing->EBufferInfoCB->Update(EBufferInfo{ 1 << RT_EBUFFER_POWER });
	}
	void Destroy() {
		delete constructingEBuffer;
		delete projectingScene->Globals;
		delete projectingScene;
		delete initializingRays;
		delete bouncing->Lighting;
		delete bouncing;
		delete traversing->EBufferInfoCB;
		delete traversing->RaytraversalDebugCB;
		delete traversing->Complexity;
		delete traversing;
	}
	void Execute() {
		float4x4 view, projection;
		scene->getCamera()->GetMatrices(RenderTarget->getWidth(), RenderTarget->getHeight(), view, projection);

		constructingEBuffer->ViewMatrix = view;
		run(constructingEBuffer);

		auto sgcProcess = constructingEBuffer->ABuffer()->SceneGeometry();

		// Creating initial rays
		clear Depth(DepthBuffer);
		clear RTV(rayOrigins); // all rays from observer (0,0,0)
		clear RTV(rayFactors, 1); // all rays with 1,1,1 factor
		clear RTV(rayDirections);
		clear RTV(tmpRayDirections);
		clear RTV(tmpRayFactors);
		clear RTV(tmpRayOrigins);
		clear RTV(hitCoords);
		clear RTV(hits);
		initializingRays->DepthBuffer = DepthBuffer;
		initializingRays->RayDirections = rayDirections;
		initializingRays->Hits = hits;
		initializingRays->HitCoords = hitCoords;
		initializingRays->DepthBuffer = DepthBuffer;
		projectingScene->Globals->Update(SG_Projection_Global{ projection });
		set clean Pipeline(projectingScene, initializingRays)
			with Viewport(RenderTarget->getWidth(), RenderTarget->getHeight());

		draw Primitive(sgcProcess->Out_Vertexes, 0, sgcProcess->VertexesCount);

		clear RTV(RenderTarget); // Ray tracing accumulations
		clear UAV(traversing->Complexity);

		RaytraversalDebug debug;
		debug.CountHits = 0;
		debug.CountSteps = 1;
		debug.LODVariations = 1;
		debug.UseAdaptiveSteps = UseAdaptiveSteps ? 1 : 0;
		traversing->RaytraversalDebugCB->Update(debug);

		int N = 2;

		for (int b = 0; b < N; b++)
		{
			bouncing->RayOrigins = rayOrigins;
			bouncing->RayDirections = rayDirections;
			bouncing->RayFactors = rayFactors;
			bouncing->HitCoords = hitCoords;
			bouncing->Hits = hits;
			bouncing->Accumulation = RenderTarget;
			bouncing->NewRayOrigins = tmpRayOrigins;
			bouncing->NewRayDirections = tmpRayDirections;
			bouncing->NewRayFactors = tmpRayFactors;
			RTLightingCB l;
			l.Position = xyz(mul(float4(scene->getLight()->Position, 1), view));
			l.Intensity = scene->getLight()->Intensity;
			bouncing->Lighting->Update(l);
			set clean Pipeline(bouncing)
				with NoDepthTest()
				with Viewport(RenderTarget->getWidth(), RenderTarget->getHeight())
				with Blending(one, one);

			draw Screen();

			/// --- SWAP RAY BUFFERS ---
			rayOrigins = bouncing->NewRayOrigins;
			rayDirections = bouncing->NewRayDirections;
			rayFactors = bouncing->NewRayFactors;
			tmpRayOrigins = bouncing->RayOrigins;
			tmpRayDirections = bouncing->RayDirections;
			tmpRayFactors = bouncing->RayFactors;
			/// ------------------------
			if (b < N - 1) // Ray cast
			{
				traversing->RayOrigins = rayOrigins;
				traversing->RayDirections = rayDirections;

				perform(traversing, RenderTarget->getWidth(), RenderTarget->getHeight());

				//DebugTexture(traversing->HitCoords);
				//DebugComplexity(traversing->Hits, 0.001);
				//return;
			}
		}
		if (ShowComplexity)
		{
			DebugTexture(traversing->HitCoords);
			DebugComplexity(traversing->Complexity);
			return;
		}
	}
public:

	bool ShowComplexity;
	bool UseAdaptiveSteps;

	RTProcess(DeviceManager* manager, ScreenDescription description) :DrawSceneProcess(manager, description) {
	}
	void SetScene(SScene* scene)
	{
		DrawSceneProcess::SetScene(scene);
		constructingEBuffer->SetScene(scene);

		auto sgcProcess = constructingEBuffer->ABuffer()->SceneGeometry();

		initializingRays->SceneGeometry = sgcProcess->Out_Vertexes;
		bouncing->SceneGeometry = sgcProcess->Out_Vertexes;
		bouncing->Materials = sgcProcess->Out_Materials;
		bouncing->Textures = sgcProcess->Out_Textures;
		bouncing->TextureCount = sgcProcess->TextureCount;
		// Scene Geometry data binding
		traversing->SceneGeometry = constructingEBuffer->ABuffer()->SceneGeometry()->Out_Vertexes;
	}
};