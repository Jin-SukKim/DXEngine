#pragma once

namespace DE {
	class GuiBase
	{
	public:
		GuiBase();
		virtual ~GuiBase();

		virtual bool Initialize(const WindowInfo& window, class RenderBase* renderer);
		// Frame �����Ҷ� ȣ��
		void PreUpdate();
		// GUI�� ���� COntrol�� ������ ������ ���
		virtual void Update();
		void PostUpdate();
		virtual void Render();

		float GetDeltaTime();
	};
}
