#pragma once

#include <memory>
#include "command/Command.h"

namespace tch {

// 命令基类
class Command;

// 命令管理器类
class CommandManager {
private:
    // 静态实例
    static std::unique_ptr<CommandManager> s_instance;
    
    // 活动命令
    std::unique_ptr<Command> m_activeCommand;
    
public:
    // 构造函数
    CommandManager();
    
    // 获取单例实例
    static CommandManager& getInstance();
    
    // 执行命令
    void executeCommand(std::unique_ptr<Command> command);
    
    // 取消当前命令
    void cancelCurrentCommand();
    
    // 检查是否有活动命令
    bool hasActiveCommand();
    
    // 更新活动命令
    void updateActiveCommand();
    
    // 运行命令循环
    void runCommandLoop();
};

} // namespace tch
