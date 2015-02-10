#define NOMINMAX

#include <SDL.h>
#include <SDL_main.h>
#include <SDL_syswm.h>

#include "common.hpp"
#include "math.hpp"
#include "gfx.hpp"
#include "rdr.hpp"
#include "model.hpp"
#include "input.hpp"
#include "camera.hpp"
#include "texture.hpp"
#include "imgui_impl.hpp"

#include <imgui.h>

namespace dx = DirectX;
using dx::XMFLOAT4;

#include <assimp\Importer.hpp>

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
	vec2i mWindowSize;
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

	vec2i get_window_size() const { return mWindowSize; }

private:
	void init(cstr title, int x, int y, int w, int h, Uint32 flags) {
		win = SDL_CreateWindow(title.p, x, y, w, h, flags);
		mWindowSize = { w, h };
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
	GlobalSingleton<cInputMgr> input;
	GlobalSingleton<cCamera> camera;
	GlobalSingleton<cConstBufStorage> cbufStorage;
	GlobalSingleton<cSamplerStates> samplerStates;
	GlobalSingleton<cBlendStates> blendStates;
	GlobalSingleton<cRasterizerStates> rasterizeStates;
	GlobalSingleton<cDepthStencilStates> depthStates;
	GlobalSingleton<cImgui> imgui;
};

sGlobals globals;

cGfx& get_gfx() { return globals.gfx.get(); }
cShaderStorage& get_shader_storage() { return globals.shaderStorage.get(); }
cInputMgr& get_input_mgr() { return globals.input.get(); }
cCamera& get_camera() { return globals.camera.get(); }
vec2i get_window_size() { return globals.win.get().get_window_size(); }
cConstBufStorage& cConstBufStorage::get() { return globals.cbufStorage.get(); }
cSamplerStates& cSamplerStates::get() { return globals.samplerStates.get(); }
cBlendStates& cBlendStates::get() { return globals.blendStates.get(); }
cRasterizerStates& cRasterizerStates::get() { return globals.rasterizeStates.get(); }
cDepthStencilStates& cDepthStencilStates::get() { return globals.depthStates.get(); }
cImgui& cImgui::get() { return globals.imgui.get(); }

class cGnomon {
	struct sVtx {
		float mPos[3];
		float mClr[4];
	};

	cShader* mpVS = nullptr;
	cShader* mpPS = nullptr;
	ID3D11InputLayout* mpIL = nullptr;
	cVertexBuffer mVtxBuf;
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
		auto& cbuf = cConstBufStorage::get().mMeshCBuf;
		cbuf.mData.wmtx = dx::XMMatrixTranslation(0, 0, 0);
		cbuf.update(pCtx);
		cbuf.set_VS(pCtx);

		UINT pStride[] = { sizeof(sVtx) };
		UINT pOffset[] = { 0 };
		mVtxBuf.set(pCtx, 0, 0);
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

		mVtxBuf.init(pDev, vtx, SIZEOF_ARRAY(vtx), sizeof(vtx[0]));
	}

	void state_deinit() {
		mVtxBuf.deinit();
		if (mpIL) mpIL->Release();
	}
};


class cLightning {
	cModel mModel;
	cModelData mMdlData;
	std::unique_ptr<cTexture[]> mpTextures;
public:

	void init() {
		//mdlData.load("../data/jill.obj");
		mMdlData.load("../data/lightning.obj");

		mModel.init(mMdlData);

		mModel.mpGrpMtl = std::make_unique<sGroupMaterial[]>(mMdlData.mGrpNum);
		auto* pMtls = mModel.mpGrpMtl.get();

		mpTextures = std::make_unique<cTexture[]>(mMdlData.mGrpNum);
		
		auto pDev = get_gfx().get_dev();
		char buf[512];
		for (uint32_t i = 0; i < mMdlData.mGrpNum; ++i) {
			::sprintf_s(buf, "../data/lightning_tex/%s_base.dds", mMdlData.mpGrpNames[i].c_str());
			mpTextures[i].load(pDev, buf);
			pMtls[i].mpTexBase = &mpTextures[i];
			pMtls[i].mpSmpBase = cSamplerStates::get().linear();
		}
	}

