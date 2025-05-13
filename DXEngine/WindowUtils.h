#pragma once
#include "pch.h"

namespace WindowUtils {

	// 윈도우창 생성
	FORCEINLINE HWND Create(const std::wstring& windowName, const int& windowWidth, const int& windowHeight, WNDPROC proc) {
		// 윈도우 등록
		WNDCLASSEX wc = { sizeof(WNDCLASSEX),
						 CS_CLASSDC,
						 proc,
						 0L,
						 0L,
						 GetModuleHandle(NULL),
						 NULL,
						 NULL,
						 NULL,
						 NULL,
						 windowName.c_str(), // lpszClassName, L-string
						 NULL };

		if (!::RegisterClassEx(&wc)) {
			std::cout << "RegisterClassEx() failed." << std::endl;
			return NULL;
		}

		RECT wr = { 0, 0, windowWidth, windowHeight };
		::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false); // 타이틀바, 메뉴, 테두리 등을 제외한 크기 조정

		// 등록된 윈도우창 생성해 윈도우창의 핸들 반환
		HWND hwnd = ::CreateWindow(
			wc.lpszClassName, windowName.c_str(), WS_OVERLAPPEDWINDOW,
			// 현재 화면의 중앙에 띄우도록 위치 설정
			(GetSystemMetrics(SM_CXFULLSCREEN) - windowWidth) / 2,
			(GetSystemMetrics(SM_CYFULLSCREEN) - windowHeight) / 2,
			wr.right - wr.left, // 윈도우 가로 방향 해상도
			wr.bottom - wr.top, // 윈도우 세로 방향 해상도
			NULL,
			NULL,
			wc.hInstance,
			NULL
		);

		return hwnd;
	}

	// 생성한 윈도우창 화면에 표시하고 갱싱
	FORCEINLINE void Show(HWND hWnd, int nCmdShow) {
		::ShowWindow(hWnd, nCmdShow);
		::UpdateWindow(hWnd);
	}

	FORCEINLINE void Destroy(HWND hWnd) {
		::DestroyWindow(hWnd);
	}

	// 윈도우의 입력을 받는 Loop
	FORCEINLINE bool Tick() {
		MSG msg;
		::ZeroMemory(&msg, sizeof(msg));

		// PeekMessage: 메시지가 있다면 true, 없다면 false return
		while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				return false;

			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		return true;
	}

	FORCEINLINE LRESULT CALLBACK Proc(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_DISPLAYCHANGE:
		case WM_SIZE:
		{
			break;
		}
		case WM_CLOSE:
		{
			Destroy(hwnd);
			return 0;
		}
		case WM_DESTROY:
		{
			::PostQuitMessage(0);
			return 0;
		}
		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE) // esc 버튼
				Destroy(hwnd);
			break;
		}
		//case WM_SYSCOMMAND:
		//{
		//	if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER || wParam == SC_KEYMENU)
		//	{
		//		return 0;
		//	}
		//	break;
		//}
		}

		return ::DefWindowProc(hwnd, msg, wParam, lParam);
	}
}