#include <imgui.h>

#include <FileTabBar.h>
#include <Global.h>
#include <DocManager.h>
#include <Command.h>

FileTabBar::FileTabBar()
{
}

FileTabBar::~FileTabBar()
{
}

void FileTabBar::draw()
{
    ImGuiWindowFlags tabWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav |
                                      ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin(g_FileTabBarTitle, nullptr, tabWindowFlags);
    // file tabs
    {
        ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs |
            ImGuiTabBarFlags_TabListPopupButton | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll;
        if (ImGui::BeginTabBar("FileTabBar", tabBarFlags))
        {
            // new drawing
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            {
                g_DocManager.newUnnamedDocument();
            }
            for (std::size_t i = 0; i < g_DocManager.documentsSize(); i++)
            {
                Document& doc = g_DocManager.documentAt(i);
                ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_None;
                if (doc.documentState() == Document::UnnamedChangedFile || doc.documentState() == Document::SavedChangedFile)
                {
                    tabItemFlags |= ImGuiTabItemFlags_UnsavedDocument;
                }
                bool open = true;
                if (ImGui::BeginTabItem(doc.fileName().c_str(), &open, tabItemFlags))
                {
                    if (g_DocManager.currentDocIndex() != i)
                    {
                        g_bIsDocumentChanged = true;
                    }
                    g_DocManager.setCurrentDocumentIndex(i);
                    ImGui::EndTabItem();
                }
                // tooltip
                if (ImGui::IsItemHovered())
                {
                    if (doc.documentState() == Document::SavedChangedFile || doc.documentState() == Document::SavedUnchangedFile)
                    {
                        ImGui::SetTooltip(doc.filePathString().c_str());
                    }
                    else
                    {
                        std::string fullFileName = doc.fileName() + doc.fileExtension();
                        ImGui::SetTooltip(fullFileName.c_str());
                    }
                }
                // the tab closed
                if (!open)
                {
                    g_DocManager.closeDocument(i);
                }
            }
            ImGui::EndTabBar();
        }
    }
    g_FileTabBarHeight = ImGui::GetWindowSize().y;
    ImGui::End();
}
