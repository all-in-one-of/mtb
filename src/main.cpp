#define NOMINMAX

#include <SDL.h>
#include <SDL_main.h>
#include <SDL_syswm.h>

#include "common.hpp"
#include "math.hpp"
#include "gfx.hpp"
#include "rdr.hpp"
#include "texture.hpp"
#include "model.hpp"
#include "rig.hpp"
#include "anim.hpp"
#include "input.hpp"
#include "camera.hpp"
#include "imgui_impl.hpp"
#include "sh.hpp"
#include <imgui.h>

namespace dx = DirectX;
using dx::XMFLOAT4;

#include <assimp/Importer.hpp>
#include "assimp_loader.hpp"

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
	GlobalSingleton<cTextureStorage> textureStorage;
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
cShaderStorage& cShaderStorage::get() { return globals.shaderStorage.get(); }
cInputMgr& get_input_mgr() { return globals.input.get(); }
cCamera& get_camera() { return globals.camera.get(); }
vec2i get_window_size() { return globals.win.get().get_window_size(); }
cConstBufStorage& cConstBufStorage::get() { return globals.cbufStorage.get(); }
cSamplerStates& cSamplerStates::get() { return globals.samplerStates.get(); }
cBlendStates& cBlendStates::get() { return globals.blendStates.get(); }
cRasterizerStates& cRasterizerStates::get() { return globals.rasterizeStates.get(); }
cDepthStencilStates& cDepthStencilStates::get() { return globals.depthStates.get(); }
cImgui& cImgui::get() { return globals.imgui.get(); }
cTextureStorage& cTextureStorage::get() { return globals.textureStorage.get(); }

class cGnomon {
	struct sVtx {
		float mPos[3];
		float mClr[4];
	};

	cShader* mpVS = nullptr;
	cShader* mpPS = nullptr;
	com_ptr<ID3D11InputLayout> mpIL;
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
		auto& ss = cShaderStorage::get();
		mpVS = ss.load_VS("simple.vs.cso");
		mpPS = ss.load_PS("simple.ps.cso");

		D3D11_INPUT_ELEMENT_DESC vdsc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(sVtx, mPos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(sVtx, mClr), D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		auto pDev = get_gfx().get_dev();
		auto& code = mpVS->get_code();
		HRESULT hr = pDev->CreateInputLayout(vdsc, LENGTHOF_ARRAY(vdsc), code.get_code(), code.get_size(), mpIL.pp());
		if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateInputLayout failed");

		sVtx vtx[6] = {
			{ 0.0f, 0.0f, 0.0f, 1.0, 0.0f, 0.0f, 1.0f },
			{ 1.0f, 0.0f, 0.0f, 1.0, 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0, 1.0f, 0.0f, 1.0f },
			{ 0.0f, 1.0f, 0.0f, 0.0, 1.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0, 0.0f, 1.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f, 0.0, 0.0f, 1.0f, 1.0f },
		};

		mVtxBuf.init(pDev, vtx, LENGTHOF_ARRAY(vtx), sizeof(vtx[0]));
	}

	void state_deinit() {
		mVtxBuf.deinit();
		mpIL.reset();
	}
};


class cLightning {
	cModel mModel;
	cModelData mMdlData;
	cModelMaterial mMtl;
public:

	bool init() {
		//mdlData.load("../data/jill.obj");
		bool res = true;
		res = res && mMdlData.load("../data/lightning.geo");
		res = res && mMtl.load(get_gfx().get_dev(), mMdlData, "../data/lightning.mtl");
		res = res && mModel.init(mMdlData, mMtl);

		//mModel.mWmtx = DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f);

		return res;
	}

	void deinit() {
		mModel.deinit();
		mMdlData.unload();
	}

	void disp() {
		mModel.dbg_ui();
		mModel.disp();
	}
};
class cSphere {
	cModel mModel;
	cModelData mMdlData;
	cModelMaterial mMtl;
public:

	bool init() {
		bool res = true;
		res = res && mMdlData.load("../data/sphere.obj");
		res = res && mMtl.load(get_gfx().get_dev(), mMdlData, nullptr);
		res = res && mModel.init(mMdlData, mMtl);

		//mModel.mWmtx = DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f);

		return res;
	}

	void deinit() {
		mModel.deinit();
		mMdlData.unload();
	}

	void disp() {
		mModel.dbg_ui();
		mModel.disp();
	}
};

