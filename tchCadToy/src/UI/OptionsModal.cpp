#include <OptionsModal.h>
#include <Global.h>

void OptionsModal::draw()
{
    if (g_OptionsModalOpen)
    {
        ImGui::OpenPopup("Options");
        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Options", &g_OptionsModalOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            // pick box sizes
            {
                int pickBoxSize = g_Canvas.getPickBoxSize();
                ImGui::SliderInt("Pickbox Size", &pickBoxSize, 0, 50);
                g_Canvas.setPickBoxSize(pickBoxSize);
            }
            // cursor size
            {
                int cursorSize = g_Canvas.getCursorSize();
                ImGui::SliderInt("Cursor Size", &cursorSize, 10, 200);
                g_Canvas.setCursorSize(cursorSize);
            }

            ImGui::EndPopup();
        }
    }
}