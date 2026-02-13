#include "command/CommandParser.h"
#include "Geometry.h"
#include "Layer.h"
#include "Transform.h"
#include "Color.h"
#include "UndoRedo.h"
#include "SaveLoad.h"
#include "utils/GlobalUtils.h"
#include <sstream>

namespace tch {

// 解析命令
bool CommandParser::parseCommand(const std::string& command) {
    // 分割命令行参数
    auto parts = splitArguments(command);
    if (parts.empty()) {
        return false;
    }
    
    // 获取命令名称
    std::string commandName = parts[0];
    
    // 转换为大写
    for (char& c : commandName) {
        c = toupper(c);
    }
    
    // 提取参数
    std::vector<std::string> arguments;
    for (size_t i = 1; i < parts.size(); ++i) {
        arguments.push_back(parts[i]);
    }
    
    // 执行命令
    return executeCommand(commandName, arguments);
}

// 分割命令行参数
std::vector<std::string> CommandParser::splitArguments(const std::string& command) {
    std::vector<std::string> parts;
    std::istringstream iss(command);
    std::string part;
    
    while (iss >> part) {
        parts.push_back(part);
    }
    
    return parts;
}

// 执行命令
bool CommandParser::executeCommand(const std::string& commandName, const std::vector<std::string>& arguments) {
    if (commandName == "LINE") {
        return executeLineCommand(arguments);
    } else if (commandName == "CIRCLE") {
        return executeCircleCommand(arguments);
    } else if (commandName == "RECT") {
        return executeRectCommand(arguments);
    } else if (commandName == "TRANSLATE") {
        return executeTranslateCommand(arguments);
    } else if (commandName == "ROTATE") {
        return executeRotateCommand(arguments);
    } else if (commandName == "SCALE") {
        return executeScaleCommand(arguments);
    } else if (commandName == "LAYER") {
        return executeLayerCommand(arguments);
    } else if (commandName == "DELETE_LAYER") {
        return executeDeleteLayerCommand(arguments);
    } else if (commandName == "SWITCH_LAYER") {
        return executeSwitchLayerCommand(arguments);
    } else if (commandName == "COLOR") {
        return executeColorCommand(arguments);
    } else if (commandName == "UNDO") {
        return executeUndoCommand(arguments);
    } else if (commandName == "REDO") {
        return executeRedoCommand(arguments);
    } else if (commandName == "SAVE") {
        return executeSaveCommand(arguments);
    } else if (commandName == "LOAD") {
        return executeLoadCommand(arguments);
    } else if (commandName == "EXIT") {
        return executeExitCommand(arguments);
    } else if (commandName == "HELP") {
        return executeHelpCommand(arguments);
    } else {
        tch::cmdPrint("Unknown command: " + commandName);
        return false;
    }
}

// 执行绘制直线命令
bool CommandParser::executeLineCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 4) {
        tch::cmdPrint("Usage: LINE X1 Y1 X2 Y2");
        return false;
    }
    
    try {
        float x1 = std::stof(arguments[0]);
        float y1 = std::stof(arguments[1]);
        float x2 = std::stof(arguments[2]);
        float y2 = std::stof(arguments[3]);
        
        // 创建直线
        auto line = std::make_shared<Line>(glm::vec2(x1, y1), glm::vec2(x2, y2));
        
        // 获取当前图层
        auto layer = LayerManager::getInstance().getCurrentLayer();
        if (layer) {
            // 添加到图层
            layer->addShape(line);
            
            // 添加到撤销栈
            UndoRedoManager::getInstance().addOperation(std::make_shared<DrawOperation>(line));
            
            tch::cmdPrint("Line drawn from (" + std::to_string(x1) + ", " + std::to_string(y1) + ") to (" + std::to_string(x2) + ", " + std::to_string(y2) + ")");
            return true;
        }
    } catch (...) {
        tch::cmdPrint("Invalid arguments for LINE command");
    }
    
    return false;
}

// 执行绘制圆命令
bool CommandParser::executeCircleCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3) {
        tch::cmdPrint("Usage: CIRCLE X Y RADIUS");
        return false;
    }
    
    try {
        float x = std::stof(arguments[0]);
        float y = std::stof(arguments[1]);
        float radius = std::stof(arguments[2]);
        
        // 创建圆
        auto circle = std::make_shared<Circle>(glm::vec2(x, y), radius);
        
        // 获取当前图层
        auto layer = LayerManager::getInstance().getCurrentLayer();
        if (layer) {
            // 添加到图层
            layer->addShape(circle);
            
            // 添加到撤销栈
            UndoRedoManager::getInstance().addOperation(std::make_shared<DrawOperation>(circle));
            
            tch::cmdPrint("Circle drawn at (" + std::to_string(x) + ", " + std::to_string(y) + ") with radius " + std::to_string(radius));
            return true;
        }
    } catch (...) {
        tch::cmdPrint("Invalid arguments for CIRCLE command");
    }
    
    return false;
}

