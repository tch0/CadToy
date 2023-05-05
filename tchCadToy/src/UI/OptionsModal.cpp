#include <OptionsModal.h>
#include <Global.h>
#include <Command.h>

OptionsModal::OptionsModal()
{
    registerModal("options", &g_OptionsModalOpen);
}

OptionsModal::~OptionsModal()
{
    unregisterModal("options");
}

void OptionsModal::draw()
{
    if (g_OptionsModalOpen)
    {
        ImGui::OpenPopup("Options");
        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Options", &g_OptionsModalOpen))
        {
            if (ImGui::BeginTabBar("OptionsTab", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("File"))
                {
                    drawFileTab();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Display"))
                {
                    drawDisplayTab();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Drawing"))
                {
                    drawDrawingTab();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndPopup();
        }
    }
}

void OptionsModal::drawFileTab()
{

}


void OptionsModal::drawDisplayTab()
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
    // cursor color
    {
        glm::vec4 cursorColor = g_CursorColor;
        ImGui::ColorEdit4("Cursor Color", (float*)&cursorColor);
        if (cursorColor != g_CursorColor)
        {
            g_CursorColor = cursorColor;
        }
    }
}

void OptionsModal::drawDrawingTab()
{

}