	void deinit() {
		mModel.deinit();
		mMdlData.unload();
		mpTextures.release();
	}

	void disp() {
		mModel.disp();
	}
};
class cSphere {
	cModel mModel;
	cModelData mMdlData;
public:

	void init() {
		mMdlData.load("../data/sphere.obj");

		mModel.init(mMdlData);

		mModel.mpGrpMtl = std::make_unique<sGroupMaterial[]>(mMdlData.mGrpNum);
	}

	void deinit() {
		mModel.deinit();
		mMdlData.unload();
	}

	void disp() {
		mModel.disp();
	}
};

cGnomon gnomon;
cLightning lightning;
cSphere sphere;

cTrackballCam trackballCam;

float camRot = DEG2RAD(0.01f);

void do_frame() {
	auto& gfx = get_gfx();
	gfx.begin_frame();
	cImgui::get().update();

	cRasterizerStates::get().set_def(get_gfx().get_ctx());
	cDepthStencilStates::get().set_def(get_gfx().get_ctx());

	//dx::XMMATRIX m = dx::XMMatrixRotationY(camRot);
	//dx::XMVECTOR cp = cam.mPos;
	//dx::XMVECTOR cp = dx::XMVectorSet(0, 0, 3, 1);
	//cam.mPos = dx::XMVector4Transform(cp, m);
	//cam.recalc();
	auto& cam = get_camera();
	trackballCam.update(cam);
	auto& camCBuf = cConstBufStorage::get().mCameraCBuf;
	camCBuf.mData.viewProj = cam.mView.mViewProj;
	camCBuf.mData.view = cam.mView.mView;
	camCBuf.mData.proj = cam.mView.mProj;
	camCBuf.mData.camPos = cam.mView.mPos;
	camCBuf.update(gfx.get_ctx());
	camCBuf.set_VS(gfx.get_ctx());
	camCBuf.set_PS(gfx.get_ctx());

	//obj.exec();
	//obj.disp();

	//lightning.disp();
	sphere.disp();

	gnomon.exec();
	gnomon.disp();

	ImGui::Text("Hello, world!");

	cImgui::get().disp();
	gfx.end_frame();
}

void loop() {
	bool quit = false;
	SDL_Event ev;
	auto& inputMgr = get_input_mgr();
	while (!quit) {
		Uint32 ticks = SDL_GetTicks();
		inputMgr.preupdate();

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_MOUSEMOTION:
				inputMgr.on_mouse_motion(ev.motion);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				inputMgr.on_mouse_button(ev.button);
				break;
			}
		}

		inputMgr.update();

		if (!quit) {
			do_frame();
		} else {
			//obj.deinit();
			gnomon.deinit();
		}

		Uint32 now = SDL_GetTicks();
		Uint32 spent = now - ticks;
		if (spent < 16) {
			SDL_Delay(16 - spent);
		}
	}
}


int main(int argc, char* argv[]) {
	cSDLInit sdl;
	auto win = globals.win.ctor_scoped("TestBed", 1200, 900, 0);
	auto input = globals.input.ctor_scoped();
	auto gfx = globals.gfx.ctor_scoped(globals.win.get().get_handle());
	auto ss = globals.shaderStorage.ctor_scoped();
	auto cbuf = globals.cbufStorage.ctor_scoped(get_gfx().get_dev());
	auto smps = globals.samplerStates.ctor_scoped(get_gfx().get_dev());
	auto blnds = globals.blendStates.ctor_scoped(get_gfx().get_dev());
	auto rsst = globals.rasterizeStates.ctor_scoped(get_gfx().get_dev());
	auto dpts = globals.depthStates.ctor_scoped(get_gfx().get_dev());
	auto imgui = globals.imgui.ctor_scoped(get_gfx());
	auto cam = globals.camera.ctor_scoped();

	trackballCam.init(get_camera());

	//lightning.init();
	sphere.init();

	loop();

	return 0;
}