class cOwl {
	cModel mModel;
	cModelData mMdlData;
	cModelMaterial mMtl;
	cRigData mRigData;
	cRig mRig;

	cAnimationDataList mAnimDataList;
	cAnimationList mAnimList;

	float mFrame = 0.0f;
	float mSpeed = 1.0f;
	int mCurAnim = 0;
public:

	bool init() {
		bool res = true;

//#define OBJPATH "../data/owl/"
#define OBJPATH "../data/succub/"

		res = res && mMdlData.load(OBJPATH "def.geo");
		res = res && mMtl.load(get_gfx().get_dev(), mMdlData, OBJPATH "def.mtl");
		res = res && mModel.init(mMdlData, mMtl);

		mRigData.load(OBJPATH "def.rig");
		mRig.init(&mRigData);

		mAnimDataList.load(OBJPATH, "def.alist");
		mAnimList.init(mAnimDataList, mRigData);

#undef OBJPATH

		float scl = 0.01f;
		mModel.mWmtx = DirectX::XMMatrixScaling(scl, scl, scl);

		auto pRootJnt = mRig.get_joint(0);
		if (pRootJnt) {
			pRootJnt->set_parent_mtx(&mModel.mWmtx);
		}

		return res;
	}

	void deinit() {
		mModel.deinit();
		mMdlData.unload();
	}

	void disp() {
		int32_t animCount = mAnimList.get_count();
		auto& anim = mAnimList[mCurAnim];
		float lastFrame = anim.get_last_frame();

		anim.eval(mRig, mFrame);
		mFrame += mSpeed;
		if (mFrame > lastFrame)
			mFrame = 0.0f;

		ImGui::Begin("anim");
		ImGui::LabelText("name", "%s", anim.get_name());
		ImGui::SliderInt("curAnim", &mCurAnim, 0, animCount - 1);
		ImGui::SliderFloat("frame", &mFrame, 0.0f, lastFrame);
		ImGui::SliderFloat("speed", &mSpeed, 0.0f, 3.0f);
		ImGui::End();


		mRig.calc_local();
		mRig.calc_world();
		mRig.upload_skin(get_gfx().get_ctx());

		mModel.dbg_ui();
		mModel.disp();
	}
};


class cUnrealPuppet {
	cModel mModel;
	cModelData mMdlData;
	cModelMaterial mMtl;
	cRigData mRigData;
	cRig mRig;

	cAnimationDataList mAnimDataList;
	cAnimationList mAnimList;

	float mFrame = 0.0f;
	float mSpeed = 1.0f / 60.0f;
	int mCurAnim = 0;
public:

	bool init() {
		bool res = true;

#define OBJPATH "../data/unreal_puppet/"
		{
			cAssimpLoader loader;
			res = res && loader.load_unreal_fbx(OBJPATH "SideScrollerSkeletalMesh.FBX");
			res = res && mMdlData.load_assimp(loader);
			res = res && mMtl.load(get_gfx().get_dev(), mMdlData, OBJPATH "def.mtl");
			res = res && mModel.init(mMdlData, mMtl);

			mRigData.load(loader);
			mRig.init(&mRigData);
		}
		{
			cAssimpLoader animLoader;
			//animLoader.load_unreal_fbx(OBJPATH "SideScrollerIdle.FBX");
			animLoader.load_unreal_fbx(OBJPATH "SideScrollerWalk.FBX");
			mAnimDataList.load(animLoader);
			mAnimList.init(mAnimDataList, mRigData);
		}
#undef OBJPATH

		float scl = 0.01f;
		mModel.mWmtx = DirectX::XMMatrixScaling(scl, scl, scl);
		mModel.mWmtx *= DirectX::XMMatrixRotationX(DEG2RAD(-90.0f)); 

		auto pRootJnt = mRig.get_joint(0);
		if (pRootJnt) {
			pRootJnt->set_parent_mtx(&mModel.mWmtx);
		}

		return res;
	}

	void deinit() {
		mModel.deinit();
		mMdlData.unload();
	}

