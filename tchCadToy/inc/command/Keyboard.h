#pragma once
#include <string>
#include <utility>
#include <source_location>

#include <imgui.h>

// register all keyboard shortcuts here
void registerAllKeyboardShortcuts();

// register a combination keyboard shortcut, like Ctrl+S
void registerCombinationKeyboardShortcut(bool ctrl, bool shift, bool alt, bool supper, ImGuiKey key, const std::string& command, const std::source_location& loc = std::source_location::current());

// register a independent keyboard shortcut, like delete
void registerIndependentKeyboardShortcut(ImGuiKey key, const std::string& command, const std::source_location& loc = std::source_location::current());

// detect what keyboard short cut is pressed, if any, execute corresponding command.
// all keyboard shortcuts are global, if imgui need accept current keyboard input, it won't pass to here.
void detectKeyboardShortcutAndExecute();

