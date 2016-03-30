#pragma once

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
//#include <d3d11_3.h>
#include <d3d11shader.h>
#include "gmath.h"
#include "ddsSupport.h"
#include <D2d1.h>
#include <dwrite.h>

class DeviceManager;
class Clearing;
class Builder;
class Loader;
class Setter;
class Drawer;
class Resource;
class Buffer;
class Texture2D;
class ShaderBinding;
class VertexShaderBinding;
class PixelShaderBinding;
class GeometryShaderBinding;
class ComputeShaderBinding;

class DeviceManager
{
	friend class Presenter;
	friend class Builder;
	friend class Setter;
	friend class Loader;
	friend class Clearing;
	friend class Drawer;

private:
	ID3D11Device *device;
	ID3D11DeviceContext *context;
	VertexShaderBinding *vs;
	GeometryShaderBinding *gs;
	PixelShaderBinding *ps;
	ComputeShaderBinding *cs;

protected:
	DeviceManager(ID3D11Device* device);

public:
	Setter* const setter;
	Builder* const builder;
	Loader* const loader;
	Clearing* const clearing;
	Drawer* const drawer;
	inline ID3D11Device* getDevice() { return device; }
	inline ID3D11DeviceContext* getContext() { return context; }

	virtual ~DeviceManager() {
		delete setter;
		delete builder;
		delete loader;
		delete clearing;
		delete drawer;
	}

	template <class P> P* Load()
	{
		P* process = new P(this);
		process->Load();
		return process;
	}

	template <class P> void Run(P* process)
	{
		process->Execute();
	}

	void Dispatch(int xCount, int yCount, int zCount);

	void Perform(PixelShaderBinding *ps, int width, int height);
};

class Resource
{
private:
	ID3D11ShaderResourceView* srv;
	ID3D11UnorderedAccessView* uav;
protected:
	ID3D11Resource* __internalResource;
	DeviceManager *manager;
public:
	Resource(DeviceManager* manager, ID3D11Resource* resource)
	{
		this->__internalResource = resource;
		this->manager = manager;

		this->srv = NULL;
	}
	inline ID3D11ShaderResourceView* GetSRV() {
		if (srv == NULL)
			manager->getDevice()->CreateShaderResourceView(__internalResource, NULL, &this->srv);
		return srv;
	}
	inline ID3D11UnorderedAccessView* GetUAV() {
		if (uav == NULL)
		{
			auto hr = manager->getDevice()->CreateUnorderedAccessView(__internalResource, NULL, &this->uav);
		}
		return uav;
	}
	void Release() {
		if (srv != NULL)
		{
			srv->Release();
			delete srv;
		}
		if (uav != NULL)
		{
			uav->Release();
			delete uav;
		}
		__internalResource->Release();
		delete __internalResource;
	}
	~Resource()
	{
		Release();
	}
};

class Buffer : public Resource
{
private:
	int stride;
	int byteLength;
	Buffer* stagged = nullptr;
	inline void Update(const void* data)
	{
		D3D11_MAPPED_SUBRESOURCE map;
		auto result = manager->getContext()->Map(this->getInternalBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		memcpy_s(map.pData, map.RowPitch, data, map.RowPitch);
		manager->getContext()->Unmap(this->getInternalBuffer(), 0);
		//manager->getContext()->UpdateSubresource(this->getInternalBuffer(), 0, NULL, data, 0, 0);
	}
public:
	Buffer(DeviceManager* manager, ID3D11Buffer *buffer, int stride) :Resource(manager, buffer)
	{
		D3D11_BUFFER_DESC d;
		buffer->GetDesc(&d);
		this->stride = stride;
		this->byteLength = d.ByteWidth;
	}
	inline int getStride() const { return stride; }
	inline int getByteLength() const { return byteLength; }
	inline ID3D11Buffer* getInternalBuffer() const { return (ID3D11Buffer*) this->__internalResource; }
	template<typename T> inline void Update(T& data)
	{
		Update((void*)&data);
	}

	template<typename T> void CopyTo(T* arr, int count) {
		Buffer* stagging = stagged != nullptr ? stagged : stagged = manager->builder->StructuredBuffer<T>(byteLength / sizeof(T), D3D11_USAGE_STAGING, (D3D11_BIND_FLAG)0);
		manager->getContext()->CopyResource(stagging->getInternalBuffer(), this->getInternalBuffer());
		D3D11_MAPPED_SUBRESOURCE map;
		manager->getContext()->Map(stagging->getInternalBuffer(), 0, D3D11_MAP_READ, (D3D11_MAP_FLAG)0, &map);
		auto hr = memcpy_s(arr, sizeof(T)*count, map.pData, sizeof(T)*count);
		manager->getContext()->Unmap(stagging->getInternalBuffer(), 0);
	}
};

class Texture2D : public Resource
{
	friend class Presenter;
private:
	ID3D11RenderTargetView* rtv;
	ID3D11DepthStencilView* dsv;
	float Width;
	float Height;
public:
	Texture2D(DeviceManager *manager, ID3D11Texture2D *texture) : Resource(manager, texture) {
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);
		Width = desc.Width;
		Height = desc.Height;
		rtv = NULL;
		dsv = NULL;
	}
	inline float getWidth() { return Width; }
	inline float getHeight() { return Height; }
	~Texture2D()
	{
		if (rtv != NULL)
		{
			rtv->Release();
			delete rtv;
		}
		if (dsv != NULL)
		{
			dsv->Release();
			delete dsv;
		}
	}
	inline ID3D11RenderTargetView* GetRTV() {
		if (rtv == NULL)
			manager->getDevice()->CreateRenderTargetView(__internalResource, NULL, &rtv);
		return rtv;
	}
	inline ID3D11DepthStencilView* GetDSV() {
		if (dsv == NULL)
			manager->getDevice()->CreateDepthStencilView(__internalResource, NULL, &this->dsv);
		return dsv;
	}
};

