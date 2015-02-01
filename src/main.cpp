#include <SDL.h>
#include <SDL_main.h>
#include <SDL_syswm.h>

#include "common.hpp"
#include "gfx.hpp"

#include "math.hpp"
namespace dx = DirectX;
using dx::XMFLOAT4;


class cSDLInit {
public:
	cSDLInit() {
		Uint32 initFlags = SDL_INIT_VIDEO;
		SDL_Init(initFlags);
	}

	~cSDLInit() {
		SDL_Quit();
	}
};

class cSDLWindow {
	moveable_ptr<SDL_Window> win;

public:
	cSDLWindow(cstr title, int x, int y, int w, int h, Uint32 flags) {
		init(title, x, y, w, h, flags);
	}
	cSDLWindow(cstr title, int w, int h, Uint32 flags) {
		init(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, flags);
	}
	~cSDLWindow() {
		deinit();
	}

	cSDLWindow(cSDLWindow&& o) : win(std::move(o.win)) { }
	cSDLWindow(cSDLWindow& o) = delete;

	HWND get_handle() const {
		HWND hwnd = 0;

		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (SDL_GetWindowWMInfo(win, &info)) {
			hwnd = info.info.win.window;
		}

		return hwnd;
	}

private:
	void init(cstr title, int x, int y, int w, int h, Uint32 flags) {
		win = SDL_CreateWindow(title.p, x, y, w, h, flags);
	}
	void deinit() {
		if (!win) return;

		SDL_DestroyWindow(win);
		win.reset();
	}
};

class cCamera {
public:
	struct sView {
		dx::XMMATRIX mView;
		dx::XMMATRIX mProj;
		dx::XMMATRIX mViewProj;

		void calc_view(dx::XMVECTOR const& pos, dx::XMVECTOR const& tgt, dx::XMVECTOR const& up) {
			mView = dx::XMMatrixLookAtRH(pos, tgt, up);
		}

		void calc_proj(float fovY, float aspect, float nearZ, float farZ) {
			mProj = dx::XMMatrixPerspectiveFovRH(fovY, aspect, nearZ, farZ);
		}

		void calc_viewProj() {
			mViewProj = mView * mProj;
		}
	};

	dx::XMVECTOR mPos;
	dx::XMVECTOR mTgt;
	dx::XMVECTOR mUp;

	sView mView;

	float mFovY;
	float mAspect;
	float mNearZ;
	float mFarZ;

public:
	void init() {
		mPos = dx::XMVectorSet(1.0f, 2.0f, 3.0f, 1.0f);
		//mPos = dx::XMVectorSet(0.0f, 0.0f, 3.0f, 1.0f);
		mTgt = dx::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		mUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);


		mFovY = DEG2RAD(45.0f);
		//mFovY = dx::XM_PIDIV2;
		mAspect = 640.0f / 480.0f;
		mNearZ = 0.001f;
		mFarZ = 1000.0f;

		recalc();
	}

	void recalc() {
		mView.calc_view(mPos, mTgt, mUp);
		mView.calc_proj(mFovY, mAspect, mNearZ, mFarZ);
		mView.calc_viewProj();
	}
};



struct sGlobals {
	GlobalSingleton<cSDLWindow> win;
	GlobalSingleton<cGfx> gfx;
	GlobalSingleton<cShaderStorage> shaderStorage;
};

sGlobals globals;

cGfx& get_gfx() { return globals.gfx.get(); }
cShaderStorage& get_shader_storage() { return globals.shaderStorage.get(); }


class cConstBufferBase : noncopyable {
	moveable_ptr<ID3D11Buffer> mpBuf;
public:
	cConstBufferBase() = default;
	~cConstBufferBase() { deinit(); }
	cConstBufferBase(cConstBufferBase&& o) : mpBuf(std::move(o.mpBuf)) {}
	cConstBufferBase& operator=(cConstBufferBase&& o) {
		mpBuf = std::move(o.mpBuf);
		return *this;
	}

	void deinit() {
		if (mpBuf) {
			mpBuf->Release();
			mpBuf = nullptr;
		}
	}

	void set_VS(ID3D11DeviceContext* pCtx, UINT slot) {
		ID3D11Buffer* bufs[1] = { mpBuf };
		pCtx->VSSetConstantBuffers(slot, 1, bufs);
	}

	void set_PS(ID3D11DeviceContext* pCtx, UINT slot) {
		ID3D11Buffer* bufs[1] = { mpBuf };
		pCtx->PSSetConstantBuffers(slot, 1, bufs);
	}
protected:
	void init(ID3D11Device* pDev, size_t size) {
		auto desc = D3D11_BUFFER_DESC();
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.ByteWidth = (UINT)size;

		HRESULT hr = pDev->CreateBuffer(&desc, nullptr, mpBuf.pp());
		if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateBuffer const buffer failed");
	}

