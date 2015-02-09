#include <bitset>

struct SDL_MouseMotionEvent;
struct SDL_MouseButtonEvent;

class cInputMgr {
public:
	enum eMouseBtn {
		EMBLEFT,
		EMBRIGHT,
		EMBMIDDLE,

		EMBLAST
	};

	vec2i mMousePos = { 0, 0 };
	vec2i mMousePosPrev = { 0, 0 };
	vec2i mMousePosStart[EMBLAST];

	std::bitset<EMBLAST> mMouseBtn;
	std::bitset<EMBLAST> mMouseBtnPrev;
public:
		
	void on_mouse_button(SDL_MouseButtonEvent const& ev);
	void on_mouse_motion(SDL_MouseMotionEvent const& ev);

	void preupdate();
	void update();

	bool mbtn_pressed(eMouseBtn btn) const { return mMouseBtn[btn] && !mMouseBtnPrev[btn]; }
	bool mbtn_released(eMouseBtn btn) const { return !mMouseBtn[btn] && mMouseBtnPrev[btn]; }
	bool mbtn_holded(eMouseBtn btn) const { return mMouseBtn[btn] && mMouseBtnPrev[btn]; }
	bool mbtn_state(eMouseBtn btn) const { return mMouseBtn[btn]; }
	bool mbtn_state_prev(eMouseBtn btn) const { mMouseBtnPrev[btn]; }
};

cInputMgr& get_input_mgr();