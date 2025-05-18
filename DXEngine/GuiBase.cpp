#include "pch.h"
#include "GuiBase.h"
#include "RenderBase.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

namespace DE {
	GuiBase::GuiBase()
	{
	}
	GuiBase::~GuiBase()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	bool GuiBase::Initialize(const WindowInfo& window, RenderBase* renderer)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.DisplaySize = ImVec2(float(window.width), float(window.height));
		ImGui::StyleColorsDark();

		// Setup Playform/Renderer backends
		if (!ImGui_ImplDX11_Init(renderer->GetDevice().Get(), renderer->GetContext().Get()))
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
		ImGui::Begin("Scene Control");

		// ImGui가 측정해주는 Framerate 출력
		ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
			1000.0f / ImGui::GetIO().Framerate,
			ImGui::GetIO().Framerate);
	}

	void GuiBase::Update()
	{
	}

	void GuiBase::PostUpdate()
	{
		ImGui::End();
		ImGui::Render();
	}
	void GuiBase::Render()
	{
		// GUI 렌더링
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	float GuiBase::GetDeltaTime()
	{
		return ImGui::GetIO().DeltaTime;
	}
}