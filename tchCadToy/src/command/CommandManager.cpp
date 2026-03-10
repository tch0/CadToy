#include <utility>

#include "command/CommandManager.h"
#include "input/InputContext.h"
#include "command/CommandLine.h"
#include "command/CommandClose.h"

namespace tch {

// 静态成员初始化
std::shared_ptr<CommandManager> CommandManager::s_instance = nullptr;

// 构造函数
CommandManager::CommandManager() :
    m_activeCommand(nullptr),
    m_pendingCommands() {
}

// 获取单例实例
CommandManager& CommandManager::getInstance() {
    if (s_instance == nullptr) {
        s_instance = std::make_shared<CommandManager>();
    }
    return *s_instance;
}

// 执行命令
void CommandManager::executeCommand(std::shared_ptr<Command> command) {
    // 取消当前命令（如果有）
    if (m_activeCommand) {
        m_activeCommand = nullptr;
        
        // 重置输入上下文
        InputContext::getInstance().setInCommandExecution(false);
        InputContext::getInstance().resetStatus();
    }
    
    // 设置新命令
    m_activeCommand = std::move(command);
    
    // 设置输入上下文为命令执行状态
    InputContext::getInstance().setInCommandExecution(true);
}



// 检查是否有活动命令
bool CommandManager::hasActiveCommand() {
    return m_activeCommand != nullptr;
}

// 获取活动命令（用于预览）
std::shared_ptr<Command> CommandManager::getActiveCommand() {
    return m_activeCommand;
}

// 解析命令
void CommandManager::parseCommand(const std::string& command) {
    // 简单的命令解析
    if (command == "line" || command == "l") {
        // 创建线段命令并添加到待执行列表
        m_pendingCommands.push_back(std::make_shared<CommandLine>());
    }
    else if (command == "close") {
        // 创建关闭命令并添加到待执行列表
        m_pendingCommands.push_back(std::make_shared<CommandClose>());
    }
    // 其他命令的解析...
}

// 运行命令循环
void CommandManager::runCommandLoop() {
    // 检查是否有待执行的命令
    if (!m_pendingCommands.empty() && m_activeCommand == nullptr) {
        // 执行第一个待执行的命令
        executeCommand(m_pendingCommands.front());
        m_pendingCommands.erase(m_pendingCommands.begin());
    }
    
    // 更新活动命令
    if (m_activeCommand) {
        // 检查是否需要强制中止命令
        if (InputContext::getInstance().shouldAbortCommand()) {
            m_activeCommand = nullptr;
            
            // 重置输入上下文
            InputContext::getInstance().setInCommandExecution(false);
            InputContext::getInstance().resetStatus();
            return;
        }
        
        m_activeCommand->onUpdate();
        
        // 检查命令是否完成
        if (m_activeCommand->isCompleted()) {
            m_activeCommand = nullptr;
            
            // 重置输入上下文
            InputContext::getInstance().setInCommandExecution(false);
        }
    }
}

} // namespace tch
