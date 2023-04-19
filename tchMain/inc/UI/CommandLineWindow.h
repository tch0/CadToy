#pragma once
#include <vector>
#include <string>
#include <format>

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
    // execute a command
    void executeCommand(const std::string& command);

    // text edit call back, for completion, history
    int textEditCallback(ImGuiInputTextCallbackData* data);
};
