#pragma once
#include "..\ca4G.h"
#include "..\ca4SD.h"
#include "..\ca4GDSL.h"

typedef struct GlobalTransforms
{
	float4x4 Projection;
	float4x4 View;
	GlobalTransforms() {}
	GlobalTransforms(const float4x4 &view, const float4x4 &proj) : View(view), Projection(proj) { }
} GlobalTransforms;

typedef struct LocalTransforms
{
	float4x4 World;
	LocalTransforms() { }
	LocalTransforms(const float4x4 &world) : World(world) { }
} LocalTransforms;

class BasicVS : public VertexShaderBinding
{
protected:
	void Load()
	{
		LoadCode(".\\Shaders\\BasicVS.cso");

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
	void OnGlobal()
	{
		CB(0, Globals);
	}
	void OnLocal()
	{
		CB(1, Locals);
	}

public:
	Buffer* Globals;
	Buffer* Locals;
};

class BasicPS : public PixelShaderBinding
{
protected:
	void Load()
	{
		LoadCode("Shaders\\BasicPS.cso");
	}
	void OnGlobal()
	{
		RT(0, RenderTarget);
		DB(DepthBuffer);

		SMP(0, Sampling);

		CB(0, Lighting);
		CB(1, Material);
	}
	void OnLocal() {
		SRV(0, Texture);
	}
public:
	Texture2D* RenderTarget;
	Texture2D* DepthBuffer;

	Texture2D* Texture;
	Sampler* Sampling;
	Buffer* Lighting;
	Buffer* Material;
};

struct MaterialCB
{
	float3 Diffuse;
	int rem0;

	float3 Specular;
	float Power;

	int TextureIndex;
	float3 rem1;
};

struct LightingCB {
	float3 Position;
	float rem0;
	float3 Intensity;
	float rem1;
};

class BasicProcess : public DrawSceneProcess
{
private:
	BasicVS* vs;
	BasicPS* ps;
	Buffer* Globals;
	Buffer* Locals;
	Buffer* Lighting;
	Buffer* Material;
	Texture2D* texture;
	Sampler* sampling;
protected:

	void Initialize()
	{
		// Creates DepthBuffer
		DrawSceneProcess::Initialize();

		load Shader(ps);
		load Shader(vs);

		Globals = create ConstantBuffer<GlobalTransforms>();
		Locals = create ConstantBuffer<LocalTransforms>();

		Lighting = create ConstantBuffer<LightingCB>();
		Material = create ConstantBuffer<MaterialCB>();

		sampling = create Sampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_MIRROR, D3D11_TEXTURE_ADDRESS_MIRROR, D3D11_TEXTURE_ADDRESS_MIRROR);

		vs->Globals = Globals;
		vs->Locals = Locals;

		ps->Lighting = Lighting;
		ps->Material = Material;
		ps->Sampling = sampling;
		//texture = load Texture("seafloor.dds");
	}

	void Destroy() {
		delete vs;
		delete ps;
		delete Globals;
		delete Locals;
	}

	void Execute()
	{
		static LocalTransforms Locals;
		static MaterialCB Material;

		float4x4 view, projection;
		scene->getCamera()->GetMatrices(RenderTarget->getWidth(), RenderTarget->getHeight(), view, projection);

		GlobalTransforms globals(view, projection);
		vs->Globals->Update(globals);

		LightingCB Lighting;
		Lighting.Intensity = scene->getLight()->Intensity;
		Lighting.Position = xyz(mul(float4(scene->getLight()->Position, 1), view));
		ps->Lighting->Update(Lighting);

		// Binds render target and depth buffer before setting on pipeline
		ps->RenderTarget = RenderTarget;
		ps->DepthBuffer = DepthBuffer;
		set Pipeline(vs, ps);
		set Viewport(RenderTarget->getWidth(), RenderTarget->getHeight());
		set Fill(solid);
		set Cull(none);

		float4 backColor = scene->getBackColor();
		clear RTV(RenderTarget, (float*)&backColor);
		clear Depth(DepthBuffer, 1);

		for (int i = 0; i < scene->Length(); i++)
		{
			SVisual* v = scene->getVisual(i);

			Locals.World = v->getWorldTransform();
			vs->Locals->Update(Locals);

			MATERIAL* mat = v->getMaterial();

			if (mat != NULL)
			{
				Material.Diffuse = mat->Diffuse;
				Material.Specular = mat->Specular;
				Material.Power = mat->SpecularPower;
				Material.TextureIndex = mat->TextureIndex;
				ps->Texture = scene->getTexture(mat->TextureIndex);
			}
			else
			{
				Material.Diffuse = float3(0.7, 0.7, 0.9);
				Material.Specular = float3(1, 1, 1);
				Material.Power = 40;
				Material.TextureIndex = -1;
				ps->Texture = nullptr;
			}
			ps->Material->Update(Material);
			v->Draw();
		}
	}
public:
	BasicProcess(DeviceManager* manager, ScreenDescription description) :DrawSceneProcess(manager, description) { }
};

