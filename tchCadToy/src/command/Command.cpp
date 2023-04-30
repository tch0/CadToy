#include <algorithm>

#include <Command.h>
#include <Global.h>
#include <CommandProperties.h>
#include <Logger.h>

// register one command
void registerCommand(const std::string& commandName, Command* pCommand, int category)
{
    // case unsensitive, so save lower case command only.
    std::string command = commandName;
    std::for_each(command.begin(), command.end(), [](char& c)->void { c = std::tolower(c); });

    if (getCommandsMap().find(command) != getCommandsMap().end())
    {
        globalLogger().warning(std::format("Command {} existed, register failed!", command));
    }
    else
    {
        getCommandsMap().emplace(command, std::make_pair(pCommand, category));
    }
}

// unregister one command
void unregisterCommand(const std::string& commandName)
{
    if (getCommandsMap().find(commandName) == getCommandsMap().end())
    {
        globalLogger().warning(std::format("Command {} has been already unregistered or did not even registerd!", commandName));
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
