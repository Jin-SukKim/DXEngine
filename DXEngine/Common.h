#pragma once
#include <Windows.h>

namespace DE {
	struct WindowInfo {
		WindowInfo(HWND hwnd, int width, int height) : Hwnd(hwnd), Width(width), Height(height) {}
		HWND Hwnd;
		int Width;
		int Height;
	};
}