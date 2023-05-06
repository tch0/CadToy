#include <DocManager.h>
#include <Logger.h>

DocManager::DocManager()
{
    // limit the max document count, reserve enough space for all documents/document attributes to prevent reference invalidation when capacity growth (command new)
    m_Documents.reserve(MAX_DOCUMENT_COUNT);
    m_DocCanvasAttrs.reserve(MAX_DOCUMENT_COUNT);
    m_DocCmdLineAttrs.reserve(MAX_DOCUMENT_COUNT);
}

DocManager::~DocManager()
{
}

// cureent document
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

// current document canvas attributes
DocManager::DocumentCanvasAttribute& DocManager::currentDocCanvasAttributes()
{
    if (m_Documents.empty())
    {
        newUnnamedDocument();
    }
    tchAssert(m_CurrentDocIndex < m_Documents.size());
    tchAssert(m_Documents[m_CurrentDocIndex] != nullptr, "current document is nullptr");
    return m_DocCanvasAttrs[m_CurrentDocIndex];
}

// current document's command line window attrbutes
DocManager::DocumentCmdLineAttribute& DocManager::currentDocCmdLineAttributes()
{
    if (m_Documents.empty())
    {
        newUnnamedDocument();
    }
    tchAssert(m_CurrentDocIndex < m_Documents.size());
    tchAssert(m_Documents[m_CurrentDocIndex] != nullptr, "current document is nullptr");
    return m_DocCmdLineAttrs[m_CurrentDocIndex];
}

// set current document index
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
    if (m_Documents.size() == MAX_DOCUMENT_COUNT)
    {
        cmdLinePrint(std::format("The amount of documents exceed the document number limit {}, can not create new document, maybe close some to create new!", MAX_DOCUMENT_COUNT));
        return false;
    }
    std::unique_ptr<Document> up;
    bool res = Document::openFile(filePath, up);
    if (res)
    {
        m_Documents.push_back(std::move(up));
        m_DocCanvasAttrs.push_back(DocumentCanvasAttribute{});
        m_DocCmdLineAttrs.push_back(DocumentCmdLineAttribute{});
        m_CurrentDocIndex = m_Documents.size() - 1;
    }
    return res;
}

// create a unnamed new document, convert to this document, return true if sucess, do nothing and return false when fail.
bool DocManager::newUnnamedDocument()
{
    if (m_Documents.size() == MAX_DOCUMENT_COUNT)
    {
        cmdLinePrint(std::format("The amount of documents exceed the document number limit {}, can not create new document, maybe close some to create new!", MAX_DOCUMENT_COUNT));
        return false;
    }
    std::unique_ptr<Document> up;
    bool res = Document::createUnnamedFile(up);
    if (res)
    {
        m_Documents.push_back(std::move(up));
        m_DocCanvasAttrs.push_back(DocumentCanvasAttribute{});
        m_DocCmdLineAttrs.push_back(DocumentCmdLineAttribute{});
        m_CurrentDocIndex = m_Documents.size() - 1;
    }
    return res;
}


// for documents traversing
std::size_t DocManager::currentDocIndex()
{
    return m_CurrentDocIndex;
}
std::size_t DocManager::documentsSize()
{
    if (m_Documents.empty())
    {
        newUnnamedDocument();
    }
    return m_Documents.size();
}
Document& DocManager::documentAt(std::size_t index)
{
    tchAssert(index <= m_Documents.size());
    return *m_Documents[index];
}

// message printing, for user interaction
void DocManager::cmdLinePrint(const std::string& message)
{
    DocumentCmdLineAttribute& cmdLineAttr = currentDocCmdLineAttributes();
    cmdLineAttr.commandLogs.push_back(message);
}
