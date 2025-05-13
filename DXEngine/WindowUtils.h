#pragma once
#include "pch.h"

namespace WindowUtils {

	// ������â ����
	FORCEINLINE HWND Create(const std::wstring& windowName, const int& windowWidth, const int& windowHeight, WNDPROC proc) {
		// ������ ���
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
		::AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false); // Ÿ��Ʋ��, �޴�, �׵θ� ���� ������ ũ�� ����

		// ��ϵ� ������â ������ ������â�� �ڵ� ��ȯ
		HWND hwnd = ::CreateWindow(
			wc.lpszClassName, windowName.c_str(), WS_OVERLAPPEDWINDOW,
			// ���� ȭ���� �߾ӿ� ��쵵�� ��ġ ����
			(GetSystemMetrics(SM_CXFULLSCREEN) - windowWidth) / 2,
			(GetSystemMetrics(SM_CYFULLSCREEN) - windowHeight) / 2,
			wr.right - wr.left, // ������ ���� ���� �ػ�
			wr.bottom - wr.top, // ������ ���� ���� �ػ�
			NULL,
			NULL,
			wc.hInstance,
			NULL
		);

		return hwnd;
	}

	// ������ ������â ȭ�鿡 ǥ���ϰ� ����
	FORCEINLINE void Show(HWND hWnd, int nCmdShow) {
		::ShowWindow(hWnd, nCmdShow);
		::UpdateWindow(hWnd);
	}

	FORCEINLINE void Destroy(HWND hWnd) {
		::DestroyWindow(hWnd);
	}

	// �������� �Է��� �޴� Loop
	FORCEINLINE bool Tick() {
		MSG msg;
		::ZeroMemory(&msg, sizeof(msg));

		// PeekMessage: �޽����� �ִٸ� true, ���ٸ� false return
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
			if (wParam == VK_ESCAPE) // esc ��ư
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