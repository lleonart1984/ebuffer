#pragma once
#include "..\ca4G.h"
#include "..\ca4SD.h"
#include "..\ca4GDSL.h"

struct FaceInfo {
	float4x4 FaceTransform;
	int ActiveFace;
	int CubeLength;
	float2 rem;
};

class LLABuffer_PS : public PixelShaderBinding {
public:
	// Constant Buffers
	Buffer* FaceInfo;
	// UAVs
	Buffer* Fragments;
	Buffer* NextBuffer;
	Texture2D* CountBuffer;
	Texture2D* FirstBuffer;
	Buffer* Malloc;
	// SRVs
	Buffer* SceneGeometry;
protected:
	void Load() {
		LoadCode("Shaders\\LLABuffer_PS.cso");
	}
	void OnGlobal() {
		CB(0, FaceInfo);
		UAV(0, Fragments);
		UAV(1, NextBuffer);
		UAV(2, CountBuffer);
		UAV(3, FirstBuffer);
		UAV(4, Malloc);
		SRV(0, SceneGeometry);
	}
};

class ConservativeRasterization_GS : public GeometryShaderBinding {
public:
	// Constant Buffers
	Buffer* FaceInfo;

protected:
	void Load() {
		LoadCode("Shaders\\ConservativeRasterization_GS.cso");
	}
	void OnGlobal() {
		CB(0, FaceInfo);
	}
};

class AllocationAndSort_PS : public PixelShaderBinding{
public:
	//UAVs
	Texture2D* StartBuffer;
	Buffer* Malloc;
	Buffer* Indices;
	Buffer* Temp;
	//SRVs
	Texture2D* CountBuffer;
	Texture2D* FirstBuffer;
	Buffer* NextBuffer;
	Buffer* Fragments;
protected:
	void Load() {
		LoadCode("Shaders\\AllocationAndSort_PS.cso");
	}
	void OnGlobal() {
		SRV(0, CountBuffer);
		SRV(1, FirstBuffer);
		SRV(2, NextBuffer);
		SRV(3, Fragments);

		UAV(0, StartBuffer);
		UAV(1, Malloc);
		UAV(2, Indices);
		UAV(3, Temp);
	}
};

struct ABufferDescription {
	int CubeLength;
};

struct Fragment {
	int Index;
	float MinDepth;
	float MaxDepth;
};

struct CubeFaces
{
	CubeFaces()
	{
		float4x4 cubeProjection = PerspectiveFovRH(PI / 2, 1, 0.015, 1000);
		for (int f = 0; f < 6; f++)
			projectedFaces[f] = mul(faces[f], cubeProjection);
	}

		// Transformation to each face of the cube
	float4x4 faces [6] =
	{
		float4x4(
			0,0,-1,0,
			0,1,0,0,
			1,0,0,0,
			0,0,0,1), // positive x

		float4x4(
			0,0,1,0,
			0,1,0,0,
			1,0,0,0,
			0,0,0,1), // negative x

		float4x4(
			1,0,0,0,
			0,0,-1,0,
			0,1,0,0,
			0,0,0,1), // positive y

		float4x4(
			1, 0, 0, 0,
			0, 0, 1, 0,
			0, 1, 0, 0,
			0,0,0,1),

		float4x4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, -1, 0,
			0, 0, 0, 1),

		float4x4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1)
	};

	float4x4 projectedFaces [6];
};

#define MAX_NUMBER_OF_FRAGMENTS 10000000

class ABufferConstructionProcess : public SceneProcess<ABufferDescription> {
public:
	ABufferConstructionProcess(DeviceManager* manager, ABufferDescription description):SceneProcess<ABufferDescription>(manager, description) {
	}
	// Output Buffers
	Buffer* Fragments;
	Buffer* Indices;
	Texture2D* StartBuffer;
	Texture2D* CountBuffer;
	Buffer* Malloc; // required to allocate indices arrays

	float4x4 ViewMatrix;
	
