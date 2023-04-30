#include <MainMenuBar.h>
#include <Global.h>

MainMenuBar::MainMenuBar()
{
    
}


void MainMenuBar::draw()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            drawMenuFile();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            drawMenuEdit();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void MainMenuBar::drawMenuFile()
{
    if (ImGui::MenuItem("New", "Ctrl+N"))
    {
        // todo: new file
    }
    if (ImGui::MenuItem("Open", "Ctrl+O"))
    {
        // todo: open file
    }
    if (ImGui::BeginMenu("Open Recent", !g_RecentFiles.empty()))
    {
        std::size_t index = 0;
        for (index = 0; index < g_RecentFiles.size() && index < 10; index++)
        {
            if (ImGui::MenuItem(g_RecentFiles[index].c_str()))
            {
                // todo: open corresponding file
            }
        }
        if (index < g_RecentFiles.size())
        {
            if (ImGui::MenuItem("More..."))
            {
                for (;index < g_RecentFiles.size(); index++)
                {
                    if (ImGui::MenuItem(g_RecentFiles[index].c_str()))
                    {
                        // todo: open corresponding file
                    }
                }
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Clear Recent Files", nullptr, false, true))
        {
            // todo: clear recent files
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Save", "Ctrl+S"))
    {
        // todo: save file
    }
    if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
    {
        // todo: save file as
    }
}

void MainMenuBar::drawMenuEdit()
{
    if (ImGui::MenuItem("Undo", "CTRL+Z"))
    {
        // todo: undo
    }
    if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))  // Disabled item
    {
        // todo: redo
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Cut", "CTRL+X"))
    {
        // todo: cut
    }
    if (ImGui::MenuItem("Copy", "CTRL+C"))
    {
        // todo: copy
    }
    if (ImGui::MenuItem("Paste", "CTRL+V"))
    {
        // todo: paste
    }
}
