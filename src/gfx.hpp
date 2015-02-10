#include <d3d11.h>
#include <vector>
#include <memory>

class cSDLWindow;

class cGfx : noncopyable {
	struct sDev : noncopyable {
		com_ptr<IDXGISwapChain> mpSwapChain;
		com_ptr<ID3D11Device> mpDev;
		com_ptr<ID3D11DeviceContext> mpImmCtx;

		sDev() {}
		sDev(DXGI_SWAP_CHAIN_DESC& sd, UINT flags);
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
				mpSwapChain.reset();
			}
			mpImmCtx.reset();
			mpDev.reset();
		}
	};
	struct sRTView : noncopyable {
		com_ptr<ID3D11RenderTargetView> mpRTV;

		sRTView() {}
		sRTView(sDev& dev);
		sRTView& operator=(sRTView&& o) {
			mpRTV = std::move(o.mpRTV);
			return *this;
		}
	};

	struct sDepthStencilBuffer : noncopyable {
		com_ptr<ID3D11Texture2D> mpTex;
		com_ptr<ID3D11DepthStencilView> mpDSView;

		sDepthStencilBuffer() {}
		sDepthStencilBuffer(sDev& dev, UINT w, UINT h);
		sDepthStencilBuffer& operator=(sDepthStencilBuffer&& o) {
			mpTex = std::move(o.mpTex);
			mpDSView = std::move(o.mpDSView);
			return *this;
		}
	};

	sDev mDev;
	sRTView mRTV;
	sDepthStencilBuffer mDS;

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
	com_ptr<ID3D11DeviceChild> mpShader;
	cShaderBytecode            mCode;
public:
	cShader(ID3D11DeviceChild* pShader, cShaderBytecode&& code) : mpShader(pShader), mCode(std::move(code)) {}
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

	cShader* create_VS(cstr code);
	cShader* create_PS(cstr code);

private:
	cShader* create_VS(cShaderBytecode&& code);
	cShader* create_PS(cShaderBytecode&& code);

	cShader* put_shader(ID3D11DeviceChild* pShader, cShaderBytecode&& code);
};


cGfx& get_gfx();
cShaderStorage& get_shader_storage();
vec2i get_window_size();
