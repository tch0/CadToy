#include <CommandClose.h>
#include <Global.h>

CommandClose::CommandClose()
{
    registerCommand("close", this, 0);
}

CommandClose::~CommandClose()
{
    unregisterCommand("close");
}

void CommandClose::execute(int category)
{
    g_DocManager.closeDocument(g_DocManager.currentDocIndex());
}
