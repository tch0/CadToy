#pragma once
#include <Command.h>

class CommandNew : public Command
{
public:
    CommandNew();
    ~CommandNew();
    virtual void execute(int category) override;
};

inline CommandNew g_CommandNew;