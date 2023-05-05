#pragma once
#include <memory>
#include <vector>
#include <filesystem>
#include <string>

#include <Document.h>

// will be a singleton
class DocManager
{
public:
    struct DocumentCanvasAttribute
    {
        // the canvas center point, in OpenGL 3D coordinate, but only X-Y coordinate, z is fixed (always 0) for now (aka the elevation).
        float canvasCenterX {0.0f};
        float canvasCenterY {0.0f};
        // the canvas scale factor, changed through mouse wheel, determines canvas OpenGL 3d Coordinates together with the canvas center point and screen width/height of canvas.
        // specifically, this factor is equal to the length of one pixel in 3D OpenGL coordinate.
        float canvasScaleFactor {0.1f};
        // OpenGL 3D coordinate of canvas, calculated by canvas center point & canvas scale factor & canvas screen coordinates
        float canvasLeft {0.0f};
        float canvasRight {0.0f};
        float canvasTop {0.0f};
        float canvasBottom {0.0f};
        // grid scale factor, the width of the main grid, should always be pow(5, n), n is integer
        float gridScaleFactor {5.0f};
        // A fixed parameter for grid auto-ajusting, change along with g_CanvasScaleFactor and g_GridScaleFactor.
        float gridAutoAjustFactor{4.98f};
    };
    struct DocumentCmdLineAttribute
    {
        DocumentCmdLineAttribute()
            : historyPos(-1)
            , bInCommandExecution(false)
        {
            inputBuffer.resize(1024);
        }

        // command logs
        std::vector<std::string> commandLogs;
        // command history
        std::vector<std::string> history;
        // history position, -1 for new line, 0 ~ history.size()-1 browsing history
        int historyPos {-1};
        // input buffer
        std::string inputBuffer;

        // the unprocessed input string that will be either pass to command (if the command is executing now) as inputs 
        //      or be parsed as next command (if pervious command do not need inputs, when pervious command finish).
        std::string unprocessedInput;
        // whether the program is current in the execution of a command:
        // specifically:
        //  - a command is excuting and waiting for text/point/... input.
        //  - the command pops up a modal dialog, and is waiting to process/close.
        //  - ... (other kind, to be added here)
        bool bInCommandExecution {false};
    };
private:
    std::vector<std::unique_ptr<Document>> m_Documents;
    std::vector<DocumentCanvasAttribute> m_DocCanvasAttrs;
    std::vector<DocumentCmdLineAttribute> m_DocCmdLineAttrs;
    std::size_t m_CurrentDocIndex {0};
public:
    DocManager();
    ~DocManager();
    // current document
    Document& currentDoc();
    // current document's canvas attributes
    DocumentCanvasAttribute& currentDocCanvasAttributes();
    // current document's command line window attrbutes
    DocumentCmdLineAttribute& currentDocCmdLineAttributes();
    // set current document index
    void setCurrentDocumentIndex(std::size_t index);
    // open a existing file, do some necessary initialization for this document, convert to this document, return true if sucess, do nothing and return false when fail.
    bool newNamedDocument(const std::filesystem::path& filePath);
    // create a unnamed new document, convert to this document, return true if sucess, do nothing and return false when fail.
    bool newUnnamedDocument();
};
