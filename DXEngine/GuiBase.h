#pragma once

namespace DE {
	class GuiBase
	{
		GENERATE_SINGLE(GuiBase)
	public:
		virtual ~GuiBase();

		virtual bool Initialize(const WindowInfo& window, class RenderBase& renderer);
		// Frame 시작할때 호출
		void PreUpdate();
		void Update();
		void PostUpdate();
		virtual void Render();

		static float GetDeltaTime();
		Vector2 GetSize();
		void SetSize(const Vector2& size);
		void SetPos(const Vector2& pos);
		bool IsSizeChanged();
	private:
		Vector2 prevSize;
	};
}