	void disp() {

		int32_t animCount = mAnimList.get_count();
		auto& anim = mAnimList[mCurAnim];
		float lastFrame = anim.get_last_frame();

		anim.eval(mRig, mFrame);
		mFrame += mSpeed;
		if (mFrame > lastFrame)
			mFrame = 0.0f;

		ImGui::Begin("anim");
		ImGui::LabelText("name", "%s", anim.get_name());
		ImGui::SliderInt("curAnim", &mCurAnim, 0, animCount - 1);
		ImGui::SliderFloat("frame", &mFrame, 0.0f, lastFrame);
		ImGui::SliderFloat("speed", &mSpeed, 0.0f, 3.0f);
		ImGui::End();

		mRig.calc_local();
		mRig.calc_world();
		mRig.upload_skin(get_gfx().get_ctx());

		mModel.dbg_ui();
		mModel.disp();
	}
};

cGnomon gnomon;
cLightning lightning;
cSphere sphere;
cOwl owl;
cUnrealPuppet upuppet;

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

	//lightning.disp();
	//sphere.disp();
	//owl.disp();
	upuppet.disp();

	gnomon.exec();
	gnomon.disp();

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
			case SDL_TEXTINPUT:
				inputMgr.on_text_input(ev.text);
				break;
			}
		}

		inputMgr.update();

		if (!quit) {
			do_frame();
		} else {
			gnomon.deinit();
		}

		Uint32 now = SDL_GetTicks();
		Uint32 spent = now - ticks;
		if (spent < 16) {
			SDL_Delay(16 - spent);
		}
	}
}




// See "Stupid Spherical Harmonics (SH) Tricks" by Peter-Pike Sloan, Appendix A10
void pack_sh_param(sSHCoef::sSHChan const& chan, DirectX::XMVECTOR sh[7], int idx) {
	static const float spi = (float)sqrt(M_PI);
	const float C0 = 1.0f / (2.0f * spi);
	const float C1 = sqrtf(3.0f) / (3.0f * spi);
	const float C2 = sqrtf(15.0f) / (8.0f  * spi);
	const float C3 = sqrtf(5.0f) / (16.0f * spi);
	const float C4 = 0.5f * C2;

	dx::XMVECTOR mul1 = dx::XMVectorSet(-C1, -C1, C1, C0);
	dx::XMVECTOR mul2 = dx::XMVectorSet(C2, -C2, 3.0f * C3, -C2);

	dx::XMVECTOR v = dx::XMVectorSet(chan[3], chan[1], chan[2], chan[0]);
	v = dx::XMVectorMultiply(v, mul1);
	v.m128_f32[3] -= C3 * chan[6];
	sh[idx] = v;

	v = dx::XMVectorSet(chan[4], chan[5], chan[6], chan[7]);
	v = dx::XMVectorMultiply(v, mul2);
	sh[idx + 3] = v;

	sh[6].m128_f32[idx] = C4*chan[8];
}

void pack_sh_param(sSHCoef const& coef, DirectX::XMVECTOR sh[7]) {
	pack_sh_param(coef.mR, sh, 0);
	pack_sh_param(coef.mG, sh, 1);
	pack_sh_param(coef.mB, sh, 2);
	sh[6].m128_f32[3] = 1.0f;
}


