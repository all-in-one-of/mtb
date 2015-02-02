#include <d3d11.h>

#include "common.hpp"
#include "math.hpp"
#include "rdr.hpp"

void cConstBufferBase::init(ID3D11Device* pDev, size_t size) {
	auto desc = D3D11_BUFFER_DESC();
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.ByteWidth = (UINT)size;

	HRESULT hr = pDev->CreateBuffer(&desc, nullptr, mpBuf.pp());
	if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateBuffer const buffer failed");
}

void cConstBufferBase::update(ID3D11DeviceContext* pCtx, void const* pData, size_t size) {
	D3D11_MAPPED_SUBRESOURCE mr;
	HRESULT hr = pCtx->Map(mpBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mr);
	if (SUCCEEDED(hr)) {
		::memcpy(mr.pData, pData, size);
		pCtx->Unmap(mpBuf, 0);
	}
}


void cBufferBase::init_immutable(ID3D11Device* pDev, void const* pData, uint32_t size, D3D11_BIND_FLAG bind) {
	auto desc = D3D11_BUFFER_DESC();
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = bind;
	
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = pData;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	HRESULT hr = pDev->CreateBuffer(&desc, &initData, mpBuf.pp());
	if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateBuffer immutable failed");
}

void cVertexBuffer::init(ID3D11Device* pDev, void const* pVtxData, uint32_t vtxCount, uint32_t vtxSize) {
	init_immutable(pDev, pVtxData, vtxCount * vtxSize, D3D11_BIND_VERTEX_BUFFER);
	mVtxSize = vtxSize;
}

void cVertexBuffer::set(ID3D11DeviceContext* pCtx, uint32_t slot, uint32_t offset) const {
	pCtx->IASetVertexBuffers(slot, 1, &mpBuf.p, &mVtxSize, &offset);
}

void cIndexBuffer::init(ID3D11Device* pDev, void const* pIdxData, uint32_t idxCount, DXGI_FORMAT format) {
	uint32_t size = 0;
	switch (format) {
	case DXGI_FORMAT_R16_UINT: size = 2 * idxCount; break;
	case DXGI_FORMAT_R32_UINT: size = 4 * idxCount; break;
	}

	init_immutable(pDev, pIdxData, size, D3D11_BIND_INDEX_BUFFER);
	mFormat = format;
}

void cIndexBuffer::set(ID3D11DeviceContext* pCtx, uint32_t offset) const {
	pCtx->IASetIndexBuffer(mpBuf, mFormat, offset);
}
