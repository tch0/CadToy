#pragma once

#include <string>
#include <source_location>

// Legal characters for command name:
//  1. a-zA-Z0-9, and leading -+_
//  2. spaces (\r\n\t and space) as separator.
//  3. all other characters are just ignored.
//  4. case unsensitive.
class Command
{
public:
    virtual void execute(int category) = 0;
};

// register one command
void registerCommand(const std::string& commandName, Command* pCommand, int category, const std::source_location& loc = std::source_location::current());
// unregister one command
void unregisterCommand(const std::string& commandName, const std::source_location& loc = std::source_location::current());

// Execute a specific command:
//      What actually doing is pushing new command to unprocessed input, the command execution mechanism will extract command from it and execute the command if the program is on a proper context to execute command.
//      If the program is currently not on a proper context (eg. in the execution process of another command and waiting for input), current executing command will be canceled and then execute new command.
void executeCommand(const std::string& command);

// Cancel current executing command:
//      equivalent to pressing multiple Esc until to the state of waiting for command.
void cancelCurrentCommand();

// manage all modal dialogs informations, to know whether a modal shows or not now.
void registerModal(const std::string& name, bool* pShow, const std::source_location& loc = std::source_location::current());
void unregisterModal(const std::string& name, const std::source_location& loc = std::source_location::current());

// maintain the state of g_bInCommandExecution, basically for modal dialogs
// - if a modal dialog shows, it's definitely in a command execution now, save old g_bInCommandExecution value and set it to true.
// - if a modal dialog closes, restore g_bInCommandExecution to the state before the modal dialog shows.
void maintainCommandExecutionState();
