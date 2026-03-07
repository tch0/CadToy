#pragma once

#include <memory>
#include <vector>
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
    
    // 待执行的命令列表
    std::vector<std::shared_ptr<Command>> m_pendingCommands;

public:
    // 构造函数
    CommandManager();
    
    // 获取单例实例
    static CommandManager& getInstance();
    
    // 执行命令
    void executeCommand(std::shared_ptr<Command> command);
    
    // 检查是否有活动命令
    bool hasActiveCommand();
    
    // 获取活动命令（用于预览）
    std::shared_ptr<Command> getActiveCommand();
    
    // 解析命令
    void parseCommand(const std::string& command);
    
    // 运行命令循环
    void runCommandLoop();
};

} // namespace tch
