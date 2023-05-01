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
    registerCombinationKeyboardShortcut(true, false, false, false, ImGuiKey_N, "save");         // Ctrl+N
    registerCombinationKeyboardShortcut(true, false, false, false, ImGuiKey_O, "open");         // Ctrl+O
    registerCombinationKeyboardShortcut(true, false, false, false, ImGuiKey_S, "save");         // Ctrl+O
    registerCombinationKeyboardShortcut(true,  true, false, false, ImGuiKey_S, "saveas");       // Ctrl+Shift+S
    registerCombinationKeyboardShortcut(true, false, false, false, ImGuiKey_Q, "quit");         // Ctrl+Shift+S
    // Edit
    registerCombinationKeyboardShortcut(true, false, false, false, ImGuiKey_Z, "undo");         // Ctrl+Z
    registerCombinationKeyboardShortcut(true, false, false, false, ImGuiKey_Y, "redo");         // Ctrl+Y
    registerCombinationKeyboardShortcut(true, false, false, false, ImGuiKey_X, "cutclip");      // Ctrl+X
    registerCombinationKeyboardShortcut(true, false, false, false, ImGuiKey_C, "copyclip");     // Ctrl+C
    registerCombinationKeyboardShortcut(true, false, false, false, ImGuiKey_V, "pasteclip");    // Ctrl+V
    registerCombinationKeyboardShortcut(true, false, false, false, ImGuiKey_A, "selectall");    // Ctrl+A
    registerIndependentKeyboardShortcut(ImGuiKey_Delete, "erase");                              // Del


    // todo: let shortcuts works when focus is on command line window, Ctrl+V works differently (paste clipboard info to command line window), others are basically the same.
}

static std::string shortcutToString(const std::tuple<bool, bool, bool, bool, ImGuiKey>& shortcut)
{
    return std::format("{}{}{}{}{}",
        std::get<0>(shortcut) ? "Ctrl+" : "",
        std::get<1>(shortcut) ? "Shift+" : "",
        std::get<2>(shortcut) ? "Alt+" : "",
        std::get<3>(shortcut) ? "Supper+" : "",
        ImGui::GetKeyName(std::get<4>(shortcut)));
}

// register a combination keyboard shortcut, like Ctrl+S
void registerCombinationKeyboardShortcut(bool ctrl, bool shift, bool alt, bool supper, ImGuiKey key, const std::string& command, const std::source_location& loc)
{
    tchAssert(ctrl || shift || alt || supper, "Combination key must have one or more Ctrl/Shift/Alt/Supper modes, please register this as an independent shortcut!", loc);
    std::tuple<bool, bool, bool, bool, ImGuiKey> shortcut {ctrl, shift, alt, supper, key};
    auto iter = g_CombinationKeyboardShortcutsMap.find(shortcut);
    if (iter == g_CombinationKeyboardShortcutsMap.end())
    {
        g_CombinationKeyboardShortcutsMap.emplace(shortcut, command);
    }
    else
    {
        globalLogger().warning(std::format("Combination keyboard shortcut {} already exist, invalid repeat registration!", shortcutToString(shortcut)), loc);
    }
}

// register a independent keyboard shortcut, like delete
void registerIndependentKeyboardShortcut(ImGuiKey key, const std::string& command, const std::source_location& loc)
{
    for (auto iter = g_CombinationKeyboardShortcutsMap.begin(); iter != g_CombinationKeyboardShortcutsMap.end(); ++iter)
    {
        if (std::get<4>(iter->first) == key)
        {
            globalLogger().warning(std::format("Combination keyboard shortcut {} already existed, confilct with {}, independent key will precede combination key!", shortcutToString(iter->first), ImGui::GetKeyName(key)), loc);
        }
    }
    auto iter = g_IndependentKeyboardShortcutsMap.find(key);
    if (iter == g_IndependentKeyboardShortcutsMap.end())
    {
        g_IndependentKeyboardShortcutsMap.emplace(key, command);
    }
    else
    {
        globalLogger().warning(std::format("Independent keyboard shortcut {} already exist, invalid repeat registration!", ImGui::GetKeyName(key)), loc);
    }
}

// detect what keyboard short cut is pressed, if any, execute corresponding command.
void detectKeyboardShortcutAndExecute()
{
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        ImGuiIO& io = ImGui::GetIO();
        std::tuple<bool, bool, bool, bool, ImGuiKey> shortcut {io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper, ImGuiKey_None};
        // support all keyboards: numbers/alphas/punctuations/other control keys/F1~F12/keypad keys
        for (ImGuiKey key = ImGuiKey_Tab; key <= ImGuiKey_KeypadEqual; key = ImGuiKey(key+1))
        {
            // only choose first matching keyboard, exclude Ctrl/Shift/Alt/Supper, they are detected by io.KeyCtrl/io.KeyShift/io.KeyAlt/io.KeySuper
            if ((key < ImGuiKey_LeftCtrl || key > ImGuiKey_RightSuper) && ImGui::IsKeyDown(key))
            {
                std::get<4>(shortcut) = key;
                break;
            }
        }
        // no shortcut triggered
        if (std::get<4>(shortcut) == ImGuiKey_None)
        {
            return;
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
            // independent keys precede combination keys, press Ctrl+Delete also trigger shortcut Delete.
            // if no independent key find, execute combination key.
            auto iter = g_IndependentKeyboardShortcutsMap.find(std::get<4>(shortcut));
            if (iter != g_IndependentKeyboardShortcutsMap.end())
            {
                executeCommand(iter->second);
            }
            else
            {
               auto iter2 = g_CombinationKeyboardShortcutsMap.find(shortcut);
                if (iter2 != g_CombinationKeyboardShortcutsMap.end())
                {
                    executeCommand(iter2->second);
                }
            }
        }
    }
}