class Sampler {
private:
	ID3D11SamplerState* sst;
public:
	Sampler(ID3D11SamplerState *sst) :sst(sst) {
	}
	inline ID3D11SamplerState* getInnerSampler() { return sst; }
};

class ShaderBinding
{
	friend class DeviceManager;
	friend class Setter;
	friend class Loader;

	friend class VertexShaderBinding;
	friend class GeometryShaderBinding;
	friend class PixelShaderBinding;
	friend class ComputeShaderBinding;

private:
	void Bind(DeviceManager* manager)
	{
		this->Manager = manager;
	}

	int SRVSlots[32];
	int srvCount = 0;
	int CBSlots[32];
	int cbCount = 0;
	int UAVSlots[32];
	int uavCount = 0;
	void Unset() {
		for (int i = 0; i < srvCount; i++)
			SRV(SRVSlots[i], NULL);
		for (int i = 0; i < cbCount; i++)
			CB(CBSlots[i], NULL);
		for (int i = 0; i < uavCount; i++)
			UAV(UAVSlots[i], NULL);
	}
protected:
	DeviceManager *Manager;
	byte* code;
	int codeLength;

	virtual void CreateShader() = 0;

	void LoadCode(const char* fileName)
	{
		FILE* file;
		if (fopen_s(&file, fileName, "rb") != 0)
		{
			throw "Can not find the shader";
			return;
		}
		fseek(file, 0, SEEK_END);
		long long count;
		count = ftell(file);
		fseek(file, 0, SEEK_SET);

		code = new byte[count];
		int offset = 0;
		while (offset < count) {
			offset += fread_s(&code[offset], min(1024, count - offset), sizeof(byte), 1024, file);
		}
		fclose(file);

		codeLength = count;
		CreateShader();
	}
	virtual void OnGlobal() { }
	virtual void OnLocal() { }
	virtual void Load() = 0;
	virtual void Set() = 0;
	inline void CB(int slot, Buffer* cb) {
		CBSlots[cbCount++] = slot;
		OnCB(slot, cb);
	}
	inline void SRV(int slot, Resource* resource) {
		SRVSlots[srvCount++] = slot;
		OnSRV(slot, resource);
	}
	void SRV(int slot, Resource** resources, int count)
	{
		for (int i = 0; i < count; i++, slot++)
			SRV(slot, resources[i]);
	}

	inline void SMP(int slot, Sampler* smp) {
		OnSMP(slot, smp);
	}
	void UAV(int slot, Resource* resource) {
		UAVSlots[uavCount++] = slot;
		OnUAV(slot, resource);
	}


	virtual void OnCB(int slot, Buffer* cb) = 0;
	virtual void OnSRV(int slot, Resource* resource) = 0;
	virtual void OnSMP(int slot, Sampler* smp) = 0;
	virtual void OnUAV(int slot, Resource* resource) = 0;

	~ShaderBinding() {
		delete[] code;
	}
public:

	ShaderBinding() {
	}
	void UpdateLocal()
	{
		OnLocal();
	}
};

class VertexShaderBinding : public ShaderBinding
{
private:
	ID3D11VertexShader *__Shader;
	ID3D11InputLayout *__InputLayout;

	void OnUAV(int slot, Resource* resource) {

	}
protected:
	void VertexShaderBinding::LoadInputLayout(D3D11_INPUT_ELEMENT_DESC* layout, int numElements)
	{
		HRESULT hr = Manager->getDevice()->CreateInputLayout(layout, numElements, code, codeLength, &__InputLayout);
	}
	void CreateShader()
	{
		auto err = Manager->getDevice()->CreateVertexShader(code, codeLength, NULL, &__Shader);
	}
	void Set()
	{
		Manager->getContext()->VSSetShader(__Shader, NULL, 0);
		Manager->getContext()->IASetInputLayout(__InputLayout);

		OnGlobal();
	}
	void OnSRV(int slot, Resource* resource)
	{
		ID3D11ShaderResourceView* view = resource == NULL ? NULL : resource->GetSRV();
		Manager->getContext()->VSSetShaderResources(slot, 1, &view);
	}
	void OnCB(int slot, Buffer* cb)
	{
		ID3D11Buffer* b = cb == nullptr ? nullptr : cb->getInternalBuffer();
		this->Manager->getContext()->VSSetConstantBuffers(slot, 1, &b);
	}
	void OnSMP(int slot, Sampler* smp) {
		ID3D11SamplerState *sst = smp == nullptr ? nullptr : smp->getInnerSampler();
		this->Manager->getContext()->VSSetSamplers(slot, 1, &sst);
	}
public:
	~VertexShaderBinding()
	{
		__Shader->Release();
		delete __Shader;
	}
};

class ComputeShaderBinding : public ShaderBinding {
private:
	ID3D11ComputeShader *__Shader;

protected:
	void CreateShader()
	{
		Manager->getDevice()->CreateComputeShader(code, codeLength, NULL, &__Shader);
	}

	void Set()
	{
		Manager->getContext()->OMSetRenderTargetsAndUnorderedAccessViews(0, NULL, NULL, 0, 0, NULL, NULL);

		Manager->getContext()->CSSetShader(__Shader, NULL, 0);
		OnGlobal();
	}

	void OnSRV(int slot, Resource* resource)
	{
		ID3D11ShaderResourceView* view = resource == NULL ? NULL : resource->GetSRV();
		Manager->getContext()->CSSetShaderResources(slot, 1, &view);
	}