	void SetScene(SScene *scene) {
		SceneProcess<ABufferDescription>::SetScene(scene);
		sgcProcess->SetScene(scene);
		creatingLL->SceneGeometry = sgcProcess->Out_Vertexes;
	}
	inline SceneGeometryConstructionProcess* SceneGeometry() {
		return sgcProcess;
	}
protected:
	void Initialize() {
		// Process to store Scene on the GPU
		load Process(sgcProcess);
		// Pipeline to create linked list
		load Shader(projectingSG);
		load Shader(conservativeRaster);
		load Shader(creatingLL);
		// Allocation and Sort of linked list using indices
		load Shader(allocatingAndSorting);

		// Constant Buffers
		projectingSG->Globals = create ConstantBuffer<SG_Projection_Global>();
		FaceInfoCB = creatingLL->FaceInfo = conservativeRaster->FaceInfo 
			= create ConstantBuffer<FaceInfo>();

		// Buffers
		Malloc = create StructuredBuffer<int>(1);
		Fragments = create StructuredBuffer<Fragment>(MAX_NUMBER_OF_FRAGMENTS);
		Indices = create StructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
		NextBuffer = create StructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
		Temp = create StructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
		FirstBuffer = create Texture<int>(Description.CubeLength * 6, Description.CubeLength);
		StartBuffer = create Texture<int>(Description.CubeLength * 6, Description.CubeLength);
		CountBuffer = create Texture<int>(Description.CubeLength * 6, Description.CubeLength);
		
		creatingLL->CountBuffer = CountBuffer;
		creatingLL->Fragments = Fragments;
		creatingLL->FirstBuffer = FirstBuffer;
		creatingLL->NextBuffer = NextBuffer;
		creatingLL->Malloc = Malloc;
		
		allocatingAndSorting->Fragments = Fragments;
		allocatingAndSorting->CountBuffer = CountBuffer;
		allocatingAndSorting->FirstBuffer = FirstBuffer;
		allocatingAndSorting->Indices = Indices;
		allocatingAndSorting->Malloc = Malloc;
		allocatingAndSorting->NextBuffer = NextBuffer;
		allocatingAndSorting->StartBuffer = StartBuffer;
		allocatingAndSorting->Temp = Temp;
	}
	void Execute() {
		// Store Scene on the GPU
		sgcProcess->In_ViewMatrix = ViewMatrix;
		run(sgcProcess);

		// Create linked list for each face
		clear UAV(CountBuffer, 0u);
		clear UAV(Malloc, 0u);

		set clean Pipeline(projectingSG, conservativeRaster, creatingLL)
			with Viewport(Description.CubeLength, Description.CubeLength);

		for (int faceIndex = 0; faceIndex < 6; faceIndex++) 
		{
			SG_Projection_Global Globals;
			Globals.Projection = TFaces.projectedFaces[faceIndex];
			projectingSG->Globals->Update(Globals);

			FaceInfo FaceInfoData;
			FaceInfoData.ActiveFace = faceIndex;
			FaceInfoData.CubeLength = Description.CubeLength;
			FaceInfoData.FaceTransform = TFaces.faces[faceIndex];
			FaceInfoCB->Update(FaceInfoData);
			
			draw Primitive(sgcProcess->Out_Vertexes, 0, sgcProcess->VertexesCount);
		}

		// Compute allocation and sort
		clear UAV(Malloc);
		
		perform(allocatingAndSorting, Description.CubeLength * 6, Description.CubeLength);
	}
private:
	// Subprocess
	SceneGeometryConstructionProcess *sgcProcess;
	// Shaders
	SG_Projection_VS *projectingSG;
	LLABuffer_PS *creatingLL;
	ConservativeRasterization_GS *conservativeRaster;
	AllocationAndSort_PS *allocatingAndSorting;
	// Temp Buffers
	Texture2D* FirstBuffer; // required during Linked List ABuffer
	Buffer* NextBuffer; // required during Linked List ABuffer
	Buffer* Temp; // required by bottom-up MergeSort algorithm
	// shared Constant Buffers
	Buffer* FaceInfoCB;
	// Tool objects
	CubeFaces TFaces;
};