int main(int argc, char* argv[]) {
	cSDLInit sdl;
	auto win = globals.win.ctor_scoped("TestBed", 1200, 900, 0);
	auto input = globals.input.ctor_scoped();
	auto gfx = globals.gfx.ctor_scoped(globals.win.get().get_handle());
	auto ss = globals.shaderStorage.ctor_scoped();
	auto ts = globals.textureStorage.ctor_scoped(get_gfx().get_dev());
	auto cbuf = globals.cbufStorage.ctor_scoped(get_gfx().get_dev());
	auto smps = globals.samplerStates.ctor_scoped(get_gfx().get_dev());
	auto blnds = globals.blendStates.ctor_scoped(get_gfx().get_dev());
	auto rsst = globals.rasterizeStates.ctor_scoped(get_gfx().get_dev());
	auto dpts = globals.depthStates.ctor_scoped(get_gfx().get_dev());
	auto imgui = globals.imgui.ctor_scoped(get_gfx());
	auto cam = globals.camera.ctor_scoped();

	trackballCam.init(get_camera());

	//lightning.init();
	//sphere.init();
	//owl.init();
	upuppet.init();

	auto& l = cConstBufStorage::get().mLightCBuf; 
	::memset(&l.mData, 0, sizeof(l.mData));
	
	l.mData.pos[0] = dx::XMVectorSet(0, 0, 1, 1);
	l.mData.clr[0] = dx::XMVectorSet(1, 1, 1, 1);
	l.mData.clr[0] = dx::XMVectorScale(l.mData.clr[0], 1.0f);
	l.mData.isEnabled[0] = false;
	
	l.mData.pos[1] = dx::XMVectorSet(0, 1, -1, 1);
	l.mData.clr[1] = dx::XMVectorSet(1, 1, 1, 1);
	l.mData.isEnabled[1] = true;

	l.mData.pos[2] = dx::XMVectorSet(0, 3, 0, 1);
	l.mData.clr[2] = dx::XMVectorSet(1, 1, 1, 1);
	l.mData.isEnabled[2] = true;

	//sSHCoef sh;
	//sh.mR = {{ 3.783264f, -2.887 813f, 0.3790306f, -1.033029f, 
	//	0.623285f, -0.07800784f, -0.9355605f, -0.5741177f, 
	//	0.2033477f }};
	//sh.mG = {{4.260424f, -3.586802f, 0.2952161f, -1.031689f,
	//	0.5558126f, 0.1486536f, -1.25426f, -0.5034486f,
	//	-0.0442012f}};
	//sh.mB = {{4.504587f, -4.147052f, 0.09856746f, -0.8849242f,
	//	0.3977577f, 0.47249f, -1.52563f, -0.3643064f,
	//	-0.4521801f}};

	// blossom
	//sSHCoef sh;
	//sh.mR = { { 1.4311553239822388f, -1.1200312376022339f, -0.48424786329269409f, 0.27141338586807251f, -0.2891039252281189f, 0.5796363353729248f, -0.36794754862785339f, -0.16639560461044312f, -0.78260189294815063f } };
	//sh.mG = { { 1.4695711135864258f, -1.2203395366668701f, -0.4916711151599884f, 0.28614127635955811f, -0.31375649571418762f, 0.58923381567001343f, -0.38483285903930664f, -0.1729036271572113f, -0.82542788982391357f } };
	//sh.mB = { { 1.541804313659668f, -1.4233723878860474f, -0.52123433351516724f, 0.29771363735198975f, -0.35702097415924072f, 0.60223019123077393f, -0.44343027472496033f, -0.19318416714668274f, -0.94708257913589478f } };

	// blossom flip z
	sSHCoef sh;
	sh.mR = { { 1.4311553239822388f, -1.1200312376022339f, 0.48424786329269409f, 0.27141338586807251f, -0.2891039252281189f, -0.5796363353729248f, -0.36794754862785339f, 0.16639560461044312f, -0.78260189294815063f } };
	sh.mG = { { 1.4695711135864258f, -1.2203395366668701f, 0.4916711151599884f, 0.28614127635955811f, -0.31375649571418762f, -0.58923381567001343f, -0.38483285903930664f, 0.1729036271572113f, -0.82542788982391357f } };
	sh.mB = { { 1.541804313659668f, -1.4233723878860474f, 0.52123433351516724f, 0.29771363735198975f, -0.35702097415924072f, -0.60223019123077393f, -0.44343027472496033f, 0.19318416714668274f, -0.94708257913589478f } };

	//

	// subway
	//sSHCoef sh;
	//sh.mR = { { 2.9015493392944336f, -0.97448551654815674f, -0.035802226513624191f, 0.5218314528465271f, -0.56667429208755493f, -0.15583968162536621f, 0.5977063775062561f, 0.37468424439430237f, 0.82314270734786987f } };
	//sh.mG = { { 2.691716194152832f, -0.94804292917251587f, -0.073127821087837219f, 0.52174961566925049f, -0.55379670858383179f, -0.11602098494768143f, 0.57575201988220215f, 0.34524986147880554f, 0.79096674919128418f } };
	//sh.mB = { { 2.4294383525848389f, -0.92562246322631836f, -0.11760011315345764f, 0.51144671440124512f, -0.54375362396240234f, -0.069152571260929108f, 0.54059600830078125f, 0.20570482313632965f, 0.75606906414031982f } };

	sh.scl(1.2f);

	pack_sh_param(sh, l.mData.sh);

	l.update(get_gfx().get_ctx());
	l.set_PS(get_gfx().get_ctx()); 


	auto& skinCBuf = cConstBufStorage::get().mSkinCBuf;
	skinCBuf.set_VS(get_gfx().get_ctx());

	loop();

	return 0;
}