	void OnUAV(int slot, Resource* resource)
	{
		ID3D11UnorderedAccessView* uav = resource == NULL ? NULL : resource->GetUAV();
		unsigned int offset = -1;
		this->Manager->getContext()->CSSetUnorderedAccessViews(slot, 1, &uav, &offset);
	}

	void OnCB(int slot, Buffer* cb)
	{
		ID3D11Buffer* b = cb == nullptr ? nullptr : cb->getInternalBuffer();
		this->Manager->getContext()->CSSetConstantBuffers(slot, 1, &b);
	}

	void OnSMP(int slot, Sampler* smp) {
		ID3D11SamplerState *sst = smp == nullptr ? nullptr : smp->getInnerSampler();
		this->Manager->getContext()->CSSetSamplers(slot, 1, &sst);
	}
};

class PixelShaderBinding : public ShaderBinding
{
private:
	ID3D11PixelShader *__Shader;

	int startRT;
	int endRT;
	ID3D11RenderTargetView* renderTargets[128];

	int endUAV;
	int startUAV;
	ID3D11UnorderedAccessView* uavs[128];

	ID3D11DepthStencilView* dsv;

protected:
	void CreateShader()
	{
		Manager->getDevice()->CreatePixelShader(code, codeLength, NULL, &__Shader);
	}
	void Set()
	{
		endRT = 0;
		startRT = 128;

		endUAV = 0;
		startUAV = 128;

		Manager->getContext()->PSSetShader(__Shader, NULL, 0);

		OnGlobal();

		Manager->getContext()->OMSetRenderTargetsAndUnorderedAccessViews(max(0, endRT - startRT + 1), &renderTargets[startRT], dsv, startUAV, max(0, endUAV - startUAV + 1), &uavs[startUAV], NULL);
		//Manager->getContext()->OMSetRenderTargets (1, &renderTargets[0], dsv);
	}
	void OnSRV(int slot, Resource* resource)
	{
		ID3D11ShaderResourceView* view = resource == NULL ? NULL : resource->GetSRV();
		Manager->getContext()->PSSetShaderResources(slot, 1, &view);
	}
	void OnUAV(int slot, Resource* resource)
	{
		endUAV = max(endUAV, slot);
		startUAV = min(startUAV, slot);
		uavs[slot] = resource == NULL ? NULL : resource->GetUAV();
	}
	void RT(Texture2D* resource) {
		RT(0, resource);
	}
	void RT(int slot, Texture2D* resource)
	{
		endRT = max(endRT, slot);
		startRT = min(startRT, slot);
		renderTargets[slot] = resource == NULL ? NULL : resource->GetRTV();
	}
	void DB(Texture2D* texture) {
		if (texture == NULL)
			dsv = NULL;
		dsv = texture == NULL ? NULL : texture->GetDSV();
	}
	void OnCB(int slot, Buffer* cb)
	{
		ID3D11Buffer* b = cb == nullptr ? nullptr : cb->getInternalBuffer();
		this->Manager->getContext()->PSSetConstantBuffers(slot, 1, &b);
	}
	void OnSMP(int slot, Sampler* smp) {
		ID3D11SamplerState *sst = smp == nullptr ? nullptr : smp->getInnerSampler();
		this->Manager->getContext()->PSSetSamplers(slot, 1, &sst);
	}

public:
	PixelShaderBinding()
	{
		ZeroMemory(&renderTargets, ARRAYSIZE(renderTargets)*sizeof(ID3D11RenderTargetView*));
		ZeroMemory(&uavs, ARRAYSIZE(uavs)*sizeof(ID3D11UnorderedAccessView*));
	}
	~PixelShaderBinding()
	{
		__Shader->Release();
		delete __Shader;
	}
};

class GeometryShaderBinding : public ShaderBinding
{
private:
	ID3D11GeometryShader *__Shader;

	void OnUAV(int slot, Resource* resource) {

	}
protected:
	void CreateShader()
	{
		Manager->getDevice()->CreateGeometryShader(code, codeLength, NULL, &__Shader);
	}

	void Set()
	{
		Manager->getContext()->GSSetShader(__Shader, NULL, 0);

		OnGlobal();
	}

	void OnSRV(int slot, Resource* resource)
	{
		ID3D11ShaderResourceView* view = resource == NULL ? NULL : resource->GetSRV();
		Manager->getContext()->GSSetShaderResources(slot, 1, &view);
	}

	void OnCB(int slot, Buffer* cb)
	{
		ID3D11Buffer* b = cb == nullptr ? nullptr : cb->getInternalBuffer();
		this->Manager->getContext()->GSSetConstantBuffers(slot, 1, &b);
	}

	void OnSMP(int slot, Sampler* smp) {
		ID3D11SamplerState *sst = smp == nullptr ? nullptr : smp->getInnerSampler();
		this->Manager->getContext()->GSSetSamplers(slot, 1, &sst);
	}
public:
	~GeometryShaderBinding()
	{
		__Shader->Release();
		delete __Shader;
	}
};

class Presenter
{
private:
	DeviceManager* manager;
	IDXGISwapChain* swapChain;
	Texture2D* backBuffer;
	HWND hWnd;
	ID3D11Texture2D* pBackBuffer;
	IDXGISurface *pBackBufferSurface;
	ID2D1RenderTarget* pRT;
	ID2D1SolidColorBrush *solidColorBrush;
	IDWriteTextFormat* format;

