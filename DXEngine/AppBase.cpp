#include "pch.h"
#include "AppBase.h"
#include "WindowUtils.h"
#include "Scene.h"
#include "CameraActor.h"

#include <imgui_impl_win32.h>
// imgui_impl_win32.cpp�� ���ǵ� �޽��� ó�� �Լ��� ���� ���� ����
// Vcpkg�� ���� IMGUI�� ����� ��� �����ٷ� ��� �� �� ����
//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
//	HWND hWnd,	UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

namespace DE {
	InputManager AppBase::m_inputManager;

	AppBase::AppBase()
		: m_window(0, 1920, 1080) {
	}

	AppBase::~AppBase() { WindowUtils::Destroy(m_window.hwnd); }

	bool AppBase::Initialize() {
		if (!initWindow())
			return false;
		
		if (!m_renderer.Initialize(m_window))
			return false;

		if (!m_gui.Initialize(m_window, m_renderer))
			return false;

		// �ܼ�â�� ������ â�� ���� ���� ����
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
		// ������ GUI â ũ�⿡ ���� viewport ũ�� ��ȯ
		// TODO: �� �����Ӹ��� ���� �ʰ� GUI â ũ�Ⱑ ����Ǹ� ���� �������ֱ�
		//if (m_gui->IsSizeChanged()) {
		//	float width = m_window.width - m_gui->GetSize().x;
		//	m_gui->SetPos({ width, 0.f });
		//	m_renderer->SetViewport(width, float(m_window.height));
		//	m_scene->GetMainCamera()->SetAspectRatio(width / m_window.height);
		//}
	}

	void AppBase::update() {
		m_gui.Update();
		m_inputManager.Update();
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
		// GUI �Է�
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
			return true;

		// Window �Է�
		switch (msg) {
		case WM_SIZE:
			// ȭ�� �ػ󵵰� �ٲ�� SwapChain�� �ٽ� ����
			if (m_renderer.GetSwapChain()) {
				m_window.width = int(LOWORD(lParam));
				m_window.height= int(HIWORD(lParam));

				// �����찡 Minimize ��忡���� width, height�� 0
				if (m_window.width && m_window.height) {
					//std::cout << "Resize SwapChain to " << m_window.width << " " << m_window.height << std::endl;

					m_renderer.ResizeSwapChain(m_window);

					if (m_scene && m_scene->GetMainCamera()) {
						m_scene->GetMainCamera()->SetAspectRatio(this->getAspectRatio());
					}
				}
			}
			break;
		case WM_CLOSE:
			WindowUtils::Destroy(hwnd);
			return 0;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) // esc ��ư
				WindowUtils::Destroy(hwnd);
			return 0;
		}

		return ::DefWindowProc(hwnd, msg, wParam, lParam);
	}

} // namespace DE