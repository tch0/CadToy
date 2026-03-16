#include "command/CommandManager.h"
#include "input/InputContext.h"
#include "command/CommandLine.h"
#include "command/CommandClose.h"
#include "debug/Logger.h"
#include "Utils/GlobalUtils.h"
#include "render/Renderer.h"

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
    
    // 设置输入上下文为命令执行状态，成功解析为了命令那才设置
    InputContext::getInstance().setInCommandExecution(m_activeCommand != nullptr);
}

// 取消当前执行的命令，用于比如文档切换、关闭文档、快捷键命令等需要先取消命令的场景
void CommandManager::cancelCurrentCommand()
{
    InputContext& inputContext = InputContext::getInstance();
    // 如果有任务正在执行，先取消任务(比如选择任务)，通常来说一次取消已经足够
    if (inputContext.isAnyCommandOrTaskRunning())
    {
        std::string input = Renderer::getAndClearCommandBuffer();
        inputContext.handleEscape(input);
    }
    
    // 没有任何命令在执行、或者有命令但已经执行完毕
    if (m_activeCommand == nullptr || (m_activeCommand != nullptr && m_activeCommand->isCompleted()))
    {
        m_activeCommand = nullptr;
        std::string input = Renderer::getAndClearCommandBuffer();
        if (!input.empty())
        {
            inputContext.handleEscape(input);
        }
    }
    // 有命令正在执行且没有执行完毕
    else
    {
        // 和Esc的行为一样，需要清空缓冲区
        std::string input = Renderer::getAndClearCommandBuffer();
        // 通过调用handleEscape模拟Esc的行为来取消，以正确处理提示
        inputContext.handleEscape(input);
        // 通过至多三次取消来取消当前执行的命令，一般来说无论什么命令处于哪个分支，三次取消都应该能够结束了
        for (int i = 0; i < 3; i++)
        {
            // 多调用几次以确保命令切实执行到了等待输入的状态，而不是在可以连续执行的不需要等待输入的状态之间输出多个提示
            // onUpdate也本身都是可重复调用的，在等待输入的时候多次调用几乎没有代价
            m_activeCommand->onUpdate();
            m_activeCommand->onUpdate();
            m_activeCommand->onUpdate();
            m_activeCommand->onUpdate();
            m_activeCommand->onUpdate();
            if (!m_activeCommand->isCompleted())
            {
                inputContext.handleEscape("");
            }
        }
        // 如果取消三次还没有结束，那么就再取消三次，如果六次取消还未结束，那么命令流程大概率出BUG了，就强制结束命令(直接析构掉命令对象)
        if (!m_activeCommand->isCompleted())
        {
            LOG_WARNING("The current command has not ended after 3 cancellations. "
                "Please check if the command logic is correct and the command flow is necessary.");
            for (int i = 0; i < 3 && !m_activeCommand->isCompleted(); i++)
            {
                inputContext.handleEscape("");
                // 多调用几次以确保命令切实执行到了等待输入的状态，而不是在可以连续执行的不需要等待输入的状态之间输出多个提示
                m_activeCommand->onUpdate();
                m_activeCommand->onUpdate();
                m_activeCommand->onUpdate();
                m_activeCommand->onUpdate();
                m_activeCommand->onUpdate();
            }
            if (!m_activeCommand->isCompleted())
            {
                LOG_ERROR("The current command has not ended after 6 cancellations. "
                    "Please check if the command logic is stuck in an infinite loop. The command will now be forcibly terminated.");
            }
        }
        // 置空当前命令
        m_activeCommand = nullptr;
        
        // 重置输入上下文为无命令执行状态
        inputContext.setInCommandExecution(false);
        
        // 最后再输出一个空行
        inputContext.handleEnterSpace("");
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
    // 首先更新输入上下文
    InputContext::getInstance().onUpdate();
    
    // 更新活动命令
    if (m_activeCommand) {
        // 更新命令
        //      虽然这里每一帧都会进入一次，不缺少调用次数，但是一般命令里的设置输入信息和获取输入是被切割为两个状态，
        //      还有命令流程结束后也需要一个状态来完成，我们希望在一帧内执行足够多的流程的话就调用两次就足够了。
        //      执行时大多数命令状态都是在等待输入只执行查询输入上下文的操作，没什么开销，原则上调几次对性能和结果没有也不应该有任何影响。
        m_activeCommand->onUpdate();
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