	HRESULT InitDevice(bool fullScreen)
	{
		// Create 2D rt for text rendering
		D3D_DRIVER_TYPE g_driverType;
		ID3D11Device* g_pd3dDevice;
		ID3D11Device1* g_pd3dDevice1;
		D3D_FEATURE_LEVEL g_featureLevel;
		ID3D11DeviceContext* g_pImmediateContext;
		ID3D11DeviceContext1* g_pImmediateContext1;
		IDXGISwapChain1* g_pSwapChain1;
		IDXGISwapChain* g_pSwapChain;

		HRESULT hr = S_OK;

		RECT rc;
		GetClientRect(hWnd, &rc);
		UINT width = rc.right - rc.left;
		UINT height = rc.bottom - rc.top;

		UINT createDeviceFlags = 0;
		createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_DRIVER_TYPE driverTypes[] =
		{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE,
		};
		UINT numDriverTypes = ARRAYSIZE(driverTypes);

		D3D_FEATURE_LEVEL featureLevels[] =
		{
			//D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};
		UINT numFeatureLevels = ARRAYSIZE(featureLevels);

		for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
		{
			g_driverType = driverTypes[driverTypeIndex];
			hr = D3D11CreateDevice(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
				D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

			if (hr == E_INVALIDARG)
			{
				// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
				hr = D3D11CreateDevice(NULL, g_driverType, NULL, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
					D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
			}

			if (SUCCEEDED(hr))
				break;
		}
		if (FAILED(hr))
			return hr;

		// Obtain DXGI factory from device (since we used NULL for pAdapter above)
		IDXGIFactory1* dxgiFactory = NULL;
		{
			IDXGIDevice* dxgiDevice = NULL;
			hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
			if (SUCCEEDED(hr))
			{
				IDXGIAdapter* adapter = NULL;
				hr = dxgiDevice->GetAdapter(&adapter);
				if (SUCCEEDED(hr))
				{
					hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
					adapter->Release();
				}
				dxgiDevice->Release();
			}
		}
		if (FAILED(hr))
			return hr;

		// Create swap chain
		IDXGIFactory2* dxgiFactory2 = NULL;
		hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
		if (dxgiFactory2)
		{
			// DirectX 11.1 or later
			hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
			if (SUCCEEDED(hr))
			{
				(void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));
			}

			DXGI_SWAP_CHAIN_DESC1 sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.Width = width;
			sd.Height = height;
			sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.BufferCount = 1;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsd;
			fsd.RefreshRate.Numerator = 60;
			fsd.RefreshRate.Denominator = 1;
			fsd.Scaling = DXGI_MODE_SCALING_STRETCHED;
			fsd.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
			fsd.Windowed = !fullScreen;

			hr = dxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, hWnd, &sd, fullScreen ? &fsd : NULL, NULL, &g_pSwapChain1);
			if (SUCCEEDED(hr))
			{
				hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
				this->swapChain = g_pSwapChain;
			}
			this->swapChain = g_pSwapChain1;

			dxgiFactory2->Release();
		}
		else
		{
			// DirectX 11.0 systems
			DXGI_SWAP_CHAIN_DESC sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.BufferCount = 1;
			sd.BufferDesc.Width = width;
			sd.BufferDesc.Height = height;
			sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.BufferDesc.RefreshRate.Numerator = 60;
			sd.BufferDesc.RefreshRate.Denominator = 1;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.OutputWindow = hWnd;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.Windowed = !fullScreen;
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;

			hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
			this->swapChain = g_pSwapChain;
		}

		// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
		dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

		dxgiFactory->Release();

		if (FAILED(hr))
			return hr;

		this->manager = new DeviceManager(g_pd3dDevice);

		if (FAILED(hr))
			return hr;

		// Create a render target view
		pBackBuffer = NULL;
		pBackBufferSurface = NULL;

		hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
		if (FAILED(hr))
			return hr;

		pBackBuffer->QueryInterface<IDXGISurface>(&pBackBufferSurface);

		ID2D1Factory* pD2DFactory = NULL;
		hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			&pD2DFactory
			);

		FLOAT dpiX;
		FLOAT dpiY;
		pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);

		D2D1_RENDER_TARGET_PROPERTIES props =
			D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_HARDWARE,
				D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
				dpiX,
				dpiY
				);

		// Create a Direct2D render target            
		pRT = NULL;
		hr = pD2DFactory->CreateDxgiSurfaceRenderTarget(pBackBufferSurface,
			&props,
			&pRT
			);

		IDWriteFactory *dwfactory;
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&dwfactory)
			);

		D2D1_COLOR_F color;
		color.a = 1;
		color.b = 1;
		color.g = 1;
		color.r = 1;

		pRT->CreateSolidColorBrush(color, &solidColorBrush);

		dwfactory->CreateTextFormat(L"Consolas", NULL,
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			20.0f,
			L"en-us",
			&format
			);

		ID3D11RenderTargetView *backBufferRTV;
		hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &backBufferRTV);

		this->backBuffer = new Texture2D(this->manager, pBackBuffer);

		backBuffer->rtv = backBufferRTV;

		return hr;
	}
public:
	Presenter(HWND handleWindow, bool fullScreen)
	{
		this->hWnd = handleWindow;
		HRESULT hr = this->InitDevice(fullScreen);
		if (FAILED(hr))
			MessageBox(hWnd, TEXT("ERROR"), TEXT("Presenter..."), MB_OK);
	}
	inline Texture2D* GetBackBuffer() { return this->backBuffer; }
	inline DeviceManager* GetManager() { return this->manager; }

	template <typename P, typename D> P* Load(D description)
	{
		P* process = new P(this->manager, description);
		((Process<D>*) process)->Initialize();
		return process;
	}

	template <class P> void Run(P* process)
	{
		process->Execute();
	}

	inline void Present() { swapChain->Present(0, 0); line = 0; }

	int line = 0;
	
	void WriteLine(const char* text) {
		int count = strlen(text);
		wchar_t *converted = new wchar_t[count+1];
		OemToCharW(text, converted);
		WriteLineW(converted);
		delete[] converted;
	}

	void WriteLineW(const wchar_t* text) {
		pRT->BeginDraw();

		D2D_RECT_F r;
		r.left = 20;
		r.right = 2000;
		r.top = 20 + line * 20;
		r.bottom = 20 + (line + 1) * 20;
		pRT->DrawTextW(text, lstrlenW(text), format, &r, solidColorBrush);

		pRT->EndDraw();

		line++;
	}
};

