#include "pch.h"
#include "GuiBase.h"
#include "RenderBase.h"

namespace DE {
	GuiBase::~GuiBase()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	bool GuiBase::Initialize(const WindowInfo& window, RenderBase& renderer)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.DisplaySize = ImVec2(float(window.width), float(window.height));
		ImGui::StyleColorsDark();

		// Setup Playform/Renderer backends
		if (!ImGui_ImplDX11_Init(renderer.GetDevice().Get(), renderer.GetContext().Get()))
			return false;

		if (!ImGui_ImplWin32_Init(window.hwnd)) {
			return false;
		}

		return true;
	}

	void GuiBase::PreUpdate()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();
	}

	void GuiBase::Render()
	{
		ImGui::Render();
		// GUI ·»´õ¸µ
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	float GuiBase::GetDeltaTime()
	{
		return ImGui::GetIO().DeltaTime;
	}

	Vector2 GuiBase::GetSize()
	{
		auto size = ImGui::GetWindowSize();
		return { size.x, size.y };
	}

	void GuiBase::SetSize(const Vector2& size)
	{
		ImGui::SetWindowSize({ size.x, size.y });
	}

	void GuiBase::SetPos(const Vector2& pos)
	{
		ImGui::SetWindowPos({ pos.x, pos.y });
	}

	bool GuiBase::IsSizeChanged()
	{
		Vector2 size = GetSize();
		if (prevSize != size) 
			return true;

		prevSize = size;
		return false;
	}
}