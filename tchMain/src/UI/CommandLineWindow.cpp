#include <algorithm>
#include <string_view>

#include <CommandLineWindow.h>
#include <Logger.h>
#include <Global.h>


CommandLineWindow::CommandLineWindow()
    : m_HistoryPos(-1)
    , m_bAutoScroll(true)
    , m_bScrollToBottom(false)
{
    m_InputBuffer.resize(256);
}

CommandLineWindow::~CommandLineWindow()
{
}

// add log to command window
void CommandLineWindow::addLog(const std::string& log)
{
    m_Items.push_back(log);
}

// clear the command window
void CommandLineWindow::clearLog()
{
    m_Items.clear();
}

// draw the command window, call in render loop
void CommandLineWindow::draw(const char* title, bool* pOpen, ImGuiWindowFlags flags, float& windowHeight)
{
    if (!ImGui::Begin(title, pOpen, flags))
    {
        ImGui::End();
        return;
    }
    // for layout calculating
    windowHeight = ImGui::GetWindowSize().y;
    
    // draw filter
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Filter: ");
    ImGui::SameLine();
    m_Filter.Draw("##Filter", -1.0f); // align 1 pixel to the right of the window
    ImGui::Separator();

    // reserve enough left-over for 1 separator + 1 input text
    const float footerReserveHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerReserveHeight), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        bool bCopyHistoryToClipboard = false;
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear"))
            {
                clearLog();
            }
            if (ImGui::Selectable("Paste"))
            {
                pasteCommand();
            }
            if (ImGui::Selectable("Copy History"))
            {
                bCopyHistoryToClipboard = true;
            }
            // other...
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
        if (bCopyHistoryToClipboard)
        {
            ImGui::LogToClipboard();
        }
        for (std::size_t i = 0; i < m_Items.size(); i++)
        {
            // filter contents
            const std::string& item = m_Items[i];
            if (!m_Filter.PassFilter(item.c_str()))
            {
                continue;
            }
            // Normally you would store more information in your item than just a string.
            // (e.g. make Items[] an array of structure, store color/type etc.)
            ImVec4 color;
            bool bHasColor = false;
            if (item.starts_with("Command"))
            {
                color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
                bHasColor = true;
            }
            if (bHasColor)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, color);
            }
            ImGui::TextUnformatted(item.c_str());
            if (bHasColor)
            {
                ImGui::PopStyleColor();
            }
        }
        if (bCopyHistoryToClipboard)
        {
            ImGui::LogFinish();
        }

        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if (m_bScrollToBottom || (m_bAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        {
            ImGui::SetScrollHereY(1.0f);
        }
        m_bScrollToBottom = false;
        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::Separator();

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Command: ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f); // align 1 pixel to the right of the window

    // command line input
    bool bReclaimFocus = false;
    ImGuiInputTextFlags inputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit;
    auto callback = [](ImGuiInputTextCallbackData* data) -> int {
        CommandLineWindow* pWindow = (CommandLineWindow*)data->UserData;
        return pWindow->textEditCallback(data);
    };
    if (ImGui::InputTextWithHint("##Command Input", "Input command here", m_InputBuffer.data(), m_InputBuffer.size(), inputTextFlags, callback, (void*)this))
    {
        executeCommand(m_InputBuffer.c_str());
        std::fill(m_InputBuffer.begin(), m_InputBuffer.end(), 0); // clear input buffer
        bReclaimFocus = true;
    }

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (bReclaimFocus)
    {
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
    }
    ImGui::End();
}

// paste command
void CommandLineWindow::pasteCommand()
{
    addLog("todo: paste a command to input box");
}

// execute command
void CommandLineWindow::executeCommand(const std::string& command)
{
    m_HistoryPos = -1;
    auto riter = std::find(m_History.rbegin(), m_History.rend(), command);
    if (riter != m_History.rend())
    {
        m_History.erase(riter.base() - 1);
    }
    m_History.push_back(command);
    addLog(std::format("Command: {}", command));

    // todo: trim command, ignore case
    if (command == "properties")
    {
        g_bPropertiesSideBarOpen = true;
    }
    else if (command == "propertiesclose")
    {
        g_bPropertiesSideBarOpen = false;
    }
    else
    {
        addLog("To be implemented!");
    }
    
    m_bScrollToBottom = true;
}

// text edit call back, for completion, history
int CommandLineWindow::textEditCallback(ImGuiInputTextCallbackData* data)
{
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
        {
            addLog("command completion, to be implemented yet!");
            // todo: completion
            // get current command (start: data->Buf, length: data->CursorPos)
            // build a list of candidates
            // if no candidate, do nothing
            // if one cadidate, just complete
            // if multiple cadidates, show them all (in command line, or in a popup tooltip/menu)
        }
    case ImGuiInputTextFlags_CallbackHistory:
        {
            // history
            const int prevHistoryPos = m_HistoryPos;
            if (data->EventKey == ImGuiKey_UpArrow)
            {
                if (m_HistoryPos == -1)
                {
                    m_HistoryPos = int(m_History.size() - 1);
                }
                else if (m_HistoryPos > 0)
                {
                    m_HistoryPos--;
                }
            }
            else if (data->EventKey == ImGuiKey_DownArrow)
            {
                if (m_HistoryPos != -1)
                {
                    if (++m_HistoryPos >= m_History.size())
                    {
                        m_HistoryPos = -1;
                    }
                }
            }
            if (prevHistoryPos != m_HistoryPos)
            {
                const std::string& historyStr = (m_HistoryPos >= 0) ? m_History[m_HistoryPos] : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, historyStr.c_str());
            }
        }
    case ImGuiInputTextFlags_CallbackEdit:
        {
            // during editing
        }
    }
    return 0;
}