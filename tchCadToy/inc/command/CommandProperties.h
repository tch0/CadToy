#pragma once

#include <Command.h>

class CommandProperties : public Command
{
public:
    enum CommandCategory
    {
        Properties,
        PropertiesClose
    };
    CommandProperties();
    ~CommandProperties();
    virtual void execute(int category) override;
};

inline CommandProperties g_CmdProperties;