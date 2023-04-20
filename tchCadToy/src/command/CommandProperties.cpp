#include <CommandProperties.h>
#include <Global.h>

CommandProperties::CommandProperties()
{
    registerCommand("properties", this, Properties);
    registerCommand("propertiesclose", this, PropertiesClose);
}

CommandProperties::~CommandProperties()
{
    unregisterCommand("properties");
    unregisterCommand("propertiesclose");
}

void CommandProperties::execute(int category)
{
    if (category == Properties)
    {
        g_bPropertiesSideBarOpen = true;
    }
    else if (category == PropertiesClose)
    {
        g_bPropertiesSideBarOpen = false;
    }
}