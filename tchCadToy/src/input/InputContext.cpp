#include "input/InputContext.h"
#include "render/Renderer.h"
#include "command/CommandManager.h"
#include "utils/LocalizationManager.h"
#include "utils/GlobalUtils.h"
#include "utils/StringUtils.h"
#include <glm/glm.hpp>
#include <memory>
#include <algorithm>
#include <imgui.h>

namespace tch {

// 静态实例
std::shared_ptr<InputContext> InputContext::s_instance = nullptr;

// 构造函数
InputContext::InputContext() :
    m_inCommandExecution(false),
    m_inCommandCancelProcess(false),
    m_currentStatus(InputStatus::kNone),
    m_allowedTypes(),
    m_prompt(""),
    m_pickedPoint(glm::dvec3(0, 0, 0)),
    m_inputInteger(0),
    m_inputFloat(0.0),
    m_inputString(""),
    m_inputKeyword(""),
    m_keywordOptions(),
    m_selectedEntities(),
    m_lastSpecialKeyEvent(SpecialKeyEventType::kNone),
    m_input(""),
    m_errorPrompt("") {
}

// 获取单例实例
InputContext& InputContext::getInstance() {
    if (s_instance == nullptr) {
        s_instance = std::make_shared<InputContext>();
    }
    return *s_instance;
}

// 命令执行状态管理，CommandManager通过这个接口来管理这个标记，命令开始执行时置为true，执行结束/取消执行后置回false
void InputContext::setInCommandExecution(bool inExecution) {
    m_inCommandExecution = inExecution;
    // 命令执行结束，重置输入上下文
    if (!inExecution) {
        resetStatus();
    }
}

bool InputContext::isInCommandExecution() const {
    return m_inCommandExecution;
}

// 提示信息相关
void InputContext::setPrompt(const std::string& prompt) {
    m_prompt = prompt;
}

const std::string& InputContext::getPrompt() const {
    return m_prompt;
}

// 输入状态管理
InputStatus InputContext::getCurrentStatus() {
    if (m_inCommandCancelProcess)
    {
        m_currentStatus = InputStatus::kCanceled;
    }
    return m_currentStatus;
}

void InputContext::resetStatus() {
    m_currentStatus = InputStatus::kNone;
    m_inCommandCancelProcess = false;
    m_allowedTypes.clear();
    m_prompt = "";
    m_pickedPoint = glm::dvec3(0, 0, 0);
    m_inputInteger = 0;
    m_inputFloat = 0.0;
    m_inputString = "";
    m_inputKeyword = "";
    m_keywordOptions.clear();
    m_selectedEntities.clear();
}

// 允许的输入类型管理
void InputContext::setAllowedTypes(const std::vector<InputType>& types) {
    m_allowedTypes = types;
}

const std::vector<InputType>& InputContext::getAllowedTypes() const {
    return m_allowedTypes;
}

// 点拾取相关
void InputContext::setPickedPoint(const glm::dvec3& point) {
    // 只有在允许点输入时才设置状态
    if (m_allowedTypes.empty() || std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kPoint) != m_allowedTypes.end()) {
        m_pickedPoint = point;
        m_currentStatus = InputStatus::kPointInput;
    }
}

bool InputContext::getPickedPoint(glm::dvec3& point) {
    if (m_currentStatus == InputStatus::kPointInput) {
        point = m_pickedPoint;
        m_currentStatus = InputStatus::kNone;
        return true;
    }
    return false;
}

// 处理鼠标左键点击（由InputHandler调用）
void InputContext::handleLeftMouseClick(const glm::vec2& screenPos) {
    // 检查是否有活动命令
    if (m_inCommandExecution) {
        // 将屏幕坐标转换为世界坐标
        glm::dvec3 worldPos = Renderer::getTransformManager().screenToWorld(screenPos);
        // 设置到输入上下文
        setPickedPoint(worldPos);
    }
}

// 处理鼠标右键点击（由InputHandler调用）
void InputContext::handleRightMouseClick(const glm::vec2& screenPos) {
    // 检查是否有活动命令
    if (m_inCommandExecution) {
        // 暂时空出
    }
}

// 数字输入相关
bool InputContext::getInteger(int& value) {
    if (m_currentStatus == InputStatus::kIntegerInput) {
        value = m_inputInteger;
        m_currentStatus = InputStatus::kNone;
        return true;
    }
    return false;
}

bool InputContext::getFloat(double& value) {
    if (m_currentStatus == InputStatus::kFloatInput) {
        value = m_inputFloat;
        m_currentStatus = InputStatus::kNone;
        return true;
    }
    return false;
}

