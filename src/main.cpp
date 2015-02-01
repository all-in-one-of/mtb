#include <SDL.h>
#include <SDL_main.h>
#include <SDL_syswm.h>

#include "common.hpp"
#include "gfx.hpp"

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





struct sGlobals {
	GlobalSingleton<cSDLWindow> win;
	GlobalSingleton<cGfx> gfx;
	GlobalSingleton<cShaderStorage> shaderStorage;
};

sGlobals globals;

cGfx& get_gfx() { return globals.gfx.get(); }
cShaderStorage& get_shader_storage() { return globals.shaderStorage.get(); }

class cObj {
	struct sVtx {
		float mPos[3];
	};

	cShader* mpVS = nullptr;
	cShader* mpPS = nullptr;
	ID3D11InputLayout* mpIL = nullptr;
	ID3D11Buffer* mpVtxBuf = nullptr;

	int mState = 0;
public:

	void exec() {
		switch (mState) {
		case 0:
			state_init();
			mState++;
			break;
		}
	}
	void disp() {
		auto pCtx = get_gfx().get_ctx();

		UINT pStride[] = { sizeof(sVtx) };
		UINT pOffset[] = { 0 };
		pCtx->IASetVertexBuffers(0, 1, &mpVtxBuf, pStride, pOffset);
		pCtx->IASetInputLayout(mpIL);
		pCtx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pCtx->VSSetShader(mpVS->asVS(), nullptr, 0);
		pCtx->PSSetShader(mpPS->asPS(), nullptr, 0);
		pCtx->Draw(3, 0);
	}
private:
	void state_init() {
		auto& ss = get_shader_storage();
		mpVS = ss.load_VS("simple.vs.cso");
		mpPS = ss.load_PS("simple.ps.cso");

		D3D11_INPUT_ELEMENT_DESC vdsc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(sVtx, mPos), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		auto pDev = get_gfx().get_dev();
		auto& code = mpVS->get_code();
		HRESULT hr = pDev->CreateInputLayout(vdsc, SIZEOF_ARRAY(vdsc), code.get_code(), code.get_size(), &mpIL);
		if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateInputLayout failed");

		sVtx vtx[3] = {
			{ 0.0f, 0.5f, 0.5f },
			{ 0.5f, -0.5f, 0.5f },
			{ -0.5f, -0.5f, 0.5f }
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
		if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateInputLayout failed");
	}
};

cObj obj;

void do_frame() {
	auto& gfx = get_gfx();
	gfx.begin_frame();

	obj.exec();
	obj.disp();

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
		}
	}
}

int main(int argc, char* argv[]) {
	cSDLInit sdl;
	auto win = globals.win.ctor_scoped("TestBed", 640, 480, 0);
	auto gfx = globals.gfx.ctor_scoped(globals.win.get().get_handle());
	auto ss = globals.shaderStorage.ctor_scoped();

	loop();

	return 0;
}