class Clearing
{
private:
	DeviceManager* manager;
public:
	Clearing(DeviceManager* manager) {
		this->manager = manager;
	}

	inline void UAV(Resource* resource, float data[4])
	{
		this->manager->getContext()->ClearUnorderedAccessViewFloat(resource->GetUAV(), data);
	}

	inline void UAV(Resource* resource, float value)
	{
		float data[4] = { value, value, value, value };
		UAV(resource, data);
	}

	inline void UAV(Resource* resource, UINT32 data[4])
	{
		this->manager->getContext()->ClearUnorderedAccessViewUint(resource->GetUAV(), data);
	}

	inline void UAV(Resource* resource, unsigned int value)
	{
		UINT data[4] = { value, value, value, value };
		UAV(resource, data);
	}

	inline void UAV(Resource* resource)
	{
		UAV(resource, 0U);
	}

	inline void RTV(Texture2D* texture, float4 data)
	{
		float color[4] = { data.x, data.y, data.z, data.w };
		RTV(texture, color);
	}

	inline void RTV(Texture2D* texture, const float data[4])
	{
		this->manager->getContext()->ClearRenderTargetView(texture->GetRTV(), data);
	}

	inline void RTV(Texture2D* texture, float value)
	{
		float data[4] = { value, value, value, value };
		RTV(texture, data);
	}

	inline void RTV(Texture2D* texture) {
		RTV(texture, 0.0);
	}

	inline void Depth(Texture2D* texture, float depth = 1.0f)
	{
		this->manager->getContext()->ClearDepthStencilView(texture->GetDSV(), 1, depth, 0);
	}
};

class __Screen_VS : public VertexShaderBinding {
protected:
	void Load() {
		LoadCode("Shaders\\__Screen_VS.cso");
		D3D11_INPUT_ELEMENT_DESC des[1]{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		LoadInputLayout(des, 1);
	}
};

template<typename T>
struct Formats { static const DXGI_FORMAT Value = DXGI_FORMAT_UNKNOWN; };
template<>
struct Formats<char> { static const DXGI_FORMAT Value = DXGI_FORMAT_R8_SINT; };
template<>
struct Formats<float> { static const DXGI_FORMAT Value = DXGI_FORMAT_R32_FLOAT; };
template<>
struct Formats <int> { static const DXGI_FORMAT Value = DXGI_FORMAT_R32_SINT; };
template<>
struct Formats <unsigned int>{ static const DXGI_FORMAT Value = DXGI_FORMAT_R32_UINT; };
template<>
struct Formats <float2>{ static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32_FLOAT; };
template<>
struct Formats <float3>{ static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32_FLOAT; };
template<>
struct Formats <float4>{ static const DXGI_FORMAT Value = DXGI_FORMAT_R32G32B32A32_FLOAT; };

class Builder
{
private:
	DeviceManager* manager;
	Buffer* CreateVB(const void* vertices, int dataStride, int verticesCount)
	{
		ID3D11Buffer* buffer;

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = dataStride * verticesCount;
		bd.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;//  // D3D11_BIND_VERTEX_BUFFER;
		bd.StructureByteStride = dataStride;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = vertices;
		InitData.SysMemPitch = dataStride*verticesCount;
		InitData.SysMemSlicePitch = dataStride*verticesCount;
		HRESULT hr = this->manager->getDevice()->CreateBuffer(&bd, &InitData, &buffer);
		if (FAILED(hr))
			return NULL;

		return new Buffer(this->manager, buffer, dataStride);
	};
public:
	Builder(DeviceManager* manager) { this->manager = manager; }

	template<class T> Buffer* VertexBuffer(const T* vertices, int count)
	{
		return CreateVB((void*)vertices, sizeof(T), count);
	}
	Buffer* IndexBuffer(const int* indices, int indicesCount) {

		if (indices == NULL)
			return NULL;

		ID3D11Buffer* buffer;

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(int) * indicesCount;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = indices;
		HRESULT hr = this->manager->getDevice()->CreateBuffer(&bd, &InitData, &buffer);
		if (FAILED(hr))
			return NULL;

		return new Buffer(this->manager, buffer, sizeof(int));
	};

	Buffer* ConstantBuffer(int stride)
	{
		ID3D11Buffer* buffer;

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		// Create the constant buffers
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = stride;
		bd.StructureByteStride = stride;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		HRESULT hr = this->manager->getDevice()->CreateBuffer(&bd, NULL, &buffer);
		if (FAILED(hr))
			return NULL;

		return new Buffer(this->manager, buffer, stride);
	};

	template<class T> Buffer* StructuredBuffer(int size,
		D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
		D3D11_BIND_FLAG flags = (D3D11_BIND_FLAG)(D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS))
	{
		ID3D11Buffer* buffer;

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		// Create the buffers
		bd.Usage = usage;
		bd.ByteWidth = sizeof(T) * size;
		bd.BindFlags = flags;
		bd.CPUAccessFlags = usage == D3D11_USAGE_DEFAULT ? 0 : usage == D3D11_USAGE_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : usage == D3D11_USAGE_STAGING ? D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE : 0;
		bd.MiscFlags = usage == D3D11_USAGE_STAGING ? 0 : D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.StructureByteStride = sizeof(T);
		HRESULT hr = this->manager->getDevice()->CreateBuffer(&bd, NULL, &buffer);
		if (FAILED(hr))
			return NULL;

		return new Buffer(this->manager, buffer, sizeof(T));
	}

