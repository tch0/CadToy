#include <vector>

#include <Keyboard.h>
#include <Global.h>
#include <Command.h>
#include <Logger.h>

// register all keyboard shortcuts here
void registerAllKeyboardShortcuts()
{
    // main menu bar item shortcuts:
    // File
    registerOneKeyboardShortcut(true, false, false, false, ImGuiKey_N, "save");         // Ctrl+N
    registerOneKeyboardShortcut(true, false, false, false, ImGuiKey_O, "open");         // Ctrl+O
    registerOneKeyboardShortcut(true, false, false, false, ImGuiKey_S, "save");         // Ctrl+O
    registerOneKeyboardShortcut(true,  true, false, false, ImGuiKey_S, "saveas");       // Ctrl+Shift+S
    registerOneKeyboardShortcut(true, false, false, false, ImGuiKey_Q, "quit");         // Ctrl+Shift+S
    // Edit
    registerOneKeyboardShortcut(true, false, false, false, ImGuiKey_Z, "undo");         // Ctrl+Z
    registerOneKeyboardShortcut(true, false, false, false, ImGuiKey_Y, "redo");         // Ctrl+Y
    registerOneKeyboardShortcut(true, false, false, false, ImGuiKey_X, "cutclip");      // Ctrl+X
    registerOneKeyboardShortcut(true, false, false, false, ImGuiKey_C, "copyclip");     // Ctrl+C
    registerOneKeyboardShortcut(true, false, false, false, ImGuiKey_V, "pasteclip");    // Ctrl+V

    // todo: let shortcuts works when focus is on command line window, Ctrl+V works differently (paste clipboard info to command line window), others are the same.
}


// register one keyboard shortcut
void registerOneKeyboardShortcut(bool ctrl, bool shift, bool alt, bool supper, ImGuiKey key, const std::string& command, const std::source_location& loc)
{
    std::tuple<bool, bool, bool, bool, ImGuiKey> shortcut {ctrl, shift, alt, supper, key};
    auto iter = g_KeyboardShortcutsMap.find(shortcut);
    if (iter == g_KeyboardShortcutsMap.end())
    {
        g_KeyboardShortcutsMap.emplace(shortcut, command);
    }
    else
    {
        std::string shortcutStr = std::format("{}{}{}{}{}",
            std::get<0>(shortcut) ? "Ctrl+" : "",
            std::get<1>(shortcut) ? "Shift+" : "",
            std::get<2>(shortcut) ? "Alt+" : "",
            std::get<3>(shortcut) ? "Supper+" : "",
            ImGui::GetKeyName(std::get<4>(shortcut)));
        globalLogger().warning(std::format("Keyboard shortcut {} already exist, invalid repeat registration!", shortcutStr), loc);
    }
}

// detect what keyboard short cut is pressed, if any, execute corresponding command.
void detectKeyboardShortcutAndExecute()
{
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        ImGuiIO& io = ImGui::GetIO();
        std::tuple<bool, bool, bool, bool, ImGuiKey> shortcut {io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper, ImGuiKey_None};
        // only detect numbers, characters and punctuations here
        for (ImGuiKey key = ImGuiKey_0; key <= ImGuiKey_GraveAccent; key = ImGuiKey(key+1))
        {
            // only choose first matching character
            if (ImGui::IsKeyDown(key))
            {
                std::get<4>(shortcut) = key;
                break;
            }
        }
        bool bExecuteCommand = false;
        if (g_LastKeyBoardShortcut != shortcut)
        {
            // new shortcut pressed, execute it
            g_LastKeyBoardShortcut = shortcut;
            g_LastKeyboardShortcutTimePoint = ImGui::GetTime();
            bExecuteCommand = true;
        }
        else
        {
            // if current shortcut is hold for more than 200ms, execute it for second time, more times hold, more times to execute.
            if (ImGui::GetTime() - g_LastKeyboardShortcutTimePoint > 0.2)
            {
                bExecuteCommand = true;
                g_LastKeyboardShortcutTimePoint = ImGui::GetTime();
            }
        }
        if (bExecuteCommand)
        {
            auto iter = g_KeyboardShortcutsMap.find(shortcut);
            if (iter != g_KeyboardShortcutsMap.end())
            {
                executeCommand(iter->second);
            }
        }
    }
}