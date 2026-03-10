#include "command/CommandManager.h"
#include "input/InputContext.h"
#include "command/CommandLine.h"
#include "command/CommandClose.h"
#include "debug/Logger.h"
#include "Utils/GlobalUtils.h"

namespace tch {

// 静态成员初始化
std::shared_ptr<CommandManager> CommandManager::s_instance = nullptr;

// 构造函数
CommandManager::CommandManager() :
    m_activeCommand(nullptr) {
}

// 获取单例实例
CommandManager& CommandManager::getInstance() {
    if (s_instance == nullptr) {
        s_instance = std::make_shared<CommandManager>();
    }
    return *s_instance;
}

// 检查是否有活动命令
bool CommandManager::hasActiveCommand() {
    return m_activeCommand != nullptr;
}

// 获取活动命令（用于预览）
std::shared_ptr<Command> CommandManager::getActiveCommand() {
    return m_activeCommand;
}


// 执行命令
void CommandManager::executeCommand(const std::string& command) {
    tchAssert(m_activeCommand == nullptr, "There should be no active commnd when executing a new command. Please use cancelCurrentCommandAndExecute.");
    
    parseCommand(command);
    
    // 设置输入上下文为命令执行状态
    InputContext::getInstance().setInCommandExecution(m_activeCommand != nullptr);
}

// 取消当前执行的命令，用于比如文档切换等场景
void CommandManager::cancelCurrentCommand()
{
    if (m_activeCommand)
    {
        InputContext::getInstance().setInCommandCancelProcess(true);
        // 通过至多三次取消来取消当前执行的命令，一般来说无论什么命令处于哪个分支，三次取消都应该能够结束了
        for (int i = 0; i < 3 && !m_activeCommand->isCompleted(); i++)
        {
            m_activeCommand->onUpdate();
        }
        // 如果取消三次还没有结束，那么就再取消三次，如果六次取消还未结束，那么命令流程大概率出BUG了，就强制结束命令(直接析构掉命令对象)
        if (!m_activeCommand->isCompleted())
        {
            LOG_WARNING("The current command has not ended after 3 cancellations. "
                "Please check if the command logic is correct and the command flow is necessary.");
            for (int i = 0; i < 3 && !m_activeCommand->isCompleted(); i++)
            {
                m_activeCommand->onUpdate();
            }
            if (!m_activeCommand->isCompleted())
            {
                LOG_ERROR("The current command has not ended after 6 cancellations. "
                    "Please check if the command logic is stuck in an infinite loop. The command will now be forcibly terminated.");
            }
        }
        m_activeCommand = nullptr;
        
        // 重置输入上下文状态
        InputContext::getInstance().setInCommandCancelProcess(false);
        InputContext::getInstance().setInCommandExecution(false);
    }
}

// 取消当前正在执行的命令并执行新命令
void CommandManager::cancelCurrentCommandAndExecute(const std::string& command)
{
    cancelCurrentCommand();
    executeCommand(command);
}

// 解析命令
void CommandManager::parseCommand(const std::string& command) {
    // 简单的命令解析
    if (command == "line" || command == "l") {
        // 创建线段命令并添加到待执行列表
        m_activeCommand = std::make_shared<CommandLine>();
    }
    else if (command == "close") {
        // 创建关闭命令并添加到待执行列表
        m_activeCommand = std::make_shared<CommandClose>();
    }
    // 其他命令的解析...
    else {
        cmdLinePrint(std::format("Unknown command: {}", command));
    }
}

// 运行命令循环
void CommandManager::runCommandLoop() {
    // 更新活动命令
    if (m_activeCommand) {
        // 更新命令
        m_activeCommand->onUpdate();
        
        // 检查命令是否完成
        if (m_activeCommand->isCompleted()) {
            // 完成命令后清空当前命令
            m_activeCommand = nullptr;
            
            // 重置输入上下文
            InputContext::getInstance().setInCommandExecution(false);
        }
    }
}

} // namespace tch
