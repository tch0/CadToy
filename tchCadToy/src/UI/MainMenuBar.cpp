#include <functional>

#include <MainMenuBar.h>
#include <Global.h>
#include <Command.h>

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
        if (ImGui::BeginMenu("Tools"))
        {
            drawMenuTools();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void MainMenuBar::drawMenuFile()
{
    if (ImGui::MenuItem("New", "Ctrl+N"))
    {
        executeCommand("new");
    }
    if (ImGui::MenuItem("Open", "Ctrl+O"))
    {
        executeCommand("open");
    }
    if (ImGui::BeginMenu("Open Recent", !g_RecentFiles.empty()))
    {
        std::function<void(std::size_t)> showFilesFrom = [&showFilesFrom](std::size_t start) -> void
        {
            std::size_t i = start;
            for (; i < g_RecentFiles.size() && i < start + 30; i++)
            {
                if (ImGui::MenuItem(g_RecentFiles[i].c_str()))
                {
                    // todo: execute open in command mode, not dialog mode
                    executeCommand("open " + g_RecentFiles[i]);
                }
            }
            if (i < g_RecentFiles.size())
            {
                if (ImGui::BeginMenu("More..."))
                {
                    showFilesFrom(i);
                    ImGui::EndMenu();
                }
            }
        };
        // show recent file recursively
        showFilesFrom(0);
        ImGui::Separator();
        if (ImGui::MenuItem("Clear Recent Files", nullptr, false, true))
        {
            g_RecentFiles.clear();
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Save", "Ctrl+S"))
    {
        executeCommand("save");
    }
    if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
    {
        executeCommand("saveas");
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Quit", "Ctrl+Q"))
    {
        executeCommand("quit");
    }
}

void MainMenuBar::drawMenuEdit()
{
    if (ImGui::MenuItem("Undo", "CTRL+Z"))
    {
        executeCommand("undo");
    }
    if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))  // Disabled item for now
    {
        executeCommand("redo");
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Cut", "CTRL+X"))
    {
        executeCommand("cutclip");
    }
    if (ImGui::MenuItem("Copy", "CTRL+C"))
    {
        executeCommand("copyclip");
    }
    if (ImGui::MenuItem("Paste", "CTRL+V"))
    {
        executeCommand("pasteclip");
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Select All", "Ctrl+A"))
    {
        executeCommand("selectall");
    }
    if (ImGui::MenuItem("Erase", "Del"))
    {
        executeCommand("erase");
    }
}

void MainMenuBar::drawMenuTools()
{
    if (ImGui::MenuItem("Options"))
    {
        executeCommand("options");
    }
}
