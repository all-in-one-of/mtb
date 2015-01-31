#include <SDL.h>
#include <SDL_main.h>

#include <algorithm>
//#include <type_traits>

#include "common.hpp"

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

struct sGlobals {
	GlobalSingleton<cSDLWindow> win;
};

sGlobals globals;

int main(int argc, char* argv[]) {
	cSDLInit sdl;
	globals.win.Ctor("TestBed", 640, 480, 0);



	globals.win.Dtor();

	return 0;
}