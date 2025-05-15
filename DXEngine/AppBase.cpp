#include "pch.h"
#include "AppBase.h"
#include "WindowUtils.h"
#include "RenderBase.h"

namespace DE {
	AppBase::AppBase()
		: m_window(0, 1080, 720), m_renderer(std::make_unique<RenderBase>()) {
	}

	AppBase::~AppBase() { WindowUtils::Destroy(m_window.Hwnd); }

	bool AppBase::Initialize() {
		if (!InitWindow())
			return false;

		if (!m_renderer->Initialize(m_window))
			return false;

		if (!InitGUI())
			return false;

		// 콘솔창이 렌더링 창을 덮는 것을 방지
		::SetForegroundWindow(m_window.Hwnd);

		return true;
	}

	int AppBase::Run() { 
		while (WindowUtils::Tick()) {
			Update();
			Render();
		}
		
		return 0; 
	}

	void AppBase::Update() {}

	void AppBase::Render() {}

	bool AppBase::InitWindow() {
		m_window.Hwnd = WindowUtils::Create(L"DXEngine", m_window.Width, m_window.Height, [](HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
			AppBase* app = reinterpret_cast<AppBase*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if (app) {
				return app->MsgProc(hwnd, msg, wParam, lParam);
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
			});

		if (!m_window.Hwnd) {
			std::cout << "CreateWindow() failed." << std::endl;
			return false;
		}

		SetWindowLongPtr(m_window.Hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		WindowUtils::Show(m_window.Hwnd, SW_SHOWDEFAULT);

		return true;
	}

	bool AppBase::InitGUI() { 
		return true; 
	}

	LRESULT AppBase::MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
		case WM_SIZE:
			break;
		case WM_CLOSE:
			WindowUtils::Destroy(hwnd);
			return 0;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) // esc 버튼
				WindowUtils::Destroy(hwnd);
			return 0;
		}

		return ::DefWindowProc(hwnd, msg, wParam, lParam);
	}

} // namespace DE