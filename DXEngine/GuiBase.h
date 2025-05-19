#pragma once

namespace DE {
	class GuiBase
	{
	public:
		GuiBase();
		virtual ~GuiBase();

		virtual bool Initialize(const WindowInfo& window, class RenderBase& renderer);
		// Frame �����Ҷ� ȣ��
		void PreUpdate();
		// GUI�� ���� COntrol�� ������ ������ ���
		virtual void Update();
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
