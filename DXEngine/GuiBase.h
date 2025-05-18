#pragma once

namespace DE {
	class GuiBase
	{
	public:
		GuiBase();
		virtual ~GuiBase();

		virtual bool Initialize(const WindowInfo& window, class RenderBase* renderer);
		// Frame 시작할때 호출
		void PreUpdate();
		// GUI를 통해 COntrol할 값들이 있을떄 사용
		virtual void Update();
		void PostUpdate();
		virtual void Render();

		float GetDeltaTime();
	};
}
