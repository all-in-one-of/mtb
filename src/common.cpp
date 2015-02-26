#include "common.hpp"

#include <cstdarg>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

void dbg_msg1(cstr format) {
	::OutputDebugStringA(format);
}

void dbg_msg(cstr format, ...) {
	 char msg[1024];
	 va_list va;
	 va_start(va, format);
	 ::vsprintf_s(msg, format, va);
	 va_end(va);
	 ::OutputDebugStringA(msg);
}