	template<class T> Buffer* StructuredBuffer(T* data, int size,
		D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
		D3D11_BIND_FLAG flags = (D3D11_BIND_FLAG)(D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS))
	{
		ID3D11Buffer* buffer;

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		// Create the buffers
		bd.Usage = usage;
		bd.ByteWidth = sizeof(T) * size;
		bd.BindFlags = flags;
		bd.CPUAccessFlags = usage == D3D11_USAGE_DEFAULT ? 0 : usage == D3D11_USAGE_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : usage == D3D11_USAGE_STAGING ? D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE : 0;
		bd.MiscFlags = usage == D3D11_USAGE_STAGING ? 0 : D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.StructureByteStride = sizeof(T);
		D3D11_SUBRESOURCE_DATA subresourceData;
		subresourceData.pSysMem = (void*)data;
		subresourceData.SysMemPitch = sizeof(T)*size;
		subresourceData.SysMemSlicePitch = sizeof(T)*size;
		HRESULT hr = this->manager->getDevice()->CreateBuffer(&bd, &subresourceData, &buffer);
		if (FAILED(hr))
			return NULL;

		return new Buffer(this->manager, buffer, sizeof(T));
	}

	template<class T> Texture2D* Texture(int width, int height,
		D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
		D3D11_BIND_FLAG flags = (D3D11_BIND_FLAG)(D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET))
	{
		ID3D11Texture2D* texture;

		auto format = Formats<T>::Value;

		D3D11_TEXTURE2D_DESC td;
		ZeroMemory(&td, sizeof(D3D11_TEXTURE2D_DESC));
		// Create the buffers
		td.Usage = usage;
		td.Format = format;
		td.Width = width;
		td.Height = height;
		td.BindFlags = flags;
		td.CPUAccessFlags = usage == D3D11_USAGE_DEFAULT ? 0 : usage == D3D11_USAGE_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : usage == D3D11_USAGE_STAGING ? D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE : 0;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.SampleDesc.Count = 1;
		td.SampleDesc.Quality = 0;
		HRESULT hr = this->manager->getDevice()->CreateTexture2D(&td, NULL, &texture);
		if (FAILED(hr))
			return NULL;

		return new Texture2D(this->manager, texture);
	}


	template<class T> Buffer* ConstantBuffer()
	{
		return ConstantBuffer(sizeof(T));
	}
	Texture2D* DepthBuffer(int width, int height)
	{
		ID3D11Texture2D* texture;

		// Create depth stencil texture
		D3D11_TEXTURE2D_DESC descDepth;
		ZeroMemory(&descDepth, sizeof(descDepth));
		descDepth.Width = width;
		descDepth.Height = height;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		//descDepth.Format = DXGI_FORMAT_D32_FLOAT;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D11_USAGE_DEFAULT;
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;
		HRESULT hr = manager->getDevice()->CreateTexture2D(&descDepth, NULL, &texture);

		return new Texture2D(manager, texture);
	}

	Sampler* Sampler(D3D11_SAMPLER_DESC &desc) {
		ID3D11SamplerState *sst;
		manager->device->CreateSamplerState(&desc, &sst);
		return new ::Sampler(sst);
	}

	::Sampler* Sampler(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE U, D3D11_TEXTURE_ADDRESS_MODE V, D3D11_TEXTURE_ADDRESS_MODE W)
	{
		D3D11_SAMPLER_DESC desc;
		desc.AddressU = U;
		desc.AddressV = V;
		desc.AddressW = W;
		desc.Filter = filter;
		desc.MinLOD = FLT_MIN;
		desc.MaxLOD = FLT_MAX;
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 16;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		return Sampler(desc);
	}

	::Sampler* BilinearSampler() {
		return Sampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	}
};

class Loader
{
private:
	DeviceManager* manager;
public:
	Loader(DeviceManager* manager) { this->manager = manager; }
	template <class S> void Shader(S* &shader)
	{
		shader = new S();

		((ShaderBinding*)shader)->Bind(this->manager);

		((ShaderBinding*)shader)->Load();
	}

	template <typename P, typename D> void Process(P* &process, D description) {
		process = new P(manager, description);
		((::Process<D>*)process)->Initialize();
	}
	template <typename P> void Process(P* &process) {
		process = new P(manager, NoDescription());
		((::Process<NoDescription>*)process)->Initialize();
	}

	Texture2D* Texture(const char* filePath) {
		ID3D11Resource* result;
		int hr = CreateDDSTextureFromFile(manager->getDevice(), filePath, &result, NULL);
		if (FAILED(hr))
			return nullptr;
		return new Texture2D(manager, (ID3D11Texture2D*)result);
	}
};

class Setter
{
	friend class DeviceManager;
private:
	DeviceManager* manager;
	D3D11_RASTERIZER_DESC rasterizer;
	D3D11_DEPTH_STENCIL_DESC depth;
	D3D11_BLEND_DESC blending;

	D3D11_RASTERIZER_DESC default_rasterizer;
	D3D11_DEPTH_STENCIL_DESC default_depth;
	D3D11_BLEND_DESC default_blending;

	void Setter::UpdateRasterizerState() {
		ID3D11RasterizerState *rs;
		manager->getDevice()->CreateRasterizerState(&rasterizer, &rs);
		manager->getContext()->RSSetState(rs);
	}

	void UpdateDepthTestState() {
		ID3D11DepthStencilState *ds;
		manager->getDevice()->CreateDepthStencilState(&depth, &ds);
		manager->getContext()->OMSetDepthStencilState(ds, 0);
	}

