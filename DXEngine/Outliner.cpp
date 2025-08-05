#include "pch.h"
#include "Outliner.h"

namespace DE {
    void Outliner::Initialize() {
    }

	void Outliner::Update() {
		ImGui::Begin("Outliner", &m_show);
		// Outliner 업데이트 로직을 여기에 구현

        static ImGuiSelectionBasicStorage selection;
        const int ITEMS_COUNT = (int)m_actors.size();
        //if (ImGui::BeginChild("Outliner", ImVec2(-FLT_MIN, ImGui::GetFontSize() * 20), ImGuiChildFlags_ResizeY))
        if (ImGui::BeginChild("Outliner", ImVec2(0.f, 0.f)))
        {
            ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
            ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(flags, selection.Size, ITEMS_COUNT);
            selection.ApplyRequests(ms_io);

            ImGuiListClipper clipper;
            clipper.Begin(ITEMS_COUNT);
            if (ms_io->RangeSrcItem != -1)
                clipper.IncludeItemByIndex((int)ms_io->RangeSrcItem); // Ensure RangeSrc item is not clipped.
            while (clipper.Step())
            {
                for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                {
                    char label[64];
					std::wcstombs(label, m_actors[n]->GetName().c_str(), sizeof(label) - 1);
                    bool item_is_selected = selection.Contains((ImGuiID)n);
                    ImGui::SetNextItemSelectionUserData(n);
                    if (ImGui::Selectable(label, item_is_selected)) {
                        SetSelectedActor(m_actors[n]);
                    }
                }
            }

            ms_io = ImGui::EndMultiSelect();
            selection.ApplyRequests(ms_io);
        }
        ImGui::EndChild();

		ImGui::End();
	}

    void Outliner::SetActorLists(const std::vector<std::vector<std::shared_ptr<Actor>>>& actorLists)
    {
        for (const auto& actorList : actorLists) {
            for (const auto& actor : actorList) {
                if (actor) {
                    m_actors.emplace_back(actor);
                }
            }
        }
    }
} // namespace DE