// 执行绘制矩形命令
bool CommandParser::executeRectCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 4) {
        tch::cmdPrint("Usage: RECT X Y WIDTH HEIGHT");
        return false;
    }
    
    try {
        float x = std::stof(arguments[0]);
        float y = std::stof(arguments[1]);
        float width = std::stof(arguments[2]);
        float height = std::stof(arguments[3]);
        
        // 创建矩形
        auto rect = std::make_shared<Rectangle>(glm::vec2(x, y), width, height);
        
        // 获取当前图层
        auto layer = LayerManager::getInstance().getCurrentLayer();
        if (layer) {
            // 添加到图层
            layer->addShape(rect);
            
            // 添加到撤销栈
            UndoRedoManager::getInstance().addOperation(std::make_shared<DrawOperation>(rect));
            
            tch::cmdPrint("Rectangle drawn at (" + std::to_string(x) + ", " + std::to_string(y) + ") with width " + std::to_string(width) + " and height " + std::to_string(height));
            return true;
        }
    } catch (...) {
        tch::cmdPrint("Invalid arguments for RECT command");
    }
    
    return false;
}

// 执行平移命令
bool CommandParser::executeTranslateCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 2) {
        tch::cmdPrint("Usage: TRANSLATE X Y");
        return false;
    }
    
    try {
        float x = std::stof(arguments[0]);
        float y = std::stof(arguments[1]);
        glm::vec2 delta(x, y);
        
        // 这里简化处理，实际应该获取选中的图形
        // 这里只是示例，实际实现需要选中图形的逻辑
        tch::cmdPrint("Translate by (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        return true;
    } catch (...) {
        tch::cmdPrint("Invalid arguments for TRANSLATE command");
    }
    
    return false;
}

// 执行旋转命令
bool CommandParser::executeRotateCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3) {
        tch::cmdPrint("Usage: ROTATE X Y ANGLE");
        return false;
    }
    
    try {
        float x = std::stof(arguments[0]);
        float y = std::stof(arguments[1]);
        float angle = std::stof(arguments[2]);
        
        // 这里简化处理，实际应该获取选中的图形
        tch::cmdPrint("Rotate by " + std::to_string(angle) + " degrees around (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        return true;
    } catch (...) {
        tch::cmdPrint("Invalid arguments for ROTATE command");
    }
    
    return false;
}

// 执行缩放命令
bool CommandParser::executeScaleCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3) {
        tch::cmdPrint("Usage: SCALE X Y SCALE_FACTOR");
        return false;
    }
    
    try {
        float x = std::stof(arguments[0]);
        float y = std::stof(arguments[1]);
        float factor = std::stof(arguments[2]);
        
        // 这里简化处理，实际应该获取选中的图形
        tch::cmdPrint("Scale by " + std::to_string(factor) + " around (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        return true;
    } catch (...) {
        tch::cmdPrint("Invalid arguments for SCALE command");
    }
    
    return false;
}

// 执行创建图层命令
bool CommandParser::executeLayerCommand(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        tch::cmdPrint("Usage: LAYER NAME");
        return false;
    }
    
    try {
        std::string name = arguments[0];
        
        // 创建图层
        int layerId = LayerManager::getInstance().createLayer(name);
        tch::cmdPrint("Layer created: " + name + " (ID: " + std::to_string(layerId) + ")");
        return true;
    } catch (...) {
        tch::cmdPrint("Invalid arguments for LAYER command");
    }
    
    return false;
}

// 执行删除图层命令
bool CommandParser::executeDeleteLayerCommand(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        tch::cmdPrint("Usage: DELETE_LAYER NAME");
        return false;
    }
    
    try {
        std::string name = arguments[0];
        
        // 获取图层
        auto layer = LayerManager::getInstance().getLayer(name);
        if (layer) {
            // 删除图层
            LayerManager::getInstance().deleteLayer(layer->getId());
            tch::cmdPrint("Layer deleted: " + name);
            return true;
        } else {
            tch::cmdPrint("Layer not found: " + name);
        }
    } catch (...) {
        tch::cmdPrint("Invalid arguments for DELETE_LAYER command");
    }
    
    return false;
}