bool InputContext::getNumber(double& value) {
    if (m_currentStatus == InputStatus::kIntegerInput) {
        value = static_cast<double>(m_inputInteger);
        m_currentStatus = InputStatus::kNone;
        return true;
    } else if (m_currentStatus == InputStatus::kFloatInput) {
        value = m_inputFloat;
        m_currentStatus = InputStatus::kNone;
        return true;
    }
    return false;
}

// 字符串输入相关
bool InputContext::getString(std::string& str) {
    if (m_currentStatus == InputStatus::kStringInput) {
        str = m_inputString;
        m_currentStatus = InputStatus::kNone;
        return true;
    }
    return false;
}

// 关键字输入相关
void InputContext::setKeywordOptions(const std::vector<std::string>& options) {
    m_keywordOptions = options;
}

const std::vector<std::string>& InputContext::getKeywordOptions() const {
    return m_keywordOptions;
}
// 关键字输入相关
bool InputContext::getKeyword(std::string& keyword) {
    if (m_currentStatus == InputStatus::kKeywordInput) {
        keyword = m_inputKeyword;
        m_currentStatus = InputStatus::kNone;
        cmdLinePrint(m_prompt);
        return true;
    }
    return false;
}

// 实体选择相关
void InputContext::setSelectedEntities(const std::vector<void*>& entities) {
    // 只有在允许实体选择输入时才设置状态
    if (m_allowedTypes.empty() || std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kEntitySelection) != m_allowedTypes.end()) {
        m_selectedEntities = entities;
        m_currentStatus = InputStatus::kEntitySelection;
    }
}

bool InputContext::getSelectedEntities(std::vector<void*>& entities) {
    if (m_currentStatus == InputStatus::kEntitySelection) {
        entities = m_selectedEntities;
        m_currentStatus = InputStatus::kNone;
        cmdLinePrint(m_prompt);
        return true;
    }
    return false;
}

// 输入解析
void InputContext::parseInput(const std::string& input) {
    // 重置当前状态
    m_currentStatus = InputStatus::kNone;
    
    // 检查是否为空输入（回车）
    if (input.empty()) {
        // 所有输入都允许回车
        m_currentStatus = InputStatus::kEnterInput;
        // 更新命令提示
        cmdLinePrint(m_prompt);
        return;
    }
    
    // 检查是否是关键字（如果允许关键字输入）
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kKeyword) != m_allowedTypes.end()) {
        for (const auto& option : m_keywordOptions) {
            // 忽略大小写比较
            if (StringUtils::equalsIgnoreCase(input, option)) {
                m_inputKeyword = input;
                m_currentStatus = InputStatus::kKeywordInput;
                // 更新命令提示
                cmdLinePrint(m_prompt);
                return;
            }
        }
    }
    
    // 尝试解析为整数（如果允许整数输入）
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kInteger) != m_allowedTypes.end()) {
        try {
            size_t pos;
            int intValue = std::stoi(input, &pos);
            if (pos == input.length()) {
                m_inputInteger = intValue;
                m_currentStatus = InputStatus::kIntegerInput;
                // 更新命令提示
                cmdLinePrint(m_prompt);
                return;
            }
        } catch (...) {
            // 解析失败，继续尝试其他类型
        }
    }
    
    // 尝试解析为浮点数（如果允许浮点数输入）
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kFloat) != m_allowedTypes.end()) {
        try {
            size_t pos;
            double floatValue = std::stod(input, &pos);
            if (pos == input.length()) {
                m_inputFloat = floatValue;
                m_currentStatus = InputStatus::kFloatInput;
                // 更新命令提示
                cmdLinePrint(m_prompt);
                return;
            }
        } catch (...) {
            // 解析失败，继续尝试其他类型
        }
    }
    
    // 尝试作为字符串处理（如果允许字符串输入）
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kString) != m_allowedTypes.end()) {
        m_inputString = input;
        m_currentStatus = InputStatus::kStringInput;
        // 更新命令提示
        cmdLinePrint(m_prompt);
        return;
    }
    
    // 没有匹配的类型，输出错误提示
    // 输出提示字符串+输入字符串
    std::string inputPrompt = m_prompt + " " + input;
    cmdLinePrint(inputPrompt);
    
    // 输出错误提示
    if (!m_errorPrompt.empty()) {
        cmdLinePrint(m_errorPrompt);
    }
    
    // 保持当前状态为kNone，等待下一次输入
    m_currentStatus = InputStatus::kNone;
}

