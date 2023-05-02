#include <algorithm>

#include <Command.h>
#include <Global.h>
#include <CommandProperties.h>
#include <Logger.h>

// register one command
void registerCommand(const std::string& commandName, Command* pCommand, int category, const std::source_location& loc)
{
    // case unsensitive, so save lower case command only.
    std::string command = commandName;
    std::for_each(command.begin(), command.end(), [](char& c)->void { c = std::tolower(c); });

    if (getCommandsMap().find(command) != getCommandsMap().end())
    {
        globalLogger().warning(std::format("Command {} existed, register failed!", command), loc);
    }
    else
    {
        getCommandsMap().emplace(command, std::make_pair(pCommand, category));
    }
}

// unregister one command
void unregisterCommand(const std::string& commandName, const std::source_location& loc)
{
    if (getCommandsMap().find(commandName) == getCommandsMap().end())
    {
        globalLogger().warning(std::format("Command {} has been already unregistered or did not even registerd!", commandName), loc);
    }
    else
    {
        getCommandsMap().erase(commandName);
    }
}

// Execute a specific command:
//      What actually doing is pushing new command to unprocessed input, the command execution mechanism will extract command from it and execute the command if the program is on a proper context to execute command.
//      If the program is currently not on a proper context (eg. in the execution process of another command and waiting for input), current executing command will be canceled and then execute new command.
void executeCommand(const std::string& command)
{
    if (g_bInCommandExecution)
    {
        cancelCurrentCommand();
    }
    g_UnprocessedInput.append(" " + command);
}

// Cancel current executing command:
//      equivalent to pressing multiple Esc until to the state of waiting for command.
void cancelCurrentCommand()
{
    while (g_bInCommandExecution)
    {
        // todo
    }
}

// manage all modal dialogs informations, to know whether a modal shows or not now.
void registerModal(const std::string& name, bool* pShow, const std::source_location& loc)
{
    auto iter = g_ModalsMap.find(name);
    if (iter != g_ModalsMap.end())
    {
        globalLogger().warning(std::format("Modal {} existed, register failed!", name), loc);
    }
    else if (pShow == nullptr)
    {
        globalLogger().warning(std::format("Register modal {} failed, pShow is nullptr!", name), loc);
    }
    else
    {
        g_ModalsMap.emplace(name, pShow);
    }
}
void unregisterModal(const std::string& name, const std::source_location& loc)
{
    if (g_ModalsMap.find(name) == g_ModalsMap.end())
    {
        globalLogger().warning(std::format("Modal {} does not exist or already unregistered, unregister failed!", name), loc);
    }
    else
    {
        g_ModalsMap.erase(name);
    }
}

// maintain the state of g_bInCommandExecution, basically for modal dialogs
// - if a modal dialog shows, it's definitely in a command execution now, save old g_bInCommandExecution value and set it to true.
// - if a modal dialog closes, restore g_bInCommandExecution to the state before the modal dialog shows.
void maintainCommandExecutionState()
{
    static bool s_bLastAnyModal = false;
    bool bAnyModal = false;
    for (auto iter = g_ModalsMap.begin(); iter != g_ModalsMap.end(); ++iter)
    {
        if (iter->second && *iter->second)
        {
            bAnyModal = true;
            break;
        }
    }
    static bool s_bOldCommandExecution = false;
    
    // the modal closed last frame, resoter the command execution state
    if (s_bLastAnyModal && !bAnyModal)
    {
        g_bInCommandExecution = s_bOldCommandExecution;
    }

    // save old command execution state
    if (bAnyModal)
    {
        g_bInCommandExecution = true;
    }
    else
    {
        s_bOldCommandExecution = g_bInCommandExecution;
    }
    
    s_bLastAnyModal = bAnyModal;
}