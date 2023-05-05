#pragma once
#include <memory>
#include <vector>
#include <filesystem>

#include <Document.h>

// will be a singleton
class DocManager
{
private:
    std::vector<std::unique_ptr<Document>> m_Documents;
    std::size_t m_CurrentDocIndex {0};
public:
    DocManager();
    ~DocManager();
    Document& currentDoc();
    void setCurrentDocumentIndex(std::size_t index);
    // open a existing file, do some necessary initialization for this document, convert to this document, return true if sucess, do nothing and return false when fail.
    bool newNamedDocument(const std::filesystem::path& filePath);
    // create a unnamed new document, convert to this document, return true if sucess, do nothing and return false when fail.
    bool newUnnamedDocument();
};
