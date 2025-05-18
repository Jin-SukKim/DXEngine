#include "pch.h"
#include "AppBase.h"
#include "WindowUtils.h"
#include "RenderBase.h"
#include "GuiBase.h"
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

		// �ܼ�â�� ������ â�� ���� ���� ����
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
		// GUI �Է�
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
			return true;

		// Window �Է�
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
			if (wParam == VK_ESCAPE) // esc ��ư
				WindowUtils::Destroy(hwnd);
			return 0;
		}

		return ::DefWindowProc(hwnd, msg, wParam, lParam);
	}

} // namespace DE