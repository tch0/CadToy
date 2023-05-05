#include <BinaryDocument.h>
#include <Logger.h>

// create a unnamed new document
BinaryDocument::BinaryDocument()
{
}

// implementation of saving content to file on disk
void BinaryDocument::saveContent()
{
    // todo yet: print a log for now
    globalLogger().info(std::format("File {} saved!", fileName()));
}
