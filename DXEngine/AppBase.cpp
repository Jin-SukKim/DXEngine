#include "pch.h"
#include "AppBase.h"
#include "WindowUtils.h"
#include "RenderBase.h"
#include "GuiBase.h"
#include "Scene.h"
#include "CameraActor.h"

#include <imgui_impl_win32.h>
// imgui_impl_win32.cpp에 정의된 메시지 처리 함수에 대한 전방 선언
// Vcpkg를 통해 IMGUI를 사용할 경우 빨간줄로 경고가 뜰 수 있음
//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
//	HWND hWnd,	UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);


namespace DE {
	AppBase::AppBase()
		: m_window(0, 1080, 720), m_renderer(std::make_unique<RenderBase>()), m_gui(std::make_unique<GuiBase>()) {
	}

	AppBase::~AppBase() { WindowUtils::Destroy(m_window.hwnd); }

	bool AppBase::Initialize() {
		if (!InitWindow())
			return false;

		if (!m_renderer->Initialize(m_window))
			return false;

		if (!m_gui->Initialize(m_window, m_renderer.get()))
			return false;

		// 콘솔창이 렌더링 창을 덮는 것을 방지
		::SetForegroundWindow(m_window.hwnd);

		m_scene = std::make_unique<Scene>(m_renderer->GetDevice(), m_renderer->GetContext());
		float aspect = float(m_window.width) / m_window.height;
		m_scene->GetMainCamera()->SetAspectRatio(float(m_window.width) / m_window.height);
		m_scene->Initialize();
		
		return true;
	}

	int AppBase::Run() { 
		while (WindowUtils::Tick()) {
			m_gui->PreUpdate();
			Update();
			m_gui->PostUpdate();

			Render();
		}
		
		return 0; 
	}

	void AppBase::Update() {
		m_gui->Update();
		m_renderer->Update();
		m_scene->Update(m_gui->GetDeltaTime());
	}

	void AppBase::Render() {
		m_renderer->Render();
		m_scene->Render();
		m_gui->Render();

		m_renderer->Present();
	}

	bool AppBase::InitWindow() {
		m_window.hwnd = WindowUtils::Create(L"DXEngine", m_window.width, m_window.height, [](HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
			AppBase* app = reinterpret_cast<AppBase*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if (app) {
				return app->MsgProc(hwnd, msg, wParam, lParam);
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
			});

		if (!m_window.hwnd) {
			std::cout << "CreateWindow() failed." << std::endl;
			return false;
		}

		SetWindowLongPtr(m_window.hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		WindowUtils::Show(m_window.hwnd, SW_SHOWDEFAULT);

		return true;
	}

	LRESULT AppBase::MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam)
	{
		// GUI 입력
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
			return true;

		// Window 입력
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