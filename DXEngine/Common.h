#pragma once
#include <Windows.h>

namespace DE {
	struct WindowInfo {
		WindowInfo(HWND hwnd, int width, int height) : hwnd(hwnd), width(width), height(height) {}
		HWND hwnd;
		int width;
		int height;
		//Microsoft::WRL::ComPtr<ID3D11Device> device;
		//Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	};

	// 각 오브젝트가 가지고 있는 Component의 Update 순서를 지정하기 위해 사용
	enum class ComponentType {
		Transform,
		Model,
		BoundingVolume,
		MaxComponentType
	};
}