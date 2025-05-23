#include "pch.h"
#include "AppBase.h"
#include "WindowUtils.h"
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
	InputManager AppBase::m_inputManager;

	AppBase::AppBase()
		//: m_window(0, 1920, 1080) {
		: m_window(0, 1280, 960) {
	}

	AppBase::~AppBase() { WindowUtils::Destroy(m_window.hwnd); }

	bool AppBase::Initialize() {
		if (!initWindow())
			return false;
		
		if (!m_renderer.Initialize(m_window))
			return false;

		if (!m_gui.Initialize(m_window, m_renderer))
			return false;

		// 콘솔창이 렌더링 창을 덮는 것을 방지
		::SetForegroundWindow(m_window.hwnd);

		m_scene = std::make_unique<Scene>(m_renderer.GetDevice(), m_renderer.GetContext());
		float aspect = float(m_window.width) / m_window.height;
		m_scene->GetMainCamera()->SetAspectRatio(this->getAspectRatio());
		m_scene->Initialize();
		
		return true;
	}

	int AppBase::Run() { 
		while (WindowUtils::Tick()) {
			preUpdtea();
			update();
			m_gui.PostUpdate();

			render();
		}
		
		return 0; 
	}

	void AppBase::preUpdtea()
	{
		m_gui.PreUpdate();
		// 오른쪽 GUI 창 크기에 맞춰 viewport 크기 변환
		// TODO: 매 프레임마다 하지 않고 GUI 창 크기가 변경되면 설정 변경해주기
		//if (m_gui->IsSizeChanged()) {
		//	float width = m_window.width - m_gui->GetSize().x;
		//	m_gui->SetPos({ width, 0.f });
		//	m_renderer->SetViewport(width, float(m_window.height));
		//	m_scene->GetMainCamera()->SetAspectRatio(width / m_window.height);
		//}
	}

	void AppBase::update() {
		m_gui.Update();
		m_inputManager.Update(m_mouseX, m_mouseY, true);
		m_renderer.Update();
		m_scene->Update(GetDeltaTime());
	}

	void AppBase::render() {
		m_renderer.Render();
		m_scene->Render();
		m_gui.Render();

		m_renderer.Present();
	}

	float AppBase::GetDeltaTime() {
		return GuiBase::GetDeltaTime();
	}

	InputManager& AppBase::GetInputManager()
	{
		return m_inputManager;
	}

	bool AppBase::initWindow() {
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
			// 화면 해상도가 바뀌면 SwapChain을 다시 생성
			if (m_renderer.GetSwapChain()) {
				m_window.width = int(LOWORD(lParam));
				m_window.height= int(HIWORD(lParam));

				// 윈도우가 Minimize 모드에서는 width, height이 0
				if (m_window.width && m_window.height) {
					//std::cout << "Resize SwapChain to " << m_window.width << " " << m_window.height << std::endl;

					m_renderer.ResizeSwapChain(m_window);

					if (m_scene && m_scene->GetMainCamera()) {
						m_scene->GetMainCamera()->SetAspectRatio(this->getAspectRatio());
					}
				}
			}
			break;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
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
		case WM_MOUSEMOVE:
			m_mouseX = float(LOWORD(lParam));
			m_mouseY = float(HIWORD(lParam));

			// 마우스 커서의 위치를 NDC로 변환
			// 마우스 커서는 좌측 상단 (0, 0), 우측 하단(width-1, height-1)
			//		이때 내려갈수록 y값이 커지므로 - 를 곱해서 방향 반전
			// NDC는 좌측 하단이 (-1, -1), 우측 상단(1, 1)
			m_mouseX = m_mouseX * 2.0f / m_window.width - 1.0f;
			m_mouseY = -m_mouseY * 2.0f / m_window.height + 1.0f;
		}

		return ::DefWindowProc(hwnd, msg, wParam, lParam);
	}

} // namespace DE