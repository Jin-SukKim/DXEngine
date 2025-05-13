#pragma once

namespace DE {
	class AppBase
	{
	public:
		AppBase();
		~AppBase();

		bool Initialize();
		int Run();

		void Update();
		void Render();

		bool InitWindow();
		bool InitGUI();

		LRESULT CALLBACK MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam);
	private:
		int m_screenWidth;
		int m_screenHeight;
		HWND m_mainWindow;

	};
}
