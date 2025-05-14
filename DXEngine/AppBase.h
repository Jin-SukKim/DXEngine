#pragma once

namespace DE {
	class RenderBase;
	class AppBase
	{
	public:
		AppBase();
		~AppBase();

		bool Initialize();
		int Run();

	private:
		void Update();
		void Render();

		bool InitWindow();
		bool InitGUI();

		LRESULT CALLBACK MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam);
	private:
		WindowInfo m_window;

	protected:
		std::unique_ptr<RenderBase> m_renderer;
	};
}