	float blendFactor[4];
	void UpdateBlending() {
		ID3D11BlendState *bs;
		manager->getDevice()->CreateBlendState(&blending, &bs);
		manager->getContext()->OMSetBlendState(bs, blendFactor, 0xFFFFFFFF);
	}

	__Screen_VS *screenVS;

	void InitializeShader() {
		manager->loader->Shader(screenVS);
	}
public:
	Setter(DeviceManager* manager) {
		this->manager = manager;
		ZeroMemory(&rasterizer, sizeof(D3D11_RASTERIZER_DESC));
		rasterizer.FillMode = D3D11_FILL_SOLID;
		rasterizer.CullMode = D3D11_CULL_NONE;
		default_rasterizer = rasterizer;

		ZeroMemory(&depth, sizeof(D3D11_DEPTH_STENCIL_DESC));
		depth.DepthEnable = true;
		depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth.DepthFunc = D3D11_COMPARISON_LESS;
		depth.StencilEnable = false;
		depth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		depth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		depth.FrontFace.StencilFunc = depth.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depth.FrontFace.StencilPassOp = depth.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depth.FrontFace.StencilFailOp = depth.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		default_depth = depth;

		ZeroMemory(&blending, sizeof(D3D11_BLEND_DESC));
		blending.AlphaToCoverageEnable = false;
		blending.IndependentBlendEnable = false;
		blending.RenderTarget[0].BlendEnable = false;
		blending.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blending.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blending.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		blending.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blending.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		blending.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blending.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE; 
		default_blending = blending;
	}
	~Setter() {
		delete screenVS;
	}

	inline Setter* Clean()
	{
		depth = default_depth;
		UpdateDepthTestState();
		rasterizer = default_rasterizer;
		UpdateRasterizerState();
		blending = default_blending;
		ZeroMemory(blendFactor,16);
		UpdateBlending();
		return this;
	}

	inline DeviceManager* Viewport(float width, float height)
	{
		return Viewport(0, 0, width, height, 0, 1);
	}

	inline DeviceManager* Pipeline(VertexShaderBinding* vs, PixelShaderBinding *ps)
	{
		return Pipeline(vs, NULL, ps);
	}

	DeviceManager* Pipeline(VertexShaderBinding* vs, GeometryShaderBinding* gs, PixelShaderBinding *ps)
	{
		if (manager->vs != nullptr)
			manager->vs->Unset();
		if (manager->gs != nullptr)
			manager->gs->Unset();
		if (manager->ps != nullptr)
			manager->ps->Unset();
		if (manager->cs != nullptr)
			manager->cs->Unset();

		manager->getContext()->OMSetRenderTargetsAndUnorderedAccessViews(0, NULL, NULL, 0, 0, NULL, NULL);

		manager->vs = vs;
		manager->gs = gs;
		manager->ps = ps;
		manager->cs = nullptr;

		if (vs != NULL)
			((ShaderBinding*)vs)->Set();
		else
			manager->getContext()->VSSetShader(nullptr, nullptr, 0);

		if (gs != NULL)
			((ShaderBinding*)gs)->Set();
		else
			manager->getContext()->GSSetShader(nullptr, nullptr, 0);

		if (ps != NULL)
			((ShaderBinding*)ps)->Set();
		else
			manager->getContext()->PSSetShader(nullptr, nullptr, 0);

		return this->manager;
	}

	DeviceManager* Computation(ComputeShaderBinding* cs) {
		if (manager->vs != nullptr)
			manager->vs->Unset();
		if (manager->gs != nullptr)
			manager->gs->Unset();
		if (manager->ps != nullptr)
			manager->ps->Unset();
		if (manager->cs != nullptr)
			manager->cs->Unset();

		manager->getContext()->OMSetRenderTargetsAndUnorderedAccessViews(0, NULL, NULL, 0, 0, NULL, NULL);
		manager->vs = nullptr;
		manager->gs = nullptr;
		manager->ps = nullptr;
		manager->cs = cs;

		if (cs != nullptr)
			((ShaderBinding*)cs)->Set();
		else
			manager->getContext()->CSSetShader(nullptr, nullptr, 0);

		return manager;
	}

	DeviceManager* Viewport(float x, float y, float width, float height, float minZ, float maxZ)
	{
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = x;
		viewport.TopLeftY = y;
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = minZ;
		viewport.MaxDepth = maxZ;
		this->manager->getContext()->RSSetViewports(1, &viewport);
		return this->manager;
	}

	DeviceManager* Pipeline(PixelShaderBinding *ps) {
		return Pipeline(screenVS, ps);
	}

	DeviceManager* Fill(D3D11_FILL_MODE mode)
	{
		rasterizer.FillMode = mode;
		UpdateRasterizerState();
		return manager;
	}

	DeviceManager* Cull(D3D11_CULL_MODE mode)
	{
		rasterizer.CullMode = mode;
		UpdateRasterizerState();
		return manager;
	}

	DeviceManager* Blending(D3D11_BLEND src = D3D11_BLEND_SRC_ALPHA, D3D11_BLEND dst = D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP op = D3D11_BLEND_OP_ADD, int target = 0)
	{
		blending.RenderTarget[target].BlendEnable = true;
		blending.RenderTarget[target].BlendOp = op;
		blending.RenderTarget[target].SrcBlend = src;
		blending.RenderTarget[target].DestBlend = dst;
		UpdateBlending();
		return manager;
	}

