#include <imgui.h>

#include <CommandOptions.h>
#include <Global.h>

CommandOptions::CommandOptions()
{
    registerCommand("options", this, NoCategory);
}

CommandOptions::~CommandOptions()
{
    unregisterCommand("options");
}

void CommandOptions::execute(int category)
{
    g_OptionsModalOpen = true;
}

