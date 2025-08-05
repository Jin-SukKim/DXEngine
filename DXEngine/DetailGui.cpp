#include "pch.h"
#include "DetailGui.h"

#include "CameraActor.h"
#include "LightActor.h"
#include "Actor.h"

namespace DE
{
	void DetailGui::Update()
	{
		ImGui::Begin("Detail", &m_show);
		// ImGui가 측정해주는 Framerate 출력
		ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
			1000.0f / ImGui::GetIO().Framerate,
			ImGui::GetIO().Framerate);

		if (auto& actor = GetSelectedActor()) {
			ImGui::Text("Name: %ls", actor->GetName().c_str());

			if (auto camera = std::dynamic_pointer_cast<CameraActor>(actor)) {
				UpdateCamera();
			}
			else if (auto light = std::dynamic_pointer_cast<LightActor>(actor)) {
				UpdateLight();
			}
			else {
				UpdateActor();
			}

		}

		ImGui::End();
	}

	void DetailGui::UpdateActor()
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Transform")) {


			ImGui::TreePop();
		}

		ImGui::SetNextItemOpen(false, ImGuiCond_Once);
		if (ImGui::TreeNode("Mesh")) {


			ImGui::TreePop();
		}

		ImGui::SetNextItemOpen(false, ImGuiCond_Once);
		if (ImGui::TreeNode("Material Constants")) {


			ImGui::TreePop();
		}
	}

	void DetailGui::UpdateCamera()
	{

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Transform")) {


			ImGui::TreePop();
		}

		ImGui::SetNextItemOpen(false, ImGuiCond_Once);
		if (ImGui::TreeNode("Projection")) {


			ImGui::TreePop();
		}
	}

	void DetailGui::UpdateLight()
	{

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Transform")) {


			ImGui::TreePop();
		}

		ImGui::SetNextItemOpen(false, ImGuiCond_Once);
		if (ImGui::TreeNode("Projection")) {


			ImGui::TreePop();
		}

		ImGui::SetNextItemOpen(false, ImGuiCond_Once);
		if (ImGui::TreeNode("Light Options")) {


			ImGui::TreePop();
		}
	}
}