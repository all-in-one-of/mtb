#include "common.hpp"
#include "gfx.hpp"

#include <SDL_rwops.h>

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

	ID3D11RenderTargetView* pv = mRTV.mpRTV;
	mDev.mpImmCtx->OMSetRenderTargets(1, &pv, NULL);

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