// 执行切换图层命令
bool CommandParser::executeSwitchLayerCommand(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        tch::cmdPrint("Usage: SWITCH_LAYER NAME");
        return false;
    }
    
    try {
        std::string name = arguments[0];
        
        // 获取图层
        auto layer = LayerManager::getInstance().getLayer(name);
        if (layer) {
            // 切换图层
            LayerManager::getInstance().setCurrentLayer(layer->getId());
            tch::cmdPrint("Switched to layer: " + name);
            return true;
        } else {
            tch::cmdPrint("Layer not found: " + name);
        }
    } catch (...) {
        tch::cmdPrint("Invalid arguments for SWITCH_LAYER command");
    }
    
    return false;
}

// 执行设置颜色命令
bool CommandParser::executeColorCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3) {
        tch::cmdPrint("Usage: COLOR R G B");
        return false;
    }
    
    try {
        float r = std::stof(arguments[0]);
        float g = std::stof(arguments[1]);
        float b = std::stof(arguments[2]);
        
        // 这里简化处理，实际应该设置选中图形的颜色
        tch::cmdPrint("Set color to (" + std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ")");
        return true;
    } catch (...) {
        tch::cmdPrint("Invalid arguments for COLOR command");
    }
    
    return false;
}

// 执行撤销命令
bool CommandParser::executeUndoCommand(const std::vector<std::string>& arguments) {
    if (UndoRedoManager::getInstance().undo()) {
        tch::cmdPrint("Undo successful");
        return true;
    } else {
        tch::cmdPrint("Nothing to undo");
        return false;
    }
}

// 执行重做命令
bool CommandParser::executeRedoCommand(const std::vector<std::string>& arguments) {
    if (UndoRedoManager::getInstance().redo()) {
        tch::cmdPrint("Redo successful");
        return true;
    } else {
        tch::cmdPrint("Nothing to redo");
        return false;
    }
}

// 执行保存命令
bool CommandParser::executeSaveCommand(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        tch::cmdPrint("Usage: SAVE FILE_PATH");
        return false;
    }
    
    try {
        std::string filePath = arguments[0];
        
        if (SaveLoad::saveToFile(filePath)) {
            tch::cmdPrint("Saved to file: " + filePath);
            return true;
        } else {
            tch::cmdPrint("Failed to save file: " + filePath);
        }
    } catch (...) {
        tch::cmdPrint("Invalid arguments for SAVE command");
    }
    
    return false;
}

// 执行加载命令
bool CommandParser::executeLoadCommand(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        tch::cmdPrint("Usage: LOAD FILE_PATH");
        return false;
    }
    
    try {
        std::string filePath = arguments[0];
        
        if (SaveLoad::loadFromFile(filePath)) {
            tch::cmdPrint("Loaded from file: " + filePath);
            return true;
        } else {
            tch::cmdPrint("Failed to load file: " + filePath);
        }
    } catch (...) {
        tch::cmdPrint("Invalid arguments for LOAD command");
    }
    
    return false;
}

// 执行退出命令
bool CommandParser::executeExitCommand(const std::vector<std::string>& arguments) {
    tch::cmdPrint("Exiting...");
    // 这里简化处理，实际应该清理资源并退出程序
    return true;
}

// 执行帮助命令
bool CommandParser::executeHelpCommand(const std::vector<std::string>& arguments) {
    showHelp();
    return true;
}

// 显示帮助信息
void CommandParser::showHelp() {
    tch::cmdPrint("Available commands:");
    tch::cmdPrint("  LINE X1 Y1 X2 Y2        - Draw a line");
    tch::cmdPrint("  CIRCLE X Y RADIUS       - Draw a circle");
    tch::cmdPrint("  RECT X Y WIDTH HEIGHT   - Draw a rectangle");
    tch::cmdPrint("  TRANSLATE X Y           - Translate selected shapes");
    tch::cmdPrint("  ROTATE X Y ANGLE        - Rotate selected shapes");
    tch::cmdPrint("  SCALE X Y SCALE_FACTOR  - Scale selected shapes");
    tch::cmdPrint("  LAYER NAME              - Create a new layer");
    tch::cmdPrint("  DELETE_LAYER NAME       - Delete a layer");
    tch::cmdPrint("  SWITCH_LAYER NAME       - Switch to a layer");
    tch::cmdPrint("  COLOR R G B             - Set color");
    tch::cmdPrint("  UNDO                    - Undo last operation");
    tch::cmdPrint("  REDO                    - Redo last operation");
    tch::cmdPrint("  SAVE FILE_PATH          - Save to file");
    tch::cmdPrint("  LOAD FILE_PATH          - Load from file");
    tch::cmdPrint("  EXIT                    - Exit the program");
    tch::cmdPrint("  HELP                    - Show this help");
}

} // namespace tch