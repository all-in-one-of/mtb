#include "common.hpp"
#include "texture.hpp"

#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"

#include <locale>

bool cTexture::load(ID3D11Device* pDev, cstr filepath) {
	unload();

	bool isDDS = filepath.ends_with(".dds");
	
	wchar_t buf[512];
	size_t tmp;
	if (0 != ::mbstowcs_s(&tmp, buf, filepath, _TRUNCATE)) {
		return false;
	}

	HRESULT hr;
	if (isDDS) {
		hr = DirectX::CreateDDSTextureFromFile(pDev, buf, mpTex.pp(), mpView.pp());
	}
	else {
		hr = DirectX::CreateWICTextureFromFile(pDev, buf, mpTex.pp(), mpView.pp());
	}

	return SUCCEEDED(hr);
}

bool cTexture::create2d1_rgba8(ID3D11Device* pDev, void* data, uint32_t w, uint32_t h) {
	unload();

	auto desc = D3D11_TEXTURE2D_DESC();
	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	ID3D11Texture2D* pTex = nullptr;
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = data;
	initData.SysMemPitch = desc.Width * 4;
	initData.SysMemSlicePitch = 0;
	HRESULT hr = pDev->CreateTexture2D(&desc, &initData, &pTex);
	if (!SUCCEEDED(hr))
		return false;
	mpTex.reset(pTex);

	auto srvDesc = D3D11_SHADER_RESOURCE_VIEW_DESC();
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	hr = pDev->CreateShaderResourceView(mpTex, &srvDesc, mpView.pp());

	return SUCCEEDED(hr);
}

void cTexture::unload() {
	if (mpTex) {
		mpTex->Release();
		mpTex.reset(); 
	}
	if (mpView) {
		mpView->Release();
		mpView.reset();
	}
}

void cTexture::set_PS(ID3D11DeviceContext* pCtx, UINT slot) {
	ID3D11ShaderResourceView* views[1] = { mpView };
	pCtx->PSSetShaderResources(slot, 1, views);
}

void cTexture::set_null_PS(ID3D11DeviceContext* pCtx, UINT slot) {
	ID3D11ShaderResourceView* views[1] = { nullptr };
	pCtx->PSSetShaderResources(slot, 1, views);
}
