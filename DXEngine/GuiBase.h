#pragma once

namespace DE {
	class GuiBase
	{
	public:
		GuiBase();
		virtual ~GuiBase();

		virtual bool Initialize(const WindowInfo& window);
		// Frame �����Ҷ� ȣ��
		void PreUpdate();
		// GUI�� ���� COntrol�� ������ ������ ���
		virtual void Update();
		void PostUpdate();
		virtual void Render();
	};
}
