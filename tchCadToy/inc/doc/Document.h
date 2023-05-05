#pragma once
#include <string>
#include <fstream>
#include <filesystem>

class Document
{
public:
    enum DocumentState
    {
        UnnamedNewFile,         // new created unnamed document, not associated with any disk file, only exist in memory, nothing has been drawn
        UnnamedChangedFile,     // new created unnamed document, but something has been drawn
        SavedChangedFile,       // associate with a file on disk, newer than saved file on disk
        SavedUnchangedFile      // associate with a file on disk, newest to saved file on disk
    };
    Document();
    virtual ~Document();
    DocumentState documentState();
    void setDocumentState(DocumentState state);
    std::string fileName();
    std::string filePathString();
    void save();
    virtual void saveContent() = 0; // for inherited class to override
protected:
    DocumentState m_DocState {UnnamedNewFile};
    std::string m_FileName;
    std::string m_FilePathString;
    std::filesystem::path m_FilePath;
    std::fstream m_FileStream;
public:
    // open file on disk, return true if success, save result to upRes
    static bool openFile(const std::filesystem::path& filePath, std::unique_ptr<Document>& upRes);
    // create a unnamed new document, return true if success, save result to upRes
    static bool createUnnamedFile(std::unique_ptr<Document>& upRes);
};