	DeviceManager* BlendingAlpha(D3D11_BLEND src = D3D11_BLEND_SRC_ALPHA, D3D11_BLEND dst = D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP op = D3D11_BLEND_OP_ADD, int target = 0)
	{
		blending.RenderTarget[target].BlendEnable = true;
		blending.RenderTarget[target].BlendOpAlpha = op;
		blending.RenderTarget[target].SrcBlendAlpha = src;
		blending.RenderTarget[target].DestBlendAlpha = dst;
		UpdateBlending();
		return manager;
	}

	DeviceManager* NoBlending(int target = 0) {
		blending.RenderTarget[target].BlendEnable = true;
		UpdateBlending();
		return manager;
	}

	DeviceManager* DepthTest(bool enable = true, bool write = true, D3D11_COMPARISON_FUNC comparison = D3D11_COMPARISON_LESS) {
		depth.DepthEnable = enable;
		depth.DepthWriteMask = write ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		depth.DepthFunc = comparison;
		UpdateDepthTestState();
		return manager;
	}
	DeviceManager* DepthTestOnly(D3D11_COMPARISON_FUNC comparison = D3D11_COMPARISON_LESS) {
		DepthTest(true, false, comparison);
		return manager;
	}
	DeviceManager* NoDepthTest() {
		DepthTest(false);
		return manager;
	}
};

class Drawer
{
	friend class DeviceManager;
private:
	DeviceManager* manager;
	ID3D11Buffer* lastVB;
	ID3D11Buffer* lastIB;
	Buffer* screenVB;

	void InitializeVertexes() {
		float2 screenVertexes[6] = { float2(-1,-1), float2(1,-1), float2(1,1), float2(-1,-1), float2(1,1), float2(-1,1) };
		screenVB = manager->builder->VertexBuffer<float2>(screenVertexes, 6);
	}
public:
	Drawer(DeviceManager* manager) {
		this->manager = manager;
	}

	DeviceManager* Screen() {
		return Primitive(screenVB, 0, 6);
	}

	DeviceManager* Primitive(const Buffer* vb, const Buffer* ib, int start = 0, int count = MAXINT, D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
	{
		if (count == MAXINT) {
			count = ib->getByteLength() / ib->getStride() - start;
		}
		UINT stride = vb->getStride();
		UINT offset = 0;
		ID3D11Buffer* _vb = vb->getInternalBuffer();

		if (manager->vs)
			manager->vs->UpdateLocal();
		if (manager->gs)
			manager->gs->UpdateLocal();
		if (manager->ps)
			manager->ps->UpdateLocal();

		manager->getContext()->IASetPrimitiveTopology(topology);
		if (_vb != lastVB)
		{
			lastVB = _vb;
			manager->getContext()->IASetVertexBuffers(0, 1, &_vb, &stride, &offset);
		}
		ID3D11Buffer* _ib = ib->getInternalBuffer();
		if (_ib != lastIB) {
			lastIB = _ib;
			manager->getContext()->IASetIndexBuffer(_ib, DXGI_FORMAT_R32_UINT, 0);
		}
		manager->getContext()->DrawIndexed(count, start, 0);

		return manager;
	}

	DeviceManager* Primitive(const Buffer* vb, int start = 0, int count = MAXINT, D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
	{
		UINT stride = vb->getStride();
		UINT offset = 0;
		ID3D11Buffer* _vb = vb->getInternalBuffer();

		if (count == MAXINT) {
			count = vb->getByteLength() / stride - start;
		}
		if (manager->vs)
			manager->vs->UpdateLocal();
		if (manager->gs)
			manager->gs->UpdateLocal();
		if (manager->ps)
			manager->ps->UpdateLocal();

		manager->getContext()->IASetPrimitiveTopology(topology);
		if (_vb != lastVB)
		{
			lastVB = _vb;
			manager->getContext()->IASetVertexBuffers(0, 1, &_vb, &stride, &offset);
		}

		manager->getContext()->Draw(count, start);

		return manager;
	}
};

DeviceManager::DeviceManager(ID3D11Device* device) : loader(new Loader(this)), setter(new Setter(this)), builder(new Builder(this)), clearing(new Clearing(this)), drawer(new Drawer(this))
{
	this->device = device;
	device->GetImmediateContext(&this->context);
	this->setter->InitializeShader();
	this->drawer->InitializeVertexes();
};

void DeviceManager::Dispatch(int xCount, int yCount, int zCount)
{
	if (this->cs != nullptr)
		this->cs->UpdateLocal();
	context->Dispatch(xCount, yCount, zCount);
}

void DeviceManager::Perform(PixelShaderBinding *ps, int width, int height)
{
	setter->Clean();
	setter->Pipeline(ps);
	setter->Viewport(width, height);
	setter->NoDepthTest();

	drawer->Screen();
}

class Executable {
public:
	virtual void Execute() = 0;
};

template<typename D>
class Process : Executable
{
	friend class DeviceManager;
	friend class Presenter;
	friend class Loader;
private:
	DeviceManager* manager;
protected:
	virtual void Initialize() { }
	void Execute() { }
	virtual void Destroy() { }
	template<typename P>
	void Run(P* process) {
		((Executable*)process)->Execute();
	}
	void Perform(PixelShaderBinding *ps, int width, int height) {
		manager->Perform(ps, width, height);
	}
	inline void Dispatch(int xCount, int yCount, int zCount) {
		manager->Dispatch(xCount, yCount, zCount);
	}
public:
	Setter* const setter;
	Builder* const builder;
	Loader* const loader;
	Clearing* const clearing;
	Drawer* const drawer;
	const D Description;
	Process(DeviceManager* manager, D description) :manager(manager),setter(manager->setter), builder(manager->builder), loader(manager->loader), clearing(manager->clearing), drawer(manager->drawer), Description(description) {
	}
	~Process() {
		Destroy();
	}
};

// Implementations
