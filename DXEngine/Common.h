#pragma once
#include <Windows.h>

namespace DE {
	struct WindowInfo {
		WindowInfo(HWND hwnd, int width, int height) : Hwnd(hwnd), Width(width), Height(height) {}
		HWND Hwnd;
		int Width;
		int Height;
		Microsoft::WRL::ComPtr<ID3D11Device> Device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context;
	};
}