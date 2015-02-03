#include "common.hpp"
#include "math.hpp"
#include "gfx.hpp"

#include <SDL_rwops.h>

cGfx::sDev::sDev(DXGI_SWAP_CHAIN_DESC& sd, UINT flags) {
	D3D_FEATURE_LEVEL lvl;

	auto hr = ::D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		nullptr, 0,
		D3D11_SDK_VERSION,
		&sd,
		mpSwapChain.pp(),
		mpDev.pp(),
		&lvl,
		mpImmCtx.pp());
	if (!SUCCEEDED(hr))
		throw sD3DException(hr, "D3D11CreateDeviceAndSwapChain failed");
}

cGfx::sDepthStencilBuffer::sDepthStencilBuffer(sDev& dev, UINT w, UINT h) {
	auto desc = D3D11_TEXTURE2D_DESC();
	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	HRESULT hr = dev.mpDev->CreateTexture2D(&desc, nullptr, mpTex.pp());
	if (!SUCCEEDED(hr))
		throw sD3DException(hr, "CreateTexture2D failed: depth stencil buffer");
	
	hr = dev.mpDev->CreateDepthStencilView(mpTex, nullptr, mpDSView.pp());
	if (!SUCCEEDED(hr))
		throw sD3DException(hr, "CreateDepthStencilView failed");
}

cGfx::sRTView::sRTView(sDev& dev) {
	ID3D11Texture2D* pBack = nullptr;
	HRESULT hr = dev.mpSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBack);
	if (!SUCCEEDED(hr))
		throw sD3DException(hr, "GetBuffer failed: back buffer");

	hr = dev.mpDev->CreateRenderTargetView(pBack, nullptr, mpRTV.pp());
	pBack->Release();

	if (!SUCCEEDED(hr))
		throw sD3DException(hr, "CreateRenderTargetView failed");
}

cGfx::sRSState::sRSState(sDev& dev) {
	auto desc = D3D11_RASTERIZER_DESC();
	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_NONE;
	desc.FrontCounterClockwise = FALSE;
	desc.DepthClipEnable = TRUE;
	desc.ScissorEnable = FALSE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;

	HRESULT hr = dev.mpDev->CreateRasterizerState(&desc, mpRSState.pp());
	if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateRasterizerState");

	dev.mpImmCtx->RSSetState(mpRSState);
}

cGfx::sDSState::sDSState(sDev& dev) {
	auto desc = D3D11_DEPTH_STENCIL_DESC();
	desc.DepthEnable = false;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS;

	desc.StencilEnable = false;

	HRESULT hr = dev.mpDev->CreateDepthStencilState(&desc, mpDSState.pp());
	if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateDepthStencilState");

	dev.mpImmCtx->OMSetDepthStencilState(mpDSState, 0);
}

cGfx::cGfx(HWND hwnd) {
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(hwnd, &rc);
	UINT w = rc.right - rc.left;
	UINT h = rc.bottom - rc.top;

	UINT devFlags = 0;
	devFlags |= D3D11_CREATE_DEVICE_DEBUG;

	auto sd = DXGI_SWAP_CHAIN_DESC();
	sd.BufferCount = 1;
	sd.BufferDesc.Width = w;
	sd.BufferDesc.Height = h;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hwnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	mDev = sDev(sd, devFlags);
	mRTV = sRTView(mDev);
	mDS = sDepthStencilBuffer(mDev, w, h);

	ID3D11RenderTargetView* pv = mRTV.mpRTV;
	ID3D11DepthStencilView* pdv = mDS.mpDSView;
	mDev.mpImmCtx->OMSetRenderTargets(1, &pv, pdv);

	auto vp = D3D11_VIEWPORT();
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (FLOAT)w;
	vp.Height = (FLOAT)h;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	mDev.mpImmCtx->RSSetViewports(1, &vp);

	mRSState = sRSState(mDev);
}

void cGfx::begin_frame() {
	float clrClr[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	mDev.mpImmCtx->ClearRenderTargetView(mRTV.mpRTV, clrClr);
	mDev.mpImmCtx->ClearDepthStencilView(mDS.mpDSView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void cGfx::end_frame() {
	mDev.mpSwapChain->Present(0, 0);
}


static cShaderBytecode LoadShaderFile(cstr filepath) {
	auto rw = SDL_RWFromFile(filepath, "rb");
	if (!rw) 
		return cShaderBytecode();
	struct scope {
		SDL_RWops* p;
		scope(SDL_RWops* p) : p(p) {}
		~scope() { SDL_RWclose(p); }
	} scp(rw);

	Sint64 size = SDL_RWsize(rw);
	if (size < 0) 
		return cShaderBytecode();
	
	auto pdata = std::make_unique<char[]>(size);
	SDL_RWread(rw, pdata.get(), size, 1);
	
	return cShaderBytecode(std::move(pdata), size);
}


cShader* cShaderStorage::load_VS(cstr filepath) {
	auto code = LoadShaderFile(filepath);
	if (code.get_size() == 0) return nullptr;

	auto pDev = get_gfx().get_dev();
	ID3D11VertexShader* pVS;
	HRESULT hr = pDev->CreateVertexShader(code.get_code(), code.get_size(), nullptr, &pVS);
	if (!SUCCEEDED(hr)) throw sD3DException(hr, "cShaderStorage::LoadVS CreateVertexShader failed");

	return put_shader(pVS, std::move(code));
}

cShader* cShaderStorage::load_PS(cstr filepath) {
	auto code = LoadShaderFile(filepath);
	if (code.get_size() == 0) return nullptr;

	auto pDev = get_gfx().get_dev();
	ID3D11PixelShader* pPS;
	HRESULT hr = pDev->CreatePixelShader(code.get_code(), code.get_size(), nullptr, &pPS);
	if (!SUCCEEDED(hr)) throw sD3DException(hr, "cShaderStorage::LoadPS CreatePixelShader failed");

	return put_shader(pPS, std::move(code));
}

cShader* cShaderStorage::put_shader(ID3D11DeviceChild* pShader, cShaderBytecode&& code) {
	auto vs = std::make_unique<cShader>(pShader, std::move(code));
	auto res = vs.get();
	mShaders.push_back(std::move(vs));
	return res;
}
