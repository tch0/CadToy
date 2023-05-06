#include <CloseModal.h>
#include <Command.h>
#include <Global.h>
#include <Logger.h>

CloseModal::CloseModal()
{
    registerModal("close", &g_CloseModalOpen);
}

CloseModal::~CloseModal()
{
    unregisterModal("close");
}

void CloseModal::draw()
{
    if (g_CloseModalOpen)
    {
        ImGui::OpenPopup("close");
        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("close", &g_CloseModalOpen))
        {
            Document& doc = g_DocManager.currentDoc();
            auto state = doc.documentState();
            tchAssert(state == Document::UnnamedChangedFile || state == Document::SavedChangedFile, "The file need to be changed to pop up close modal!");
            std::string filename;
            if (state == Document::UnnamedChangedFile)
            {
                // todo: let user choose path for unnamed file
                filename = doc.fileName() + doc.fileExtension();
            }
            else if (state == Document::SavedChangedFile)
            {
                filename = doc.filePathString();
            }
            std::string prompt = std::format("Save changes to {}?", filename);
            ImGui::Text(prompt.c_str());
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                doc.save(); // save document to disk
                doc.setDocumentState(Document::SavedUnchangedFile);
                g_DocManager.removeDocument(g_DocManager.currentDocIndex());
                g_CloseModalOpen = false; // close modal
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                g_CloseModalOpen = false; // close modal
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}