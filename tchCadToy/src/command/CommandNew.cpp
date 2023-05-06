#include <CommandNew.h>
#include <DocManager.h>
#include <Global.h>

CommandNew::CommandNew()
{
    registerCommand("new", this, 0);
}

CommandNew::~CommandNew()
{
    unregisterCommand("new");
}

void CommandNew::execute(int category)
{
    g_DocManager.newUnnamedDocument();
}
