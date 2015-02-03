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
};

sGlobals globals;

cGfx& get_gfx() { return globals.gfx.get(); }
cShaderStorage& get_shader_storage() { return globals.shaderStorage.get(); }
cInputMgr& get_input_mgr() { return globals.input.get(); }
cCamera& get_camera() { return globals.camera.get(); }
vec2i get_window_size() { return globals.win.get().get_window_size(); }

struct sTestCBuf {
	XMFLOAT4 diffClr;
};

struct sCameraCBuf {
	dx::XMMATRIX viewProj;
	dx::XMMATRIX view;
	dx::XMMATRIX proj;
};

cConstBuffer<sCameraCBuf> camCBuf;



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
	cVertexBuffer mVtxBuf;
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
		mConstBuf.init(pDev);
	}

	void state_deinit() {
		mVtxBuf.deinit();
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
	cVertexBuffer mVtxBuf;
	cIndexBuffer mIdxBuf;
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
		mIdxBuf.set(pCtx, 0);
		mVtxBuf.set(pCtx, 0, 0);
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

		mVtxBuf.init(pDev, vtx, SIZEOF_ARRAY(vtx), sizeof(vtx[0]));
		mIdxBuf.init(pDev, idx, SIZEOF_ARRAY(idx), DXGI_FORMAT_R16_UINT);
		mConstBuf.init(pDev);
	}

	void state_deinit() {
		mVtxBuf.deinit();
		mIdxBuf.deinit();
		if (mpIL) mpIL->Release();
	}
};

cObj obj;
cGnomon gnomon;
cModel model;

class cTrackballCam {
	cTrackball tb;
public:
	void init() {
	}

	void update() {
		bool updated = false;
		updated = updated || update_trackball();
		updated = updated || update_distance();
		updated = updated || update_translation();
		if (updated) {
			auto& cam = get_camera();
			cam.recalc();
		}
	}
protected:
	bool update_trackball() {
		auto& input = get_input_mgr();
		const auto btn = cInputMgr::EMBMIDDLE;
		if (!input.mbtn_state(btn)) return false;
		auto pos = input.mMousePos;
		//auto prev = input.mMousePosStart[btn];
		auto prev = input.mMousePosPrev;
		if (pos == prev) return false;

		tb.update(pos, prev);
		auto& cam = get_camera();
		tb.apply(cam);
		return true;
	}

	bool update_distance() {
		auto& input = get_input_mgr();
		const auto btn = cInputMgr::EMBRIGHT;
		if (!input.mbtn_holded(btn)) return false;

		auto pos = input.mMousePos;
		auto prev = input.mMousePosPrev;

		int dy = pos.y - prev.y;
		if (dy == 0) return false;
		float scale;
		float speed = 0.08f;
		if (dy < 0) {
			scale = 1.0f - ::log10f((float)clamp(-dy, 1, 10)) * speed;
		} else {
			scale = 1.0f + ::log10f((float)clamp(dy, 1, 10)) * speed;
		}

		auto& cam = get_camera();
		auto dir = dx::XMVectorSubtract(cam.mPos, cam.mTgt);
		dir = dx::XMVectorScale(dir, scale);
		cam.mPos = dx::XMVectorAdd(dir, cam.mTgt);

		return true;
	}

	bool update_translation() {
		auto& input = get_input_mgr();
		//const auto btn = cInputMgr::EMBMIDDLE;
		const auto btn = cInputMgr::EMBLEFT;
		if (!input.mbtn_holded(btn)) return false;

		auto pos = input.mMousePos;
		auto prev = input.mMousePosPrev;

		auto dt = pos - prev;
		if (dt == vec2i(0, 0)) return false;

		vec2f dtf = dt;
		dtf = dtf * 0.001f;

		auto& cam = get_camera();
		auto cpos = cam.mPos;
		auto ctgt = cam.mTgt;
		auto cup = cam.mUp;
		auto cdir = dx::XMVectorSubtract(cpos, ctgt);
		auto side = dx::XMVector3Cross(cdir, cup); // reverse direction

		float len = dx::XMVectorGetX(dx::XMVector3LengthEst(cdir));

		auto move = dx::XMVectorSet(dtf.x, dtf.y * len, 0, 0);
		//move = dx::XMVectorScale(move, len);

		dx::XMMATRIX b(side, cup, cdir, dx::g_XMZero);
		move = dx::XMVector3Transform(move, b);

		cam.mPos = dx::XMVectorAdd(cpos, move);
		cam.mTgt = dx::XMVectorAdd(ctgt, move);

		return true;
	}
};

cTrackballCam trackballCam;

float camRot = DEG2RAD(0.01f);

void do_frame() {
	auto& gfx = get_gfx();
	gfx.begin_frame();

	//dx::XMMATRIX m = dx::XMMatrixRotationY(camRot);
	//dx::XMVECTOR cp = cam.mPos;
	//dx::XMVECTOR cp = dx::XMVectorSet(0, 0, 3, 1);
	//cam.mPos = dx::XMVector4Transform(cp, m);
	//cam.recalc();
	auto& cam = get_camera();
	trackballCam.update();
	camCBuf.mData.viewProj = cam.mView.mViewProj;
	camCBuf.mData.view = cam.mView.mView;
	camCBuf.mData.proj = cam.mView.mProj;
	camCBuf.update(gfx.get_ctx());
	camCBuf.set_VS(gfx.get_ctx(), 1);

	//obj.exec();
	//obj.disp();

	model.disp();

	gnomon.exec();
	gnomon.disp();

	gfx.end_frame();
}

void loop() {
	bool quit = false;
	SDL_Event ev;
	auto& inputMgr = get_input_mgr();
	while (!quit) {
		inputMgr.update();

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
	auto win = globals.win.ctor_scoped("TestBed", 1200, 900, 0);
	auto input = globals.input.ctor_scoped();
	auto gfx = globals.gfx.ctor_scoped(globals.win.get().get_handle());
	auto ss = globals.shaderStorage.ctor_scoped();
	auto cam = globals.camera.ctor_scoped();

	camCBuf.init(get_gfx().get_dev());

	trackballCam.init();

	cModelData mdlData;
	mdlData.load("../data/jill.obj");

	model.init(mdlData);

	loop();

	return 0;
}
