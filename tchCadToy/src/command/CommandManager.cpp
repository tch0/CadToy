#include "command/CommandManager.h"
#include "input/InputContext.h"

namespace tch {

// 静态成员初始化
std::unique_ptr<CommandManager> CommandManager::s_instance = nullptr;

// 私有构造函数
CommandManager::CommandManager() :
    m_activeCommand(nullptr) {
}

// 获取单例实例
CommandManager& CommandManager::getInstance() {
    if (s_instance == nullptr) {
        s_instance = std::make_unique<CommandManager>();
    }
    return *s_instance;
}

// 执行命令
void CommandManager::executeCommand(std::unique_ptr<Command> command) {
    // 取消当前命令（如果有）
    cancelCurrentCommand();
    
    // 设置新命令
    m_activeCommand = std::move(command);
    
    // 设置输入上下文为命令执行状态
    InputContext::getInstance().setInCommandExecution(true);
}

// 取消当前命令
void CommandManager::cancelCurrentCommand() {
    if (m_activeCommand) {
        m_activeCommand->cancel();
        m_activeCommand.reset();
        
        // 重置输入上下文
        InputContext::getInstance().setInCommandExecution(false);
        InputContext::getInstance().resetStatus();
    }
}

// 检查是否有活动命令
bool CommandManager::hasActiveCommand() {
    return m_activeCommand != nullptr;
}

// 更新活动命令
void CommandManager::updateActiveCommand() {
    if (m_activeCommand) {
        // 检查是否需要强制中止命令
        if (InputContext::getInstance().shouldAbortCommand()) {
            cancelCurrentCommand();
            return;
        }
        
        m_activeCommand->onUpdate();
        
        // 检查命令是否完成
        if (m_activeCommand->isCompleted()) {
            m_activeCommand.reset();
            
            // 重置输入上下文
            InputContext::getInstance().setInCommandExecution(false);
        }
    }
}

// 运行命令循环
void CommandManager::runCommandLoop() {
    if (hasActiveCommand()) {
        updateActiveCommand();
    }
}

} // namespace tch
