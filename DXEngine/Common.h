#pragma once
#include <Windows.h>

namespace DE {
	struct WindowInfo {
		WindowInfo(HWND hwnd, int width, int height) : hwnd(hwnd), width(width), height(height) {}
		HWND hwnd;
		int width;
		int height;
		Microsoft::WRL::ComPtr<ID3D11Device> device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	};
}