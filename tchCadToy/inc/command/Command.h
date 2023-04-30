#pragma once

#include <string>


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
void registerCommand(const std::string& commandName, Command* pCommand, int category);
// unregister one command
void unregisterCommand(const std::string& commandName);

// Execute a specific command:
//      What actually doing is pushing new command to unprocessed input, the command execution mechanism will extract command from it and execute the command if the program is on a proper context to execute command.
//      If the program is currently not on a proper context (eg. in the execution process of another command and waiting for input), current executing command will be canceled and then execute new command.
void executeCommand(const std::string& command);

// Cancel current executing command:
//      equivalent to pressing multiple Esc until to the state of waiting for command.
void cancelCurrentCommand();
