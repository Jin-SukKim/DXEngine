#pragma once

#include "RenderBase.h"
#include "GuiBase.h"
#include "InputManager.h"

namespace DE {
	class Scene;

	class AppBase
	{
	public:
		AppBase();
		~AppBase();

		bool Initialize();
		int Run();

		static float GetDeltaTime();
		static InputManager& GetInputManager();
	private:
		void preUpdtea();
		void update();
		void render();

		bool initWindow();
		float getAspectRatio() { return float(m_window.width) / m_window.height; }

		LRESULT CALLBACK MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam);

	private:
		WindowInfo m_window;

	protected:
		RenderBase m_renderer;
		GuiBase m_gui;
		std::unique_ptr<Scene> m_scene;
		static InputManager m_inputManager;
	};
}
