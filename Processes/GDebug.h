#pragma once
#include "..\ca4G.h"
#include "..\ca4SD.h"
#include "..\ca4GDSL.h"

struct DEBUGTEXTURECB {
	float4x4 matrix;
};

class DebugTexture : public PixelShaderBinding {
public:
	Texture2D* Texture;
	Texture2D* RenderTarget;
	Buffer* Transforms;
protected:
	void Load() {
		LoadCode("Shaders\\DebugTexture.cso");
	}
	void OnGlobal() {
		CB(0, Transforms);
		SRV(0, Texture);
		RT(0, RenderTarget);
	}
};

struct DEBUGCOMPLEXITYCB {
	float scale;
	float translate;
	float2 rem;
};

class DebugComplexity : public PixelShaderBinding {
public:
	Texture2D* Texture;
	Buffer* Transforms;
	Texture2D* RenderTarget;
protected:
	void Load() {
		LoadCode("Shaders\\DebugComplexity.cso");
	}
	void OnGlobal() {
		CB(0, Transforms);
		SRV(0, Texture);
		RT(0, RenderTarget);
	}
};