#pragma once
#include <vector>
#include <string>
#include <format>
#include <list>

#include <imgui.h>


class CommandLineWindow
{
private:
    std::string m_InputBuffer;
    std::vector<std::string> m_Items;
    std::vector<std::string> m_Commands;
    std::vector<std::string> m_History;
    int m_HistoryPos; // -1: new line, 0 to m_History.size-1 browsing history
    ImGuiTextFilter m_Filter;
    bool m_bAutoScroll;
    bool m_bScrollToBottom;

public:
    CommandLineWindow();
    ~CommandLineWindow();
    // add log to command window
    void addLog(const std::string& log);
    // clear the command window
    void clearLog();
    // draw the command window, call in render loop
    void draw(const char* title, bool* pOpen, ImGuiWindowFlags flags, float& windowHeight);
    // paste a command (from clipboard)
    void pasteCommand();
    // Execute command: the command input is the whole unprocessed string, split it into a command and a leftover string inside this function,
    //  the first command is treated as command to be executed, and the leftover string will be saved as next inputs.
    void executeCommand(const std::string& command, bool bFromInputBox = false);

    // text edit call back, for completion, history
    int textEditCallback(ImGuiInputTextCallbackData* data);

private:
    // Split input command line into first command and leftover string.
    // Legal characters for command name:
    //  1. a-zA-Z0\-9, and leading \-+_
    //  2. spaces (\r\n\t and space) as separator.
    //  3. all other characters are just ignored.
    //  4. case unsensitive.
    // Extract first command from unprocessed input, keep the rest in unprocessed input.
    std::string extractCommandFromUnprocessedInput();

    // search command from commands map and execute it
    void executeCommandImpl(const std::string& command);
};
