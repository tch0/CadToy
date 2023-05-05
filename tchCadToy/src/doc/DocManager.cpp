#include <DocManager.h>
#include <Logger.h>

DocManager::DocManager()
{
}

DocManager::~DocManager()
{
}

Document& DocManager::currentDoc()
{
    if (m_Documents.empty())
    {
        newUnnamedDocument();
    }
    tchAssert(m_CurrentDocIndex < m_Documents.size());
    tchAssert(m_Documents[m_CurrentDocIndex] != nullptr, "current document is nullptr");
    return *m_Documents[m_CurrentDocIndex];
}

void DocManager::setCurrentDocumentIndex(std::size_t index)
{
    if (index > m_Documents.size())
    {
        globalLogger().warning("Document index input out of range, set current to first document!");
        index = 0;
    }
    m_CurrentDocIndex = index;
}

// open a existing file, do some necessary initialization for this document, convert to this document, return true if sucess, do nothing and return false when fail.
bool DocManager::newNamedDocument(const std::filesystem::path& filePath)
{
    std::unique_ptr<Document> up;
    bool res = Document::openFile(filePath, up);
    if (res)
    {
        m_Documents.push_back(std::move(up));
        m_CurrentDocIndex = m_Documents.size() - 1;
    }
    return res;
}

// create a unnamed new document, convert to this document, return true if sucess, do nothing and return false when fail.
bool DocManager::newUnnamedDocument()
{
    std::unique_ptr<Document> up;
    bool res = Document::createUnnamedFile(up);
    if (res)
    {
        m_Documents.push_back(std::move(up));
        m_CurrentDocIndex = m_Documents.size() - 1;
    }
    return res;
}

