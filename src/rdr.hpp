#include <d3d11.h>


struct sModelVtx {
	vec3 pos;
	vec3 nrm;
};


class cBufferBase : noncopyable {
protected:
	moveable_ptr<ID3D11Buffer> mpBuf;
public:
	cBufferBase() = default;
	~cBufferBase() { deinit(); }
	cBufferBase(cBufferBase&& o) : mpBuf(std::move(o.mpBuf)) {}
	cBufferBase& operator=(cBufferBase&& o) {
		mpBuf = std::move(o.mpBuf);
		return *this;
	}

	void deinit() {
		if (mpBuf) {
			mpBuf->Release();
			mpBuf.reset();
		}
	}

protected:
	void init_immutable(ID3D11Device* pDev,
		void const* pData, uint32_t size,
		D3D11_BIND_FLAG bind);
};

class cConstBufferBase : public cBufferBase {
public:
	void set_VS(ID3D11DeviceContext* pCtx, UINT slot) {
		ID3D11Buffer* bufs[1] = { mpBuf };
		pCtx->VSSetConstantBuffers(slot, 1, bufs);
	}

	void set_PS(ID3D11DeviceContext* pCtx, UINT slot) {
		ID3D11Buffer* bufs[1] = { mpBuf };
		pCtx->PSSetConstantBuffers(slot, 1, bufs);
	}
protected:
	void init(ID3D11Device* pDev, size_t size);
	void update(ID3D11DeviceContext* pCtx, void const* pData, size_t size);
};

template <typename T>
class cConstBuffer : public cConstBufferBase {
public:
	T mData;

	void init(ID3D11Device* pDev) {
		cConstBufferBase::init(pDev, sizeof(T));
	}

	void update(ID3D11DeviceContext* pCtx) {
		cConstBufferBase::update(pCtx, &mData, sizeof(T));
	}
};

template <typename T, int slot>
class cConstBufferSlotted : public cConstBuffer<T> {
public: 
	void set_VS(ID3D11DeviceContext* pCtx) {
		cConstBuffer<T>::set_VS(pCtx, slot);
	}
	void set_PS(ID3D11DeviceContext* pCtx) {
		cConstBuffer<T>::set_PS(pCtx, slot);
	}
};

class cVertexBuffer : public cBufferBase {
	uint32_t mVtxSize = 0;
public:
	cVertexBuffer() = default;
	cVertexBuffer(cVertexBuffer&& o) : cBufferBase(std::move(o)), mVtxSize(o.mVtxSize) {}
	cVertexBuffer& operator=(cVertexBuffer&& o) {
		mVtxSize = o.mVtxSize;
		cBufferBase::operator=(std::move(o));
		return *this;
	}

	void init(ID3D11Device* pDev, void const* pVtxData, uint32_t vtxCount, uint32_t vtxSize);

	void set(ID3D11DeviceContext* pCtx, uint32_t slot, uint32_t offset) const;
};

class cIndexBuffer : public cBufferBase {
	DXGI_FORMAT mFormat;
public:
	cIndexBuffer() = default;
	cIndexBuffer(cIndexBuffer&& o) : cBufferBase(std::move(o)), mFormat(o.mFormat) {}
	cIndexBuffer& operator=(cIndexBuffer&& o) {
		mFormat = o.mFormat;
		cBufferBase::operator=(std::move(o));
		return *this;
	}

	void init(ID3D11Device* pDev, void const* pIdxData, uint32_t idxCount, DXGI_FORMAT format);
	void set(ID3D11DeviceContext* pCtx, uint32_t offset) const;
};


struct sCameraCBuf {
	DirectX::XMMATRIX viewProj;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX proj;
	DirectX::XMVECTOR camPos;
};

struct sMeshCBuf {
	DirectX::XMMATRIX wmtx;
};

class cConstBufStorage {
public:
	cConstBufferSlotted<sCameraCBuf, 0> mCameraCBuf;
	cConstBufferSlotted<sMeshCBuf, 1> mMeshCBuf;

	cConstBufStorage(ID3D11Device* pDev) {
		mCameraCBuf.init(pDev);
		mMeshCBuf.init(pDev);
	}

	static cConstBufStorage& get();
};