// 是否在取消命令执行的过程中，CommandManager全权维护，通过模拟多次Cancel来实现取消命令执行，在取消命令过程中则所有交互直接返回kCanceled
void InputContext::setInCommandCancelProcess(bool inProcess) {
    m_inCommandCancelProcess = inProcess;
    if (m_inCommandCancelProcess) {
        m_currentStatus = InputStatus::kCanceled;
    }
    else {
        m_currentStatus = InputStatus::kNone;
    }
}

bool InputContext::inCommandCancelProcess() const {
    return m_inCommandCancelProcess;
}

// 预览功能（暂时空实现）
void InputContext::drawRubberBand(const glm::dvec3& startPoint) {
    // 暂时空实现
}

// 处理Enter/Space输入
void InputContext::handleEnterSpace(const std::string& input) {
    if (m_inCommandExecution) {
        // 命令执行中，将输入作为命令参数解析
        parseInput(input);
    } else {
        // 命令执行外，作为新命令处理
        // 添加到命令历史
        auto& loc = LocalizationManager::getInstance();
        std::string promptStr = loc.get("commandBar.prompt") + " " + input;
        cmdLinePrint(promptStr);
        // 解析为新命令
        CommandManager::getInstance().executeCommand(input);
    }
}

// 处理Escape输入
void InputContext::handleEscape(const std::string& input) {
    if (m_inCommandExecution) {
        // 命令执行中，设置取消状态
        m_currentStatus = InputStatus::kCanceled;
        // 更新命令提示，添加取消标记和用户输入
        auto& loc = LocalizationManager::getInstance();
        std::string cancelPrompt = m_prompt + " " + input + " " + loc.get("commandBar.prompt.cancel");
        cmdLinePrint(cancelPrompt);
    } else {
        // 没有命令执行时按下Esc，键入的字符串也会被输出到命令历史
        auto& loc = LocalizationManager::getInstance();
        std::string promptStr = loc.get("commandBar.prompt") + " " + input + " " +  loc.get("commandBar.prompt.cancel");
        cmdLinePrint(promptStr);
    }
}

// 特殊按键事件管理
void InputContext::setSpecialKeyEvent(SpecialKeyEventType event) {
    m_lastSpecialKeyEvent = event;
}

SpecialKeyEventType InputContext::getLastSpecialKeyEvent() const {
    return m_lastSpecialKeyEvent;
}

void InputContext::clearSpecialKeyEvent() {
    m_lastSpecialKeyEvent = SpecialKeyEventType::kNone;
}

// 输入管理
void InputContext::setInput(const std::string& input) {
    m_input = input;
}

const std::string& InputContext::getInput() const {
    return m_input;
}

// 等待点输入（带基点）
void InputContext::waitForPoint(const std::string& prompt, const glm::dvec3& basePoint, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kPoint, InputType::kKeyword});
        setKeywordOptions(keywords);
    } else {
        setAllowedTypes({InputType::kPoint});
    }
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidPoint"); // *无效点*
    // 可以在这里使用basePoint进行一些计算或设置
}

// 等待点输入（无基点）
void InputContext::waitForPoint(const std::string& prompt, const std::vector<std::string>& keywords) {
    waitForPoint(prompt, glm::dvec3(0, 0, 0), keywords);
}

// 等待数值输入
void InputContext::waitForNumber(const std::string& prompt, double min, double max) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kInteger, InputType::kFloat});
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidNumber"); // 需要输入数值。
    // 可以在这里设置数值范围
}

// 等待整数输入
void InputContext::waitForInteger(const std::string& prompt, int min, int max) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kInteger});
    // 设置错误提示 - 从本地化资源加载并格式化
    auto& loc = LocalizationManager::getInstance();
    std::string errorTemplate = loc.get("inputContext.generalErrorPrompt.invalidInteger"); // 需要输入{0}到{1}的整数
    // 使用StringUtils::format进行格式化，处理可能的异常
    m_errorPrompt = StringUtils::format(errorTemplate, min, max);
    // 可以在这里设置整数范围
}

// 等待浮点数输入
void InputContext::waitForFloat(const std::string& prompt, double min, double max) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kFloat});
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidNumber"); // 需要输入数值。
    // 可以在这里设置浮点数范围
}

// 等待字符串输入
void InputContext::waitForString(const std::string& prompt) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kString});
    // 设置错误提示（字符串一般不会失效）
    m_errorPrompt = "";
}

// 等待关键字输入
void InputContext::waitForKeyword(const std::string& prompt, const std::vector<std::string>& options) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kKeyword});
    setKeywordOptions(options);
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidKeyword"); // *无效关键字*
}