	void update(ID3D11DeviceContext* pCtx, void const* pData, size_t size) {
		D3D11_MAPPED_SUBRESOURCE mr;
		HRESULT hr = pCtx->Map(mpBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mr);
		if (SUCCEEDED(hr)) {
			::memcpy(mr.pData, pData, size);
			pCtx->Unmap(mpBuf, 0);
		}
	}
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


struct sTestCBuf {
	XMFLOAT4 diffClr;
};

struct sCameraCBuf {
	dx::XMMATRIX viewProj;
	dx::XMMATRIX view;
	dx::XMMATRIX proj;
};

cConstBuffer<sCameraCBuf> camCBuf;
cCamera cam;


class cGnomon {
	struct sVtx {
		float mPos[3];
		float mClr[4];
	};

	struct sMeshCBuf {
		dx::XMMATRIX wmtx;
	};

	cShader* mpVS = nullptr;
	cShader* mpPS = nullptr;
	ID3D11InputLayout* mpIL = nullptr;
	ID3D11Buffer* mpVtxBuf = nullptr;
	cConstBuffer<sMeshCBuf> mConstBuf;
	int mState = 0;
public:

	void exec() {
		switch (mState) {
		case 0:
			state_init();
			mState++;
			break;
		case 2:
			state_deinit();
			break;
		}
	}
	void disp() {
		auto pCtx = get_gfx().get_ctx();

		//mConstBuf.mData.wmtx = dx::XMMatrixIdentity();
		mConstBuf.mData.wmtx = dx::XMMatrixTranslation(0, 0, 0);
		mConstBuf.update(pCtx);
		mConstBuf.set_VS(pCtx, 0);

		camCBuf.set_VS(pCtx, 1);

		UINT pStride[] = { sizeof(sVtx) };
		UINT pOffset[] = { 0 };
		pCtx->IASetVertexBuffers(0, 1, &mpVtxBuf, pStride, pOffset);
		pCtx->IASetInputLayout(mpIL);
		pCtx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		pCtx->VSSetShader(mpVS->asVS(), nullptr, 0);
		pCtx->PSSetShader(mpPS->asPS(), nullptr, 0);
		pCtx->Draw(6, 0);
	}

	void deinit() {
		mState = 2;
		exec();
	}
private:
	void state_init() {
		auto& ss = get_shader_storage();
		mpVS = ss.load_VS("simple.vs.cso");
		mpPS = ss.load_PS("simple.ps.cso");

		D3D11_INPUT_ELEMENT_DESC vdsc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(sVtx, mPos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(sVtx, mClr), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		auto pDev = get_gfx().get_dev();
		auto& code = mpVS->get_code();
		HRESULT hr = pDev->CreateInputLayout(vdsc, SIZEOF_ARRAY(vdsc), code.get_code(), code.get_size(), &mpIL);
		if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateInputLayout failed");

		sVtx vtx[6] = {
			{ 0.0f, 0.0f, 0.0f, 1.0, 0.0f, 0.0f, 1.0f },
			{ 1.0f, 0.0f, 0.0f, 1.0, 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0, 1.0f, 0.0f, 1.0f },
			{ 0.0f, 1.0f, 0.0f, 0.0, 1.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0, 0.0f, 1.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f, 0.0, 0.0f, 1.0f, 1.0f },
		};

		auto desc = D3D11_BUFFER_DESC();
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.ByteWidth = SIZEOF_ARRAY(vtx) * sizeof(vtx[0]);
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = vtx;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;
		hr = pDev->CreateBuffer(&desc, &initData, &mpVtxBuf);
		if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateBuffer vtx failed");

		mConstBuf.init(pDev);
	}

	void state_deinit() {
		if (mpVtxBuf) mpVtxBuf->Release();
		if (mpIL) mpIL->Release();
	}
};



class cObj {
	struct sVtx {
		float mPos[3];
		float mClr[4];
	};

	struct sMeshCBuf {
		dx::XMMATRIX wmtx;
	};

	cShader* mpVS = nullptr;
	cShader* mpPS = nullptr;
	ID3D11InputLayout* mpIL = nullptr;
	ID3D11Buffer* mpVtxBuf = nullptr;
	ID3D11Buffer* mpIdxBuf = nullptr;
	cConstBuffer<sMeshCBuf> mConstBuf;
	int mState = 0;
public:

