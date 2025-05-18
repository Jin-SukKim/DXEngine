#pragma once

namespace DE {
	class RenderBase;
	class GuiBase;
	class Scene;
	class AppBase
	{
	public:
		AppBase();
		~AppBase();

		bool Initialize();
		int Run();

	private:
		void update();
		void render();

		bool initWindow();
		float getAspectRatio() { return float(m_window.width) / m_window.height; }

		LRESULT CALLBACK MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam);

	private:
		WindowInfo m_window;

	protected:
		std::unique_ptr<RenderBase> m_renderer;
		std::unique_ptr<GuiBase> m_gui;
		std::unique_ptr<Scene> m_scene;
	};
}