// 等待回车输入
void InputContext::waitForEnter(const std::string& prompt) {
    setPrompt(prompt);
    // 不需要设置allowedTypes，因为所有输入都允许回车
    setAllowedTypes({});
    // 设置错误提示
    m_errorPrompt = "";
}

// 等待实体选择输入
void InputContext::waitForEntity(const std::string& prompt, const std::vector<void*>& existingEntities, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kEntitySelection, InputType::kKeyword});
        setKeywordOptions(keywords);
    } else {
        setAllowedTypes({InputType::kEntitySelection});
    }
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidSelection"); // *无效选择*
    // 可以在这里使用existingEntities进行一些操作
}

// 绘制输入上下文信息窗口
void InputContext::drawInfoWindow(bool* p_open) {
    if (p_open && !*p_open) {
        return;
    }
    
    // InputType枚举到字符串的静态关联列表
    static const std::unordered_map<InputType, std::string> inputTypeToString = {
        {InputType::kInteger, "Integer"},
        {InputType::kFloat, "Float"},
        {InputType::kString, "String"},
        {InputType::kKeyword, "Keyword"},
        {InputType::kPoint, "Point"},
        {InputType::kEntitySelection, "EntitySelection"}
    };
    
    // InputStatus枚举到字符串的静态关联列表
    static const std::unordered_map<InputStatus, std::string> inputStatusToString = {
        {InputStatus::kNone, "None"},
        {InputStatus::kCanceled, "Canceled"},
        {InputStatus::kEnterInput, "EnterInput"},
        {InputStatus::kIntegerInput, "IntegerInput"},
        {InputStatus::kFloatInput, "FloatInput"},
        {InputStatus::kStringInput, "StringInput"},
        {InputStatus::kKeywordInput, "KeywordInput"},
        {InputStatus::kPointInput, "PointInput"},
        {InputStatus::kEntitySelection, "EntitySelection"}
    };
    
    // 添加LocalizationManager引用
    LocalizationManager& loc = LocalizationManager::getInstance();
    
    // 修改窗口标题
    ImGui::Begin(loc.get("window.inputContextInfo.title").c_str(), p_open);
    
    // 1. 命令执行状态 - 显示是否正在执行命令
    ImGui::Text("%s: %s", 
               loc.get("window.inputContextInfo.inCommandExecution").c_str(), 
               m_inCommandExecution ? "true" : "false");
    
    // 2. 当前状态 - 显示输入状态机的当前状态
    auto statusIt = inputStatusToString.find(m_currentStatus);
    if (statusIt != inputStatusToString.end()) {
        ImGui::Text("%s: %s", 
                   loc.get("window.inputContextInfo.currentStatus").c_str(), 
                   statusIt->second.c_str());
    } else {
        ImGui::Text("%s: %s(%d)", 
                   loc.get("window.inputContextInfo.currentStatus").c_str(), 
                   loc.get("window.inputContextInfo.unknown").c_str(),
                   static_cast<int>(m_currentStatus));
    }
    
    // 3. 允许的输入类型 - 显示当前允许的输入类型列表
    ImGui::Text("%s (%zu):", 
               loc.get("window.inputContextInfo.allowedTypes").c_str(), 
               m_allowedTypes.size());
    for (size_t i = 0; i < m_allowedTypes.size(); ++i) {
        auto it = inputTypeToString.find(m_allowedTypes[i]);
        if (it != inputTypeToString.end()) {
            ImGui::Text("  [%zu] %s", i, it->second.c_str());
        } else {
            ImGui::Text("  [%zu] Unknown(%d)", i, static_cast<int>(m_allowedTypes[i]));
        }
    }
    
    // 4. 关键字选项 - 显示可选的关键字列表
    ImGui::Text("%s (%zu):", 
               loc.get("window.inputContextInfo.keywordOptions").c_str(), 
               m_keywordOptions.size());
    for (size_t i = 0; i < m_keywordOptions.size(); ++i) {
        ImGui::Text("  [%zu] %s", i, m_keywordOptions[i].c_str());
    }

    // 5. 提示信息 - 显示给用户的提示文本
    ImGui::Text("%s: %s", 
               loc.get("window.inputContextInfo.prompt").c_str(), 
               m_prompt.empty() ? "" : m_prompt.c_str());
    
    // 6. 错误提示 - 显示错误信息文本
    ImGui::Text("%s: %s", 
               loc.get("window.inputContextInfo.errorPrompt").c_str(), 
               m_errorPrompt.empty() ? "" : m_errorPrompt.c_str());
    
    ImGui::End();
}

} // namespace tch
