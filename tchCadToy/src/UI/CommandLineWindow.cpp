#include <algorithm>
#include <string_view>

#include <CommandLineWindow.h>
#include <Logger.h>
#include <Global.h>
#include <DocManager.h>


CommandLineWindow::CommandLineWindow()
    : m_bAutoScroll(true)
    , m_bScrollToBottom(false)
{
}

CommandLineWindow::~CommandLineWindow()
{
}

// add log to command window
void CommandLineWindow::addLog(const std::string& log)
{
    DocManager::DocumentCmdLineAttribute& attr = g_DocManager.currentDocCmdLineAttributes();
    attr.commandLogs.push_back(log);
}

// clear the command window
void CommandLineWindow::clearLog()
{
    DocManager::DocumentCmdLineAttribute& attr = g_DocManager.currentDocCmdLineAttributes();
    attr.commandLogs.clear();
}

// draw the command window, call in render loop
void CommandLineWindow::draw(const char* title, bool* pOpen, ImGuiWindowFlags flags, float& windowHeight)
{
    DocManager::DocumentCmdLineAttribute& attr = g_DocManager.currentDocCmdLineAttributes();

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
        for (std::size_t i = 0; i < attr.commandLogs.size(); i++)
        {
            // filter contents
            const std::string& item = attr.commandLogs[i];
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

        // Keep up at the bottom of the scroll region if it was already at the bottom at the beginning of the frame.
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

    // if not in the execution of a command, deal with unprocessed input (if there is any), is it suitable to process here?
    if (!attr.unprocessedInput.empty() && !attr.bInCommandExecution)
    {
        executeCommand("", false);
    }

    // command line input
    bool bReclaimFocus = false;
    ImGuiInputTextFlags inputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit;
    auto callback = [](ImGuiInputTextCallbackData* data) -> int {
        CommandLineWindow* pWindow = (CommandLineWindow*)data->UserData;
        return pWindow->textEditCallback(data);
    };
    if (ImGui::InputTextWithHint("##Command Input", "Input command here", attr.inputBuffer.data(), attr.inputBuffer.size(), inputTextFlags, callback, (void*)this))
    {
        executeCommand(attr.inputBuffer.c_str(), true);
        std::fill(attr.inputBuffer.begin(), attr.inputBuffer.end(), 0); // clear input buffer
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

// Execute command: the command input is the whole unprocessed string, split it into a command and a leftover string inside this function,
//  the first command is treated as command to be executed, and the leftover string will be saved as next inputs.
void CommandLineWindow::executeCommand(const std::string& command, bool bFromInputBox)
{
    DocManager::DocumentCmdLineAttribute& attr = g_DocManager.currentDocCmdLineAttributes();

    // add input command to unprocessed input
    attr.unprocessedInput += command;

    // do not execute if the program is in the execution of a command, the executing command will extract input from attr.unprocessedInput.
    if (attr.bInCommandExecution)
    {
        return;
    }
    
    // extract command from unprocessed input, then process it
    std::string commandToBeExecuted = extractCommandFromUnprocessedInput();

    // if extract nothing from unprocessed input, and it's not from input box, just return, nothing to execute.
    if (commandToBeExecuted.empty() && !bFromInputBox)
    {
        return;
    }

    // if command is empty, and from input box, execute last command in history, and do nothing to history.
    if (commandToBeExecuted.empty() && bFromInputBox)
    {
        if (attr.history.empty())
        {
            addLog(std::format("Command: {}", commandToBeExecuted));
            return;
        }
        commandToBeExecuted = attr.history.back();
        addLog(std::format("Command:\n{}", commandToBeExecuted)); // display the command in the second line to distinguish with other cases from outputs.
        executeCommandImpl(commandToBeExecuted);
        return;
    }

    // add command to the top of history, and erase the same command from history.
    attr.historyPos = -1;
    auto riter = std::find(attr.history.rbegin(), attr.history.rend(), commandToBeExecuted);
    if (riter != attr.history.rend())
    {
        attr.history.erase(riter.base() - 1);
    }
    attr.history.push_back(commandToBeExecuted);
    addLog(std::format("Command: {}", commandToBeExecuted));

    // execute command
    executeCommandImpl(commandToBeExecuted);
    
    // auto scroll to bottom after the command is executed.
    m_bScrollToBottom = true;
}

// text edit call back, for completion, history
int CommandLineWindow::textEditCallback(ImGuiInputTextCallbackData* data)
{
    DocManager::DocumentCmdLineAttribute& attr = g_DocManager.currentDocCmdLineAttributes();

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
            const int prevHistoryPos = attr.historyPos;
            if (data->EventKey == ImGuiKey_UpArrow)
            {
                if (attr.historyPos == -1)
                {
                    attr.historyPos = int(attr.history.size() - 1);
                }
                else if (attr.historyPos > 0)
                {
                    attr.historyPos--;
                }
            }
            else if (data->EventKey == ImGuiKey_DownArrow)
            {
                if (attr.historyPos != -1)
                {
                    if (++attr.historyPos >= attr.history.size())
                    {
                        attr.historyPos = -1;
                    }
                }
            }
            if (prevHistoryPos != attr.historyPos)
            {
                const std::string& historyStr = (attr.historyPos >= 0) ? attr.history[attr.historyPos] : "";
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


static inline bool isOneOf(char c, const std::string& str)
{
    return std::find(str.begin(), str.end(), c) != str.end();
}

// Legal characters for command name:
//  1. a-zA-Z0-9, and leading -+_.
//  2. spaces (\r\n\t and space) as separator.
//  3. all other characters are just ignored, include middle -+_.
//  4. case unsensitive.
// Extract first command from unprocessed input, keep the rest in unprocessed input.
std::string CommandLineWindow::extractCommandFromUnprocessedInput()
{
    DocManager::DocumentCmdLineAttribute& attr = g_DocManager.currentDocCmdLineAttributes();

    std::string command;
    static const std::string spaces = "\r\n\t ";
    static const std::string leadings = "-+_";
    for (std::size_t i = 0; i < attr.unprocessedInput.size(); i++)
    {
        // \r\n\t and space as sperator
        if (isOneOf(attr.unprocessedInput[i], spaces))
        {
            if (!command.empty())
            {
                attr.unprocessedInput = attr.unprocessedInput.substr(i+1);
                return command;
            }
        }
        // -+_
        else if (isOneOf(attr.unprocessedInput[i], leadings))
        {
            if (command.empty()) // middle -+_ are ignored.
            {
                // leading -+_  are treated as legal command name.
                command.push_back(std::tolower(attr.unprocessedInput[i])); // save lower case command
            }
        }
        // normal legal command characters
        else if ((attr.unprocessedInput[i] >= '0' && attr.unprocessedInput[i] <= '9') ||
                 (attr.unprocessedInput[i] >= 'a' && attr.unprocessedInput[i] <= 'z') ||
                 (attr.unprocessedInput[i] >= 'A' && attr.unprocessedInput[i] <= 'Z'))
        {
            command.push_back(std::tolower(attr.unprocessedInput[i])); // save lower case command
        }
        // other illegal characters are just ignored
    }
    attr.unprocessedInput.clear();
    return command;
}

// search command from commands map and execute it
// the input command is already in lower case, do not need more processing.
void CommandLineWindow::executeCommandImpl(const std::string& command)
{
    auto& commandsMap = getCommandsMap();
    auto iter = commandsMap.find(command);
    if (iter == commandsMap.end())
    {
        addLog(std::format("Unknown command: {}", command));
    }
    else
    {
        auto [pCommand, category] = iter->second;
        if (pCommand)
        {
            pCommand->execute(category);
        }
    }
}

