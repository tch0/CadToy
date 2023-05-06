#pragma once
#include <Command.h>


class CommandClose : public Command
{
public:
    CommandClose();
    ~CommandClose();
    virtual void execute(int category) override;
};

inline CommandClose g_CommandClose;