	void exec() {
		switch (mState) {
		case 0:
			state_init();
			mState++;
			break;
		case 2:
			state_deinit();
			break;
		}
	}
	void disp() {
		auto pCtx = get_gfx().get_ctx();

		//mConstBuf.mData.wmtx = dx::XMMatrixIdentity();
		mConstBuf.mData.wmtx = dx::XMMatrixTranslation(0, 0, 1);
		mConstBuf.update(pCtx);
		mConstBuf.set_VS(pCtx, 0);

		camCBuf.set_VS(pCtx, 1);

		UINT pStride[] = { sizeof(sVtx) };
		UINT pOffset[] = { 0 };
		pCtx->IASetIndexBuffer(mpIdxBuf, DXGI_FORMAT_R16_UINT, 0);
		pCtx->IASetVertexBuffers(0, 1, &mpVtxBuf, pStride, pOffset);
		pCtx->IASetInputLayout(mpIL);
		pCtx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pCtx->VSSetShader(mpVS->asVS(), nullptr, 0);
		pCtx->PSSetShader(mpPS->asPS(), nullptr, 0);
		//pCtx->Draw(3, 0);
		pCtx->DrawIndexed(3, 0, 0);
	}

	void deinit() {
		mState = 2;
		exec();
	}
private:
	void state_init() {
		auto& ss = get_shader_storage();
		mpVS = ss.load_VS("simple.vs.cso");
		mpPS = ss.load_PS("simple.ps.cso");

		D3D11_INPUT_ELEMENT_DESC vdsc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(sVtx, mPos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(sVtx, mClr), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		auto pDev = get_gfx().get_dev();
		auto& code = mpVS->get_code();
		HRESULT hr = pDev->CreateInputLayout(vdsc, SIZEOF_ARRAY(vdsc), code.get_code(), code.get_size(), &mpIL);
		if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateInputLayout failed");

		sVtx vtx[3] = {
			{ 0.0f, 0.5f, 0.0f,     1.0, 0.0f, 0.0f, 1.0f },
			{ 0.5f, -0.5f, 0.0f,    0.0, 1.0f, 0.0f, 1.0f },
			{ -0.5f, -0.5f, 0.0f,   0.0, 0.0f, 1.0f, 1.0f }
		};
		uint16_t idx[3] = { 0, 1, 2 };

		auto desc = D3D11_BUFFER_DESC();
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.ByteWidth = SIZEOF_ARRAY(vtx) * sizeof(vtx[0]);
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = vtx;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;
		hr = pDev->CreateBuffer(&desc, &initData, &mpVtxBuf);
		if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateBuffer vtx failed");

		desc = D3D11_BUFFER_DESC();
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.ByteWidth = SIZEOF_ARRAY(idx) * sizeof(uint16_t);
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		initData.pSysMem = idx;
		initData.SysMemPitch = 0;
		initData.SysMemSlicePitch = 0;
		hr = pDev->CreateBuffer(&desc, &initData, &mpIdxBuf);
		if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateBuffer idx failed");

		mConstBuf.init(pDev);
	}

	void state_deinit() {
		if (mpIdxBuf) mpIdxBuf->Release();
		if (mpVtxBuf) mpVtxBuf->Release();
		if (mpIL) mpIL->Release();
	}
};

cObj obj;
cGnomon gnomon;

float camRot = DEG2RAD(0.01f);

void do_frame() {
	auto& gfx = get_gfx();
	gfx.begin_frame();

	dx::XMMATRIX m = dx::XMMatrixRotationY(camRot);
	dx::XMVECTOR cp = cam.mPos;
	//dx::XMVECTOR cp = dx::XMVectorSet(0, 0, 3, 1);
	cam.mPos = dx::XMVector4Transform(cp, m);
	cam.recalc();

	camCBuf.mData.viewProj = cam.mView.mViewProj;
	camCBuf.mData.view = cam.mView.mView;
	camCBuf.mData.proj = cam.mView.mProj;
	camCBuf.update(gfx.get_ctx());

	obj.exec();
	obj.disp();

	gnomon.exec();
	gnomon.disp();

	gfx.end_frame();
}

void loop() {
	bool quit = false;
	SDL_Event ev;
	while (!quit) {
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				quit = true;
				break;
			}
		}

		if (!quit) {
			do_frame();
		} else {
			obj.deinit();
			gnomon.deinit();
		}
	}
}

int main(int argc, char* argv[]) {
	cSDLInit sdl;
	auto win = globals.win.ctor_scoped("TestBed", 640, 480, 0);
	auto gfx = globals.gfx.ctor_scoped(globals.win.get().get_handle());
	auto ss = globals.shaderStorage.ctor_scoped();

	camCBuf.init(get_gfx().get_dev());
	cam.init();

	loop();

	return 0;
}
