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