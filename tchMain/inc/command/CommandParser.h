#pragma once
#include <string>
#include <vector>

namespace tch {

// 命令解析器
class CommandParser {
public:
    // 解析命令
    static bool parseCommand(const std::string& command);
    
    // 执行命令
    static bool executeCommand(const std::string& commandName, const std::vector<std::string>& arguments);
    
    // 显示帮助信息
    static void showHelp();

private:
    // 分割命令行参数
    static std::vector<std::string> splitArguments(const std::string& command);
    
    // 执行绘制直线命令
    static bool executeLineCommand(const std::vector<std::string>& arguments);
    
    // 执行绘制圆命令
    static bool executeCircleCommand(const std::vector<std::string>& arguments);
    
    // 执行绘制矩形命令
    static bool executeRectCommand(const std::vector<std::string>& arguments);
    
    // 执行平移命令
    static bool executeTranslateCommand(const std::vector<std::string>& arguments);
    
    // 执行旋转命令
    static bool executeRotateCommand(const std::vector<std::string>& arguments);
    
    // 执行缩放命令
    static bool executeScaleCommand(const std::vector<std::string>& arguments);
    
    // 执行创建图层命令
    static bool executeLayerCommand(const std::vector<std::string>& arguments);
    
    // 执行删除图层命令
    static bool executeDeleteLayerCommand(const std::vector<std::string>& arguments);
    
    // 执行切换图层命令
    static bool executeSwitchLayerCommand(const std::vector<std::string>& arguments);
    
    // 执行设置颜色命令
    static bool executeColorCommand(const std::vector<std::string>& arguments);
    
    // 执行撤销命令
    static bool executeUndoCommand(const std::vector<std::string>& arguments);
    
    // 执行重做命令
    static bool executeRedoCommand(const std::vector<std::string>& arguments);
    
    // 执行保存命令
    static bool executeSaveCommand(const std::vector<std::string>& arguments);
    
    // 执行加载命令
    static bool executeLoadCommand(const std::vector<std::string>& arguments);
    
    // 执行退出命令
    static bool executeExitCommand(const std::vector<std::string>& arguments);
    
    // 执行属性栏命令
    static bool executePropertiesCommand(const std::vector<std::string>& arguments);
    
    // 执行关闭属性栏命令
    static bool executePropertiesCloseCommand(const std::vector<std::string>& arguments);
    
    // 执行选项命令
    static bool executeOptionsCommand(const std::vector<std::string>& arguments);
    
    // 执行帮助命令
    static bool executeHelpCommand(const std::vector<std::string>& arguments);
};

} // namespace tch