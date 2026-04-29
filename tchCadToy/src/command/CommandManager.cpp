// 对应头文件
#include "CommandManager.h"

// C++ 标准库
#include <algorithm>

// 第三方库
#include <imgui_internal.h>

// 项目头文件
#include "CommandTest.h"
#include "CommandNew.h"
#include "CommandOpen.h"
#include "CommandClose.h"
#include "CommandQuit.h"
#include "CommandSave.h"
#include "CommandSaveAs.h"
#include "CommandLine.h"
#include "CommandRegen.h"
#include "CommandOptions.h"
#include "CommandProperties.h"
#include "CommandUndo.h"
#include "CommandU.h"
#include "CommandRedo.h"
#include "CommandMRedo.h"
#include "CommandCircle.h"
#include "Logger.h"
#include "DocManager.h"
#include "InputContext.h"
#include "Renderer.h"
#include "GlobalUtils.h"
#include "StringUtils.h"
#include "LocalizationManager.h"
#include "UndoManager.h"

namespace tch {

// 构造函数
CommandManager::CommandManager() :
    m_activeCommand(nullptr) {
    // 注册命令
    // 测试命令
    registerCommand<CommandTest>("TEST", {});
    // 打开保存关闭类命令
    registerCommand<CommandNew>("NEW", {});
    registerCommand<CommandOpen>("OPEN", {});
    registerCommand<CommandClose>("CLOSE", {});
    registerCommand<CommandSave>("SAVE", {});
    registerCommand<CommandSaveAs>("SAVEAS", {});
    registerCommand<CommandQuit>("QUIT", {"EXIT"});
    // 实体创建类命令
    registerCommand<CommandLine>("LINE", {"L"});
    registerCommand<CommandCircle>("CIRCLE", {"C"});
    // 实体编辑类命令
    
    // 重生成类命令
    registerCommand<CommandRegen>("REGEN", {"RE"});
    
    // 标注类命令
    
    // 配置类命令
    registerCommand<CommandOptions>("OPTIONS", {"OP"});
    registerCommand<CommandPropertiesShow>("PROPERTIES", {"PR"});
    registerCommand<CommandPropertiesClose>("PROPERTIESCLOSE", {"PRCLOSE"});
    // undo/redo类命令
    registerCommand<CommandUndo>("UNDO", {});
    registerCommand<CommandU>("U", {});
    registerCommand<CommandRedo>("REDO", {});
    registerCommand<CommandMRedo>("MREDO", {});
    
    // 未实现命令
    // registerCommand<CommandCircle>("CIRCLE", {"C"});
    // registerCommand<CommandRect>("RECTANG", {"REC"});
    // registerCommand<CommandArc>("ARC", {"A"});
    // registerCommand<CommandEllipse>("ELLIPSE", {"EL"});
    // registerCommand<CommandErase>("ERASE", {"E"});
    // registerCommand<CommandMove>("MOVE", {"M"});
    // registerCommand<CommandCopy>("COPY", {"CO", "CP"});
    // registerCommand<CommandRotate>("ROTATE", {"RO"});
    // registerCommand<CommandScale>("SCALE", {"SC"});
    // registerCommand<CommandZoom>("ZOOM", {"Z"});
    // registerCommand<CommandPan>("PAN", {"P"});
    // registerCommand<CommandLayer>("LAYER", {"LA"});
    
    // 所有命令注册完成后，建立补全候选池
    rebuildCommandCompletionPool();
}

// 获取单例实例
CommandManager& CommandManager::getInstance() {
    static CommandManager instance;
    return instance;
}

// 初始化命令管理器，保存窗口指针并注册关闭回调
void CommandManager::initialize(GLFWwindow* window) {
    m_window = window;
    glfwSetWindowCloseCallback(window, windowCloseCallback);
}

// 窗口关闭回调函数（静态，供GLFW调用）
void CommandManager::windowCloseCallback(GLFWwindow* window) {
    // 取消关闭请求，由QUIT命令控制实际关闭
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    
    CommandManager& instance = getInstance();
    // 正在执行QUIT命令，不再重复执行
    if (instance.m_currentCommandName == "QUIT") {
        return;
    }
    
    // 如果有模态对话框打开，不执行QUIT，等待用户手动关闭，这符合Windows原生窗口的逻辑，GLFW窗口配合imgui对话框则需要自行实现这个逻辑
    if (ImGui::GetTopMostPopupModal() != nullptr ||
        ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
        return;
    }
    
    // 取消当前命令并执行QUIT命令
    instance.cancelCurrentCommandAndExecute("QUIT");
}

// 请求退出应用程序（由QUIT命令调用，设置窗口关闭标志）
void CommandManager::requestQuitApplication() {
    if (m_window) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

// 注册命令模板实现
template<typename T>
void CommandManager::registerCommand(const std::string& fullName, const std::vector<std::string>& aliases) {
    std::string upperFull = StringUtils::toUpperCase(fullName);
    m_creators[upperFull] = []() {
        return std::make_unique<T>();
    };
    
    for (const auto& alias : aliases) {
        m_aliasMap[StringUtils::toUpperCase(alias)] = upperFull;
    }
}

// 重建补全候选池
void CommandManager::rebuildCommandCompletionPool() {
    m_commandCompletionPool.clear();
    
    // 添加全称
    for (const auto& [name, _] : m_creators) {
        m_commandCompletionPool.push_back({name, name, false});
    }
    
    // 添加别名
    for (const auto& [alias, full] : m_aliasMap) {
        m_commandCompletionPool.push_back({alias, full, true});
    }
    
    // 按照优先级从大到小排序
    std::sort(m_commandCompletionPool.begin(), m_commandCompletionPool.end());
}

// 获取匹配的补全候选列表，按照优先级排序
std::vector<CommandCompletionItem> CommandManager::getCompletionCandidates(const std::string& partial) const {
    std::vector<CommandCompletionItem> results;
    if (partial.empty()) {
        return results;
    }
    
    std::string search = StringUtils::toUpperCase(partial);
    for (const auto& item : m_commandCompletionPool) {
        int cmp = item.key.compare(0, search.size(), search);
        // 前缀匹配，建立命令补全候选池时就是排好序的，直接按顺序添加结果就是排好序的
        if (cmp == 0) {
            results.push_back(item);
        }
        // CommandCompletionItem按照别名优先、然后比较key的方式排序，补全池中顺序是：所有别名按key递增排序、所有全名按key递增排序，
        // 所以要保证所有别名已经比较完、后续都是全名时，才能确保已经比较了所有可能的匹配项、后续肯定不匹配，此时提前退出。
        else if (!item.isAlias && cmp > 0) {
            break;
        }
    }
    return results;
}

// 查找命令全名
std::string CommandManager::findFullCommandName(const std::string& command) const {
    std::string upCommand = StringUtils::toUpperCase(command);
    auto it = m_aliasMap.find(upCommand);
    return (it != m_aliasMap.end()) ? it->second : upCommand;
}

// 检查是否有活动命令
bool CommandManager::hasActiveCommand() {
    return m_activeCommand != nullptr;
}

// 执行命令
void CommandManager::executeCommand(const std::string& command) {
    tchAssert(m_activeCommand == nullptr, "There should be no active commnd when executing a new command. Please use cancelCurrentCommandAndExecute.");
    
    // 输出当前执行的命令的提示
    auto& loc = LocalizationManager::getInstance();
    Utils::cmdLinePrint(loc.get("commandLine.prompt.command") + " " + command);
    
    // 查找命令全名，创建并设置命令对象，记录命令名
    std::string cmdFullName = findFullCommandName(command);
    auto it = m_creators.find(cmdFullName);
    if (it != m_creators.end()) {
        m_activeCommand = it->second();
        m_currentCommandName = cmdFullName;
        
        DocManager::getCurrentDocument().addToCommandExecutionHistory(cmdFullName);
        
        // 只有不跳过 undo 记录的命令才创建 undo 组
        if (!m_activeCommand->skipUndoRecording()) {
            UndoManager::getInstance().beginGroup(cmdFullName);
        }
    } else {
        Utils::cmdLinePrint(StringUtils::format(loc.get("commandLine.prompt.unknownCommand"), command)); // 未知命令: "{}"，按F1查看帮助。
    }
    
    // 设置输入上下文为命令执行状态，成功解析为了命令那才设置
    InputContext::getInstance().setInCommandExecution(m_activeCommand != nullptr);
}

// 取消当前执行的命令，用于比如文档切换、关闭文档、快捷键命令等需要先取消命令的场景
void CommandManager::cancelCurrentCommand()
{
    InputContext& inputContext = InputContext::getInstance();
    // 没有任何命令在执行、或者有命令但已经执行完毕
    if (m_activeCommand == nullptr || (m_activeCommand != nullptr && m_activeCommand->isCompleted()))
    {
        m_activeCommand = nullptr;
        std::string input = Renderer::getAndClearCommandBuffer();
        if (!input.empty() || inputContext.isAnyCommandOrTaskRunning())
        {
            // 如果没有命令但却有任务在执行，那么一次Esc应该足以取消任务
            // 如果后续实现了深分支的无法通过一次Esc取消的任务，那么这里需要对应修改
            inputContext.handleEscape(input);
            inputContext.onUpdate();
            inputContext.onUpdate();
            inputContext.onUpdate();
            inputContext.onUpdate();
            inputContext.onUpdate();
        }
    }
    // 有命令正在执行且没有执行完毕
    else
    {
        // 和Esc的行为一样，需要清空缓冲区
        std::string input = Renderer::getAndClearCommandBuffer();
        // 通过调用handleEscape模拟Esc的行为来取消
        inputContext.handleEscape(input);
        // 通过至多3次取消来取消当前执行的命令，一般来说无论什么命令处于哪个分支，3次取消都应该能够结束了
        for (int i = 0; i < 3; i++)
        {
            // 多调用几次以确保命令切实执行到了等待输入的状态，而不是在可以连续执行的不需要等待输入的状态之间输出多个提示
            // onUpdate本身都是可重复调用的，在等待输入的状态时多次调用几乎没有任何代价
            inputContext.onUpdate();
            inputContext.onUpdate();
            inputContext.onUpdate();
            inputContext.onUpdate();
            inputContext.onUpdate();
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
        // 如果取消3次还没有结束，那么就再尝试取消3次，如果6次取消还未结束，那么命令流程大概率出BUG了，就强制结束命令(直接析构掉命令对象)
        if (!m_activeCommand->isCompleted())
        {
            LOG_WARNING("The current command has not ended after 3 cancellations. "
                "Please check if the command logic is correct and the command flow is necessary.");
            for (int i = 0; i < 3 && !m_activeCommand->isCompleted(); i++)
            {
                inputContext.handleEscape("");
                inputContext.onUpdate();
                inputContext.onUpdate();
                inputContext.onUpdate();
                inputContext.onUpdate();
                inputContext.onUpdate();
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
        // 置空当前命令前，先结束 undo 组（如果需要）
        if (!m_activeCommand->skipUndoRecording()) {
            UndoManager::getInstance().endGroup();
        }
        
        // 置空当前命令
        m_activeCommand = nullptr;
        m_currentCommandName = "";
        
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

void CommandManager::executeLastCommand() {
    // 获取当前文档的命令执行历史
    auto& history = DocManager::getCurrentDocument().getCommandExecutionHistory();

    // 如果历史不为空，执行最后一条命令；否则执行HELP
    if (!history.empty()) {
        executeCommand(history.back());
    } else {
        executeCommand("HELP");
    }
}

// 运行命令循环
void CommandManager::runCommandLoop() {
    // 首先更新输入上下文
    InputContext::getInstance().onUpdate();
    
    // 更新活动命令
    if (m_activeCommand) {
        // 更新命令
        //      这里只应该调用一次onUpdate，对于没有对话框的命令来说，多次调用不影响结果，
        //      但对于有对话框或者窗口显示的命令来说调用多次就会出现问题。
        m_activeCommand->onUpdate();
        
        // 检查命令是否完成
        if (m_activeCommand->isCompleted()) {
            // 置空当前命令前，先结束 undo 组（如果需要）
            if (!m_activeCommand->skipUndoRecording()) {
                UndoManager::getInstance().endGroup();
            }
            
            // 完成命令后清空当前命令
            m_activeCommand = nullptr;
            m_currentCommandName = "";
            
            // 重置输入上下文
            InputContext::getInstance().setInCommandExecution(false);
        }
    }
}

std::string CommandManager::getRunningCommandName() {
    return m_currentCommandName;
}

} // namespace tch
