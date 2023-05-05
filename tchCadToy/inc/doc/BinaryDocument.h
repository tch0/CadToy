#pragma once
#include <Document.h>

class BinaryDocument : public Document
{
public:
    // create a unnamed new document
    BinaryDocument();
    // implementation of saving content to file on disk
    virtual void saveContent() override;
};
