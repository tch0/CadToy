#include <Document.h>
#include <BinaryDocument.h>
#include <Logger.h>

Document::Document()
    : m_DocState(UnnamedNewFile)
    , m_FileName("unnamed")
{
}

Document::~Document()
{
}

Document::DocumentState Document::documentState()
{
    return m_DocState;
}

void Document::setDocumentState(DocumentState state)
{
    m_DocState = state;
}

std::string Document::fileName()
{
    return m_FileName;
}

std::string Document::filePathString()
{
    return m_FilePathString;
}

void Document::save()
{
    saveContent();
    m_DocState = SavedUnchangedFile;
}

// open file on disk, return true if success, save result to upRes
bool Document::openFile(const std::filesystem::path& filePath, std::unique_ptr<Document>& upRes)
{
    // todo : choose different file format according to file extension, only support BinaryDocument for now.
    std::fstream fileStream(filePath, std::ios::binary | std::ios::in | std::ios::out);
    if (fileStream.is_open())
    {
        upRes = std::unique_ptr<Document>(new BinaryDocument());
        upRes->m_DocState = SavedUnchangedFile;
        upRes->m_FileName = filePath.filename().string();
        upRes->m_FilePathString = filePath.string();
        upRes->m_FilePath = filePath;
        upRes->m_FileStream = std::move(fileStream);
        return true;
    }
    else
    {
        globalLogger().info(std::format("Open file {} failed!", filePath.string()));
        return false;
    }
}

// create a unnamed new document, return true if success, save result to upRes
bool Document::createUnnamedFile(std::unique_ptr<Document>& upRes)
{
    std::unique_ptr<Document> up(new BinaryDocument());
    upRes = std::move(up);
    return true;
}