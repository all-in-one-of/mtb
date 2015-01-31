#include <SDL.h>
#include <SDL_main.h>
#include <SDL_syswm.h>

#include "common.hpp"

#include <d3d11.h>

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
		Init(title, x, y, w, h, flags);
	}
	cSDLWindow(cstr title, int w, int h, Uint32 flags) {
		Init(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, flags);
	}
	~cSDLWindow() {
		Deinit();
	}

	cSDLWindow(cSDLWindow&& o) : win(std::move(o.win)) { }
	cSDLWindow(cSDLWindow& o) = delete;

	HWND GetHandle() const {
		HWND hwnd = 0;

		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (SDL_GetWindowWMInfo(win, &info)) {
			hwnd = info.info.win.window;
		}

		return hwnd;
	}

private:
	void Init(cstr title, int x, int y, int w, int h, Uint32 flags) {
		win = SDL_CreateWindow(title.p, x, y, w, h, flags);
	}
	void Deinit() {
		if (!win) return;

		SDL_DestroyWindow(win);
		win.reset();
	}
};



class cGfx : noncopyable {
	struct sD3DException : public std::exception {
		HRESULT hr;
		sD3DException(HRESULT hr, char const* const msg) : std::exception(msg), hr(hr) {}
	};

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
			Release();
			mpSwapChain = std::move(o.mpSwapChain);
			mpDev = std::move(o.mpDev);
			mpImmCtx = std::move(o.mpImmCtx);
			return *this;
		}
		~sDev() {
			Release();
		}
		void Release() {
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

	sDev mDev;
	sRTView mRTV;
	
public:
	cGfx(cSDLWindow const& window) {
		HRESULT hr = S_OK;
		HWND hwnd = window.GetHandle();
		
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
	}

	void BeginFrame() {
		float clrClr[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
		mDev.mpImmCtx->ClearRenderTargetView(mRTV.mpRTV, clrClr);
	}

	void EndFrame() {
		mDev.mpSwapChain->Present(0, 0);
	}
};

struct sGlobals {
	GlobalSingleton<cSDLWindow> win;
	GlobalSingleton<cGfx> gfx;
};

sGlobals globals;

void do_frame() {
	auto& gfx = globals.gfx.Get();
	gfx.BeginFrame();

	gfx.EndFrame();
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
		}
	}
}

int main(int argc, char* argv[]) {
	cSDLInit sdl;
	auto win = globals.win.CtorScoped("TestBed", 640, 480, 0);
	auto gfx = globals.gfx.CtorScoped(globals.win);

	loop();

	return 0;
}
