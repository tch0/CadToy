#pragma once

#include <memory>
#include <string>
#include "command/Command.h"

namespace tch {

// 命令管理器类
class CommandManager {
private:
    // 静态实例
    static std::shared_ptr<CommandManager> s_instance;
    
    // 活动命令
    std::shared_ptr<Command> m_activeCommand;

public:
    // 构造函数
    CommandManager();
    
    // 获取单例实例
    static CommandManager& getInstance();
    
    // 检查是否有活动命令
    bool hasActiveCommand();
    
    // 获取活动命令（用于预览）
    std::shared_ptr<Command> getActiveCommand();
    
    // 执行命令
    void executeCommand(const std::string& command);
    
    // 取消当前执行的命令，用于比如文档切换等场景
    void cancelCurrentCommand();
    
    // 取消当前正在执行的命令并执行新命令
    void cancelCurrentCommandAndExecute(const std::string& command);
    
    // 解析命令
    void parseCommand(const std::string& command);
    
    // 运行命令循环
    void runCommandLoop();
};

} // namespace tch
