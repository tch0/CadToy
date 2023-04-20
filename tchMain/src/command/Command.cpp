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
    std::for_each(command.begin(), command.end(), [](char c)->char { return std::tolower(c); });

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