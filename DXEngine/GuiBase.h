#pragma once

namespace DE {
	class GuiBase
	{
	public:
		GuiBase();
		virtual ~GuiBase();

		virtual bool Initialize(const HWND& window, const int& screenWidth, const int& screenHeight);
		// Frame �����Ҷ� ȣ��
		void PreUpdate();
		// GUI�� ���� COntrol�� ������ ������ ���
		virtual void Update();
		void PostUpdate();
		virtual void Render();
	};
}
