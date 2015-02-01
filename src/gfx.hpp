#include <d3d11.h>
#include <vector>
#include <memory>

class cSDLWindow;

struct sD3DException : public std::exception {
	HRESULT hr;
	sD3DException(HRESULT hr, char const* const msg) : std::exception(msg), hr(hr) {}
};

class cGfx : noncopyable {
	

	struct sDev : noncopyable {
		moveable_ptr<IDXGISwapChain> mpSwapChain;
		moveable_ptr<ID3D11Device> mpDev;
		moveable_ptr<ID3D11DeviceContext> mpImmCtx;

		sDev() {}
		sDev(DXGI_SWAP_CHAIN_DESC& sd, UINT flags) {
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
		sDev& operator=(sDev&& o) {
			release();
			mpSwapChain = std::move(o.mpSwapChain);
			mpDev = std::move(o.mpDev);
			mpImmCtx = std::move(o.mpImmCtx);
			return *this;
		}
		~sDev() {
			release();
		}
		void release() {
			if (mpSwapChain) {
				mpSwapChain->SetFullscreenState(FALSE, nullptr);
				mpSwapChain->Release();
			}
			if (mpImmCtx) mpImmCtx->Release();
			if (mpDev) mpDev->Release();
		}
	};
	struct sRTView : noncopyable {
		moveable_ptr<ID3D11RenderTargetView> mpRTV;

		sRTView() {}
		sRTView(sDev& dev) {
			ID3D11Texture2D* pBack = nullptr;
			HRESULT hr = dev.mpSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBack);
			if (!SUCCEEDED(hr))
				throw sD3DException(hr, "GetBuffer failed: back buffer");

			hr = dev.mpDev->CreateRenderTargetView(pBack, nullptr, mpRTV.pp());
			pBack->Release();

			if (!SUCCEEDED(hr))
				throw sD3DException(hr, "CreateRenderTargetView failed");
		}
		~sRTView() {
			Release();
		}
		sRTView& operator=(sRTView&& o) {
			Release();
			mpRTV = std::move(o.mpRTV);
			return *this;
		}

		void Release() {
			if (mpRTV) mpRTV->Release();
		}
	};

	struct sRSState : noncopyable {
		moveable_ptr<ID3D11RasterizerState> mpRSState;

		sRSState() {}
		sRSState(sDev& dev) {
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
		~sRSState() {
			if (mpRSState) mpRSState->Release();
		}
		sRSState(sRSState&& o) : mpRSState(std::move(o.mpRSState)) {}
		sRSState& operator=(sRSState&& o) {
			mpRSState = std::move(o.mpRSState);
			return *this;
		}
	};

	sDev mDev;
	sRTView mRTV;
	sRSState mRSState;

public:
	cGfx(HWND hwnd);

	void begin_frame();
	void end_frame();

	ID3D11Device* get_dev() { return mDev.mpDev; }
	ID3D11DeviceContext* get_ctx() { return mDev.mpImmCtx; }
};

class cShaderBytecode : noncopyable {
	std::unique_ptr<char[]> mCode;
	size_t                  mSize;
public:
	cShaderBytecode() : mCode(), mSize(0) {}
	cShaderBytecode(std::unique_ptr<char[]> code, size_t size) : mCode(std::move(code)), mSize(size) {}

	cShaderBytecode(cShaderBytecode&& o) : mCode(std::move(o.mCode)), mSize(o.mSize) {}
	cShaderBytecode& operator=(cShaderBytecode&& o) {
		mCode = std::move(o.mCode);
		mSize = o.mSize;
		return *this;
	}

	char const* get_code() const { return mCode.get(); }
	size_t get_size() const { return mSize; }
};

class cShader  : noncopyable  {
	moveable_ptr<ID3D11DeviceChild> mpShader;
	cShaderBytecode                 mCode;
public:
	cShader(ID3D11DeviceChild* pShader, cShaderBytecode&& code) : mpShader(pShader), mCode(std::move(code)) {}
	~cShader() {
		if (mpShader) mpShader->Release();
	}
	cShader(cShader&& o) : mpShader(std::move(o.mpShader)), mCode(std::move(o.mCode)) {}
	cShader& operator=(cShader&& o) {
		mpShader = std::move(o.mpShader);
		mCode = std::move(o.mCode);
		return *this;
	}

	ID3D11VertexShader* asVS() { return static_cast<ID3D11VertexShader*>(mpShader.p); }
	ID3D11PixelShader* asPS() { return static_cast<ID3D11PixelShader*>(mpShader.p); }

	cShaderBytecode const& get_code() const { return mCode; }
};


class cShaderStorage {
	std::vector<std::unique_ptr<cShader>> mShaders;

public:
	cShaderStorage() {}

	cShader* load_VS(cstr filepath);
	cShader* load_PS(cstr filepath);

private:
	cShader* put_shader(ID3D11DeviceChild* pShader, cShaderBytecode&& code);
};


cGfx& get_gfx();
