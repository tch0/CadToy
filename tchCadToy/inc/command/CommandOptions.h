#pragma once
#include <Command.h>

class CommandOptions : public Command
{
public:
    enum CommandCategory
    {
        NoCategory
    };
    CommandOptions();
    ~CommandOptions();
    virtual void execute(int category) override;
};

inline CommandOptions g_CommandOptions;