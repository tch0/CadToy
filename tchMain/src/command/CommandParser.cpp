#include "command/CommandParser.h"
#include "Geometry.h"
#include "Layer.h"
#include "Transform.h"
#include "Color.h"
#include "UndoRedo.h"
#include "SaveLoad.h"
#include <iostream>
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
        std::cout << "Unknown command: " << commandName << std::endl;
        return false;
    }
}

// 执行绘制直线命令
bool CommandParser::executeLineCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 4) {
        std::cout << "Usage: LINE X1 Y1 X2 Y2" << std::endl;
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
            
            std::cout << "Line drawn from (" << x1 << ", " << y1 << ") to (" << x2 << ", " << y2 << ")" << std::endl;
            return true;
        }
    } catch (...) {
        std::cout << "Invalid arguments for LINE command" << std::endl;
    }
    
    return false;
}

// 执行绘制圆命令
bool CommandParser::executeCircleCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3) {
        std::cout << "Usage: CIRCLE X Y RADIUS" << std::endl;
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
            
            std::cout << "Circle drawn at (" << x << ", " << y << ") with radius " << radius << std::endl;
            return true;
        }
    } catch (...) {
        std::cout << "Invalid arguments for CIRCLE command" << std::endl;
    }
    
    return false;
}

// 执行绘制矩形命令
bool CommandParser::executeRectCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 4) {
        std::cout << "Usage: RECT X Y WIDTH HEIGHT" << std::endl;
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
            
            std::cout << "Rectangle drawn at (" << x << ", " << y << ") with width " << width << " and height " << height << std::endl;
            return true;
        }
    } catch (...) {
        std::cout << "Invalid arguments for RECT command" << std::endl;
    }
    
    return false;
}

// 执行平移命令
bool CommandParser::executeTranslateCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 2) {
        std::cout << "Usage: TRANSLATE X Y" << std::endl;
        return false;
    }
    
    try {
        float x = std::stof(arguments[0]);
        float y = std::stof(arguments[1]);
        glm::vec2 delta(x, y);
        
        // 这里简化处理，实际应该获取选中的图形
        // 这里只是示例，实际实现需要选中图形的逻辑
        std::cout << "Translate by (" << x << ", " << y << ")" << std::endl;
        return true;
    } catch (...) {
        std::cout << "Invalid arguments for TRANSLATE command" << std::endl;
    }
    
    return false;
}

// 执行旋转命令
bool CommandParser::executeRotateCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3) {
        std::cout << "Usage: ROTATE X Y ANGLE" << std::endl;
        return false;
    }
    
    try {
        float x = std::stof(arguments[0]);
        float y = std::stof(arguments[1]);
        float angle = std::stof(arguments[2]);
        
        // 这里简化处理，实际应该获取选中的图形
        std::cout << "Rotate by " << angle << " degrees around (" << x << ", " << y << ")" << std::endl;
        return true;
    } catch (...) {
        std::cout << "Invalid arguments for ROTATE command" << std::endl;
    }
    
    return false;
}

// 执行缩放命令
bool CommandParser::executeScaleCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3) {
        std::cout << "Usage: SCALE X Y SCALE_FACTOR" << std::endl;
        return false;
    }
    
    try {
        float x = std::stof(arguments[0]);
        float y = std::stof(arguments[1]);
        float factor = std::stof(arguments[2]);
        
        // 这里简化处理，实际应该获取选中的图形
        std::cout << "Scale by " << factor << " around (" << x << ", " << y << ")" << std::endl;
        return true;
    } catch (...) {
        std::cout << "Invalid arguments for SCALE command" << std::endl;
    }
    
    return false;
}

// 执行创建图层命令
bool CommandParser::executeLayerCommand(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        std::cout << "Usage: LAYER NAME" << std::endl;
        return false;
    }
    
    try {
        std::string name = arguments[0];
        
        // 创建图层
        int layerId = LayerManager::getInstance().createLayer(name);
        std::cout << "Layer created: " << name << " (ID: " << layerId << ")" << std::endl;
        return true;
    } catch (...) {
        std::cout << "Invalid arguments for LAYER command" << std::endl;
    }
    
    return false;
}

// 执行删除图层命令
bool CommandParser::executeDeleteLayerCommand(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        std::cout << "Usage: DELETE_LAYER NAME" << std::endl;
        return false;
    }
    
    try {
        std::string name = arguments[0];
        
        // 获取图层
        auto layer = LayerManager::getInstance().getLayer(name);
        if (layer) {
            // 删除图层
            LayerManager::getInstance().deleteLayer(layer->getId());
            std::cout << "Layer deleted: " << name << std::endl;
            return true;
        } else {
            std::cout << "Layer not found: " << name << std::endl;
        }
    } catch (...) {
        std::cout << "Invalid arguments for DELETE_LAYER command" << std::endl;
    }
    
    return false;
}

// 执行切换图层命令
bool CommandParser::executeSwitchLayerCommand(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        std::cout << "Usage: SWITCH_LAYER NAME" << std::endl;
        return false;
    }
    
    try {
        std::string name = arguments[0];
        
        // 获取图层
        auto layer = LayerManager::getInstance().getLayer(name);
        if (layer) {
            // 切换图层
            LayerManager::getInstance().setCurrentLayer(layer->getId());
            std::cout << "Switched to layer: " << name << std::endl;
            return true;
        } else {
            std::cout << "Layer not found: " << name << std::endl;
        }
    } catch (...) {
        std::cout << "Invalid arguments for SWITCH_LAYER command" << std::endl;
    }
    
    return false;
}

// 执行设置颜色命令
bool CommandParser::executeColorCommand(const std::vector<std::string>& arguments) {
    if (arguments.size() != 3) {
        std::cout << "Usage: COLOR R G B" << std::endl;
        return false;
    }
    
    try {
        float r = std::stof(arguments[0]);
        float g = std::stof(arguments[1]);
        float b = std::stof(arguments[2]);
        
        // 这里简化处理，实际应该设置选中图形的颜色
        std::cout << "Set color to (" << r << ", " << g << ", " << b << ")" << std::endl;
        return true;
    } catch (...) {
        std::cout << "Invalid arguments for COLOR command" << std::endl;
    }
    
    return false;
}

// 执行撤销命令
bool CommandParser::executeUndoCommand(const std::vector<std::string>& arguments) {
    if (UndoRedoManager::getInstance().undo()) {
        std::cout << "Undo successful" << std::endl;
        return true;
    } else {
        std::cout << "Nothing to undo" << std::endl;
        return false;
    }
}

// 执行重做命令
bool CommandParser::executeRedoCommand(const std::vector<std::string>& arguments) {
    if (UndoRedoManager::getInstance().redo()) {
        std::cout << "Redo successful" << std::endl;
        return true;
    } else {
        std::cout << "Nothing to redo" << std::endl;
        return false;
    }
}

// 执行保存命令
bool CommandParser::executeSaveCommand(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        std::cout << "Usage: SAVE FILE_PATH" << std::endl;
        return false;
    }
    
    try {
        std::string filePath = arguments[0];
        
        if (SaveLoad::saveToFile(filePath)) {
            std::cout << "Saved to file: " << filePath << std::endl;
            return true;
        } else {
            std::cout << "Failed to save file: " << filePath << std::endl;
        }
    } catch (...) {
        std::cout << "Invalid arguments for SAVE command" << std::endl;
    }
    
    return false;
}

// 执行加载命令
bool CommandParser::executeLoadCommand(const std::vector<std::string>& arguments) {
    if (arguments.empty()) {
        std::cout << "Usage: LOAD FILE_PATH" << std::endl;
        return false;
    }
    
    try {
        std::string filePath = arguments[0];
        
        if (SaveLoad::loadFromFile(filePath)) {
            std::cout << "Loaded from file: " << filePath << std::endl;
            return true;
        } else {
            std::cout << "Failed to load file: " << filePath << std::endl;
        }
    } catch (...) {
        std::cout << "Invalid arguments for LOAD command" << std::endl;
    }
    
    return false;
}

// 执行退出命令
bool CommandParser::executeExitCommand(const std::vector<std::string>& arguments) {
    std::cout << "Exiting..." << std::endl;
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
    std::cout << "Available commands:" << std::endl;
    std::cout << "  LINE X1 Y1 X2 Y2        - Draw a line" << std::endl;
    std::cout << "  CIRCLE X Y RADIUS       - Draw a circle" << std::endl;
    std::cout << "  RECT X Y WIDTH HEIGHT   - Draw a rectangle" << std::endl;
    std::cout << "  TRANSLATE X Y           - Translate selected shapes" << std::endl;
    std::cout << "  ROTATE X Y ANGLE        - Rotate selected shapes" << std::endl;
    std::cout << "  SCALE X Y SCALE_FACTOR  - Scale selected shapes" << std::endl;
    std::cout << "  LAYER NAME              - Create a new layer" << std::endl;
    std::cout << "  DELETE_LAYER NAME       - Delete a layer" << std::endl;
    std::cout << "  SWITCH_LAYER NAME       - Switch to a layer" << std::endl;
    std::cout << "  COLOR R G B             - Set color" << std::endl;
    std::cout << "  UNDO                    - Undo last operation" << std::endl;
    std::cout << "  REDO                    - Redo last operation" << std::endl;
    std::cout << "  SAVE FILE_PATH          - Save to file" << std::endl;
    std::cout << "  LOAD FILE_PATH          - Load from file" << std::endl;
    std::cout << "  EXIT                    - Exit the program" << std::endl;
    std::cout << "  HELP                    - Show this help" << std::endl;
}

} // namespace tch