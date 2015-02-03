#include "math.hpp"
#include "input.hpp"
#include "common.hpp"

#include <SDL_events.h>

void cInputMgr::on_mouse_motion(SDL_MouseMotionEvent const& ev) {
	mMousePos = { ev.x, ev.y };
}

static int sdl_mouse_btn_to_input(Uint8 btn) {
	switch (btn) {
	case SDL_BUTTON_LEFT: return cInputMgr::EMBLEFT;
	case SDL_BUTTON_RIGHT: return cInputMgr::EMBRIGHT;
	case SDL_BUTTON_MIDDLE: return cInputMgr::EMBMIDDLE;
	default: return -1;
	}
}

void cInputMgr::on_mouse_button(SDL_MouseButtonEvent const& ev) {
	int btn = sdl_mouse_btn_to_input(ev.button);
	if (btn < 0) return;

	mMouseBtn[btn] = (ev.state == SDL_PRESSED);

	//dbg_msg("%d %d %d\n", ev.type, ev.state, ev.clicks);
}

void cInputMgr::update() {
	for (int i = 0; i < EMBLAST; ++i) {
		if (mbtn_pressed((eMouseBtn)i)) {
			mMousePosStart[i] = mMousePos;
		}
	}
	
	mMousePosPrev = mMousePos;
	mMouseBtnPrev = mMouseBtn;
	
	//if (mMouseBtn[EMBLEFT])
	//	dbg_msg("%d %d\n", mMousePos.x, mMousePos.y);
}