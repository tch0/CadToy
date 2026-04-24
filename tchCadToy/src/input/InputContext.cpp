// 对应头文件
#include "InputContext.h"

// C++ 标准库
#include <algorithm>

// 第三方库
#include <glm/glm.hpp>
#include <imgui.h>

// 项目头文件
#include "CommandManager.h"
#include "InputHandler.h"
#include "Renderer.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"
#include "DocManager.h"

namespace tch {

// 构造函数
InputContext::InputContext() :
    m_inCommandExecution(false),
    m_currentStatus(InputStatus::kNone),
    m_allowedTypes(),
    m_prompt(""),
    m_errorPrompt(""),
    m_pickedPoint(glm::dvec3(0, 0, 0)),
    m_bHasBasePoint(false),
    m_basePoint(glm::dvec3(0, 0, 0)),
    m_inputInteger(0),
    m_intLimitMin(INT_MIN),
    m_intLimitMax(INT_MAX),
    m_inputFloat(0.0),
    m_floatLimitMin(-DBL_MAX),
    m_floatLimitMax(DBL_MAX),
    m_inputString(""),
    m_inputKeyword(""),
    m_keywordOptions(),
    m_selectedEntities(),
    m_lastSpecialKeyEvent(SpecialKeyEventType::kNone),
    m_inputContextInfoVisible(false),
    m_selectionTask() {
}

// 获取单例实例
InputContext& InputContext::getInstance() {
    static InputContext instance;
    return instance;
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

// 是否处于命令执行中或者任何任务(例如选择交互、夹点编辑交互)执行中
bool InputContext::isAnyCommandOrTaskRunning() const
{
    return m_inCommandExecution || m_selectionTask.isSelecting();
}

// 提示信息相关
void InputContext::setPrompt(const std::string& prompt){
    m_prompt = prompt;
}

const std::string& InputContext::getPrompt() const {
    return m_prompt;
}

void InputContext::setErrorPrompt(const std::string& errorPrompt) {
    m_errorPrompt = errorPrompt;
}

// 交互状态管理
InputStatus InputContext::getCurrentStatus() {
    // 状态获取到之后需要及时清理：
    //      对于kNone/kEnterInput/kCanceled之外的一切输入状态，都会在对应getXXX调用中清理，waitForxxx之后要得到输入，必须调用getxxx获取输入并清理状态
    //      但kEnterInput/kCanceled是不需要获取输入的，这个状态本身已经包含了所有信息，命令侧也不应当承担清理状态的责任，所以这里需要清理
    InputStatus status = m_currentStatus;
    if (status == InputStatus::kEnterInput || status == InputStatus::kCanceled) {
        m_currentStatus = InputStatus::kNone;
    }
    return status;
}

// 清除所有交互状态，除了交互数据，提供给选择任务使用
void InputContext::resetStatusExceptInteractionData() {
    m_currentStatus = InputStatus::kNone;
    m_allowedTypes.clear();
    m_prompt = "";
    m_errorPrompt = "";
    m_pickedPoint = glm::dvec3(0, 0, 0);
    m_bHasBasePoint = false;
    m_basePoint = glm::dvec3(0, 0, 0);
    m_inputInteger = 0;
    m_intLimitMin = INT_MIN;
    m_intLimitMax = INT_MAX;
    m_inputFloat = 0.0;
    m_floatLimitMin = -DBL_MAX;
    m_floatLimitMax = DBL_MAX;
    m_inputString = "";
    m_inputKeyword = "";
    m_keywordOptions.clear();
    m_selectedEntities.clear();
}

// 清除所有交互状态，包括交互数据
void InputContext::resetStatus() {
    resetStatusExceptInteractionData();
    m_interactionData.reset();
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
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kPoint) != m_allowedTypes.end()) {
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

// 处理鼠标左键点击事件
void InputContext::handleLeftMouseClick() {
    // TODO: 命令中与选择中点输入流程完全一致，需不需要统一起来？
    // 检查是否有活动命令
    if (m_inCommandExecution) {
        // 从InputHandler获取光标位置
        glm::vec2 screenPos = InputHandler::getCursorPosition();
        // 将屏幕坐标转换为世界坐标
        glm::dvec3 worldPos = Renderer::getTransformManager().screenToWorld(screenPos);
        // 设置到输入上下文
        if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kPoint) != m_allowedTypes.end()) {
            m_pickedPoint = worldPos;
            m_currentStatus = InputStatus::kPointInput;
            Utils::cmdLinePrint(m_prompt);
        }
    }
    else {
        // 处理选择任务中的点输入
        if (m_selectionTask.isSelecting()) {
            // 正在选择
            // 从InputHandler获取光标位置
            glm::vec2 screenPos = InputHandler::getCursorPosition();
            // 将屏幕坐标转换为世界坐标
            glm::dvec3 worldPos = Renderer::getTransformManager().screenToWorld(screenPos);
            // 设置到输入上下文
            if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kPoint) != m_allowedTypes.end()) {
                m_pickedPoint = worldPos;
                m_currentStatus = InputStatus::kPointInput;
                Utils::cmdLinePrint(m_prompt);
            }
        }
        // 没有活动任务也不在命令中，激活选择任务，进入框选过程
        else {
            activateSelectionTask(SelectionMode::kWindow);
        }
    }
}

// 处理鼠标右键点击（由InputHandler调用）
void InputContext::handleRightMouseClick() {
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
    }
    else if (m_currentStatus == InputStatus::kFloatInput) {
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
        return true;
    }
    return false;
}

// 实体选择相关
void InputContext::setSelectedEntities(const std::vector<void*>& entities) {
    // 只有在允许实体选择输入时才设置状态
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kEntitySelection) != m_allowedTypes.end()) {
        m_selectedEntities = entities;
        m_currentStatus = InputStatus::kEntitySelection;
    }
}

bool InputContext::getSelectedEntities(std::vector<void*>& entities) {
    if (m_currentStatus == InputStatus::kEntitySelection) {
        entities = m_selectedEntities;
        m_currentStatus = InputStatus::kNone;
        return true;
    }
    return false;
}

// 输入解析
void InputContext::parseInput(const std::string& input) {
    // 重置当前状态
    m_currentStatus = InputStatus::kNone;
    
    // 检查是否为空输入（Enter/Space），至于什么回车执行默认选项之类的逻辑则返回kEnterInput之后由命令自行去处理
    if (input.empty()) {
        // 所有输入都允许Enter/Space
        m_currentStatus = InputStatus::kEnterInput;
        // 更新命令提示
        if (!m_prompt.empty()) {
            Utils::cmdLinePrint(m_prompt);
        }
        return;
    }
    
    std::string inputPrompt = m_prompt + " " + input;
    // 检查是否是关键字（如果允许关键字输入）
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kKeyword) != m_allowedTypes.end()) {
        for (const auto& option : m_keywordOptions) {
            // 忽略大小写比较
            if (StringUtils::equalsIgnoreCase(input, option)) {
                m_inputKeyword = StringUtils::toUpperCase(input);
                m_currentStatus = InputStatus::kKeywordInput;
                // 更新命令提示
                Utils::cmdLinePrint(inputPrompt);
                return;
            }
        }
    }
    
    // 尝试解析为点坐标（如果允许点输入）- 支持 "x,y" 格式的二维坐标与距离输入
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kPoint) != m_allowedTypes.end()) {
        size_t pos = input.find(',');
        // 尝试解析为二维点输入: xpos,ypos -> point = (xpos,ypos,0)
        if (pos != std::string::npos && pos > 0 && pos < input.length() - 1) {
            try {
                std::string xStr = input.substr(0, pos);
                std::string yStr = input.substr(pos + 1);
                
                size_t posX, posY;
                double x = std::stod(xStr, &posX);
                double y = std::stod(yStr, &posY);
                
                if (posX == xStr.length() && posY == yStr.length()) {
                    m_pickedPoint = glm::dvec3(x, y, 0.0);
                    m_currentStatus = InputStatus::kPointInput;
                    Utils::cmdLinePrint(inputPrompt);
                    return;
                }
            } catch (...) {
                // 解析失败，不是合法点输入
            }
        }
        // 有基点，支持距离输入，尝试解析为距离输入, point = normalized(previewPoint - basePoint) * distance
        else if (m_bHasBasePoint) {
            glm::dvec3 previewPointWorld = Renderer::getTransformManager().screenToWorld(InputHandler::getCursorPosition());
            try {
                // TODO: 考虑误差和精度，当预览点和基点误差极小时，视为同一点，则获取到的新点直接等于基点
                double distance = stod(input);
                m_pickedPoint = m_basePoint + glm::normalize(previewPointWorld - m_basePoint) * distance;
                m_currentStatus = InputStatus::kPointInput;
                Utils::cmdLinePrint(inputPrompt);
                return;
            } catch (...) {
                // 解析失败，不是合法距离输入
            }
        }
    }
    
    // 尝试解析为整数（如果允许整数输入）
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kInteger) != m_allowedTypes.end()) {
        try {
            size_t pos;
            int intValue = std::stoi(input, &pos);
            if (pos == input.length()) {
                // 检查范围
                if (intValue >= m_intLimitMin && intValue <= m_intLimitMax) {
                    m_inputInteger = intValue;
                    m_currentStatus = InputStatus::kIntegerInput;
                    // 更新命令提示
                    Utils::cmdLinePrint(inputPrompt);
                    return;
                }
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
                // 检查范围
                if (floatValue >= m_floatLimitMin && floatValue <= m_floatLimitMax) {
                    m_inputFloat = floatValue;
                    m_currentStatus = InputStatus::kFloatInput;
                    // 更新命令提示
                    Utils::cmdLinePrint(inputPrompt);
                    return;
                }
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
        Utils::cmdLinePrint(inputPrompt);
        return;
    }
    
    // 没有匹配的类型，输出错误提示
    // 输出提示字符串+输入字符串
    Utils::cmdLinePrint(inputPrompt);
    
    // 输出错误提示
    if (!m_errorPrompt.empty()) {
        Utils::cmdLinePrint(m_errorPrompt);
    }
    
    // 保持当前状态为kNone，等待下一次输入
    m_currentStatus = InputStatus::kNone;
    return;
}

// 预览功能（暂时空实现）
void InputContext::drawRubberBand(const glm::dvec3& startPoint) {
    // 暂时空实现
}

// 处理Enter/Space输入
void InputContext::handleEnterSpace(const std::string& input) {
    // 选择或者命令执行中，解析输入
    if (m_selectionTask.isSelecting() || m_inCommandExecution) {
        parseInput(input);
    }
    else {
        // 输入为空则输出一个空行
        if (input.empty())
        {
            auto& loc = LocalizationManager::getInstance();
            Utils::cmdLinePrint(loc.get("commandLine.prompt.command"));
        }
        // 不为空则解析为新命令，输出当前执行的命令的操作会在executeCommand中做
        else {
            CommandManager::getInstance().executeCommand(input);
        }
    }
}

// 处理Escape输入
void InputContext::handleEscape(const std::string& input) {
    if (m_selectionTask.isSelecting() || m_inCommandExecution) {
        // 选择或者命令执行中，设置取消状态
        m_currentStatus = InputStatus::kCanceled;
        // 更新命令提示，添加取消标记和用户输入
        auto& loc = LocalizationManager::getInstance();
        std::string cancelPrompt = (m_prompt.empty() ? "" : m_prompt + " ") + input + loc.get("commandLine.prompt.cancel");
        Utils::cmdLinePrint(cancelPrompt);
    }
    else {
        // 没有命令执行时按下Esc，键入的字符串也会被输出到命令历史
        auto& loc = LocalizationManager::getInstance();
        std::string promptStr = loc.get("commandLine.prompt.command") + " " + input + loc.get("commandLine.prompt.cancel");
        Utils::cmdLinePrint(promptStr);
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

// 等待点输入（带基点）
void InputContext::waitForPoint(const std::string& prompt, const glm::dvec3& basePoint, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kPoint, InputType::kKeyword});
        setKeywordOptions(keywords);
    }
    else {
        setAllowedTypes({InputType::kPoint});
    }
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidPoint"); // *无效点*
    // 保存基点
    m_basePoint = basePoint;
    m_bHasBasePoint = true;
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
}

// 等待点输入（无基点）
void InputContext::waitForPoint(const std::string& prompt, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kPoint, InputType::kKeyword});
        setKeywordOptions(keywords);
    }
    else {
        setAllowedTypes({InputType::kPoint});
    }
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidPoint"); // *无效点*
    // 没有基点
    m_bHasBasePoint = false;
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
}

// 等待数值输入
void InputContext::waitForNumber(const std::string& prompt, double min, double max) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kInteger, InputType::kFloat});
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidNumber"); // 需要输入数值。
    
    // 数值范围
    m_floatLimitMin = min;
    m_floatLimitMax = max;
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
}

// 等待数值输入，同时允许关键字
void InputContext::waitForNumber(const std::string& prompt, double min, double max, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kInteger, InputType::kFloat, InputType::kKeyword});
        setKeywordOptions(keywords);
    }
    else {
        setAllowedTypes({InputType::kInteger, InputType::kFloat});
    }
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidNumber"); // 需要输入数值。
    
    // 数值范围
    m_floatLimitMin = min;
    m_floatLimitMax = max;
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
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
    
    // 数值范围
    m_intLimitMin = min;
    m_intLimitMax = max;
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
}

// 等待整数输入，同时允许关键字
void InputContext::waitForInteger(const std::string& prompt, int min, int max, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kInteger, InputType::kKeyword});
        setKeywordOptions(keywords);
    }
    else {
        setAllowedTypes({InputType::kInteger});
    }
    // 设置错误提示 - 从本地化资源加载并格式化
    auto& loc = LocalizationManager::getInstance();
    std::string errorTemplate = loc.get("inputContext.generalErrorPrompt.invalidInteger"); // 需要输入{0}到{1}的整数
    // 使用StringUtils::format进行格式化，处理可能的异常
    m_errorPrompt = StringUtils::format(errorTemplate, min, max);
    
    // 数值范围
    m_intLimitMin = min;
    m_intLimitMax = max;
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
}

// 等待浮点数输入
void InputContext::waitForFloat(const std::string& prompt, double min, double max) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kFloat});
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidNumber"); // 需要输入数值。
    
    // 浮点数范围
    m_floatLimitMin = min;
    m_floatLimitMax = max;
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
}

// 等待字符串输入
void InputContext::waitForString(const std::string& prompt) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kString});
    // 设置错误提示（字符串一般不会失效）
    m_errorPrompt = "";
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
}

// 等待关键字输入
void InputContext::waitForKeyword(const std::string& prompt, const std::vector<std::string>& options) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kKeyword});
    setKeywordOptions(options);
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidKeyword"); // *无效关键字*
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
}

// 等待回车输入
void InputContext::waitForEnter(const std::string& prompt) {
    setPrompt(prompt);
    // 不需要设置allowedTypes，因为所有输入都允许回车
    setAllowedTypes({});
    // 设置错误提示
    m_errorPrompt = "";
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
}

// 等待实体选择输入
void InputContext::waitForEntity(const std::string& prompt, const std::vector<void*>& existingEntities, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kEntitySelection, InputType::kKeyword});
        setKeywordOptions(keywords);
    }
    else {
        setAllowedTypes({InputType::kEntitySelection});
    }
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidSelection"); // *无效选择*
    // 可以在这里使用existingEntities进行一些操作
}

// 绘制输入上下文信息窗口
void InputContext::drawInfoWindow() {
    if (!m_inputContextInfoVisible) {
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
    ImGui::Begin(loc.get("window.inputContextInfo.title").c_str(), &m_inputContextInfoVisible);
    
    // 命令执行状态 - 显示是否正在执行命令
    ImGui::Text("%s: %s", 
               loc.get("window.inputContextInfo.inCommandExecution").c_str(), 
               m_inCommandExecution ? "true" : "false");
               
    // 选择任务执行状态 - 显示是否正在进行选择
    ImGui::Text("%s: %s", 
                loc.get("window.inputContextInfo.inSelectionTask").c_str(),
                m_selectionTask.isSelecting() ? "true" : "false");
    
    // 当前状态 - 显示输入状态机的当前状态
    auto statusIt = inputStatusToString.find(m_currentStatus);
    if (statusIt != inputStatusToString.end()) {
        ImGui::Text("%s: %s", 
                   loc.get("window.inputContextInfo.currentStatus").c_str(), 
                   statusIt->second.c_str());
    }
    else {
        ImGui::Text("%s: %s(%d)", 
                   loc.get("window.inputContextInfo.currentStatus").c_str(), 
                   loc.get("window.inputContextInfo.unknown").c_str(),
                   static_cast<int>(m_currentStatus));
    }
    
    // 允许的输入类型 - 显示当前允许的输入类型列表
    ImGui::Text("%s (%zu):", 
               loc.get("window.inputContextInfo.allowedTypes").c_str(), 
               m_allowedTypes.size());
    for (size_t i = 0; i < m_allowedTypes.size(); ++i) {
        auto it = inputTypeToString.find(m_allowedTypes[i]);
        if (it != inputTypeToString.end()) {
            ImGui::Text("  [%zu] %s", i, it->second.c_str());
        }
        else {
            ImGui::Text("  [%zu] Unknown(%d)", i, static_cast<int>(m_allowedTypes[i]));
        }
    }
    
    // 关键字选项 - 显示可选的关键字列表
    ImGui::Text("%s (%zu):", 
               loc.get("window.inputContextInfo.keywordOptions").c_str(), 
               m_keywordOptions.size());
    for (size_t i = 0; i < m_keywordOptions.size(); ++i) {
        ImGui::Text("  [%zu] %s", i, m_keywordOptions[i].c_str());
    }

    // 提示信息 - 显示给用户的提示文本
    ImGui::Text("%s: %s", 
               loc.get("window.inputContextInfo.prompt").c_str(), 
               m_prompt.empty() ? "" : m_prompt.c_str());
    
    // 错误提示 - 显示错误信息文本
    ImGui::Text("%s: %s", 
               loc.get("window.inputContextInfo.errorPrompt").c_str(), 
               m_errorPrompt.empty() ? "" : m_errorPrompt.c_str());
    
    // 添加分隔线
    ImGui::Separator();
    
    // 交互数据
    ImGui::Text("%s:", loc.get("window.inputContextInfo.interactionData").c_str());
    
    // 光标模式
    static const std::unordered_map<CursorMode, std::string> cursorModeToString = {
        {CursorMode::kDefault, "Default"},
        {CursorMode::kCrosshair, "Crosshair"},
        {CursorMode::kPickbox, "Pickbox"},
        {CursorMode::kPanning, "Panning"}
    };
    auto cursorModeIt = cursorModeToString.find(m_interactionData.cursorMode);
    if (cursorModeIt != cursorModeToString.end()) {
        ImGui::Text("  %s: %s", 
                   loc.get("window.inputContextInfo.cursorMode").c_str(), 
                   cursorModeIt->second.c_str());
    }
    else {
        ImGui::Text("  %s: Unknown(%d)", 
                   loc.get("window.inputContextInfo.cursorMode").c_str(), 
                   static_cast<int>(m_interactionData.cursorMode));
    }
    
    // 光标标记
    static const std::unordered_map<CursorMarker, std::string> cursorMarkerToString = {
        {CursorMarker::kNone, "None"},
        {CursorMarker::kLocked, "Locked"},
        {CursorMarker::kOrthogonal, "Orthogonal"},
        {CursorMarker::kErase, "Erase"},
        {CursorMarker::kCopy, "Copy"},
        {CursorMarker::kMove, "Move"},
        {CursorMarker::kRotate, "Rotate"},
        {CursorMarker::kScale, "Scale"},
        {CursorMarker::kAddSelect, "AddSelect"},
        {CursorMarker::kRemoveSelect, "RemoveSelect"},
        {CursorMarker::kCrossingSelect, "CrossingSelect"},
        {CursorMarker::kWindowSelect, "WindowSelect"}
    };
    auto cursorMarkerIt = cursorMarkerToString.find(m_interactionData.cursorMarker);
    if (cursorMarkerIt != cursorMarkerToString.end()) {
        ImGui::Text("  %s: %s", 
                   loc.get("window.inputContextInfo.cursorMarker").c_str(), 
                   cursorMarkerIt->second.c_str());
    }
    else {
        ImGui::Text("  %s: Unknown(%d)", 
                   loc.get("window.inputContextInfo.cursorMarker").c_str(), 
                   static_cast<int>(m_interactionData.cursorMarker));
    }
    
    // 选择模式
    static const std::unordered_map<SelectionMode, std::string> selectionModeToString = {
        {SelectionMode::kNone, "None"},
        {SelectionMode::kSingle, "Single"},
        {SelectionMode::kWindow, "Window"},
        {SelectionMode::kCrossing, "Crossing"},
        {SelectionMode::kFence, "Fence"},
        {SelectionMode::kWindowLasso, "WindowLasso"},
        {SelectionMode::kCrossingLasso, "CrossingLasso"},
        {SelectionMode::kWindowPolygon, "WindowPolygon"},
        {SelectionMode::kCrossingPolygon, "CrossingPolygon"},
        {SelectionMode::kAll, "All"}
    };
    auto selectionModeIt = selectionModeToString.find(m_interactionData.selectionMode);
    if (selectionModeIt != selectionModeToString.end()) {
        ImGui::Text("  %s: %s", 
                   loc.get("window.inputContextInfo.selectionMode").c_str(), 
                   selectionModeIt->second.c_str());
    }
    else {
        ImGui::Text("  %s: Unknown(%d)", 
                   loc.get("window.inputContextInfo.selectionMode").c_str(), 
                   static_cast<int>(m_interactionData.selectionMode));
    }
    
    // 选择状态
    ImGui::Text("  %s: %s", 
               loc.get("window.inputContextInfo.isSelectionActive").c_str(), 
               m_interactionData.isSelectionActive ? "true" : "false");
    
    // 选择初始点
    ImGui::Text("  %s: (%.2f, %.2f, %.2f)", 
               loc.get("window.inputContextInfo.selectionInitialPoint").c_str(), 
               m_interactionData.selectionInitialPointWorld.x, 
               m_interactionData.selectionInitialPointWorld.y,
               m_interactionData.selectionInitialPointWorld.z);
    
    // 选择预览点
    ImGui::Text("  %s: (%.2f, %.2f, %.2f)", 
               loc.get("window.inputContextInfo.selectionPreviewPoint").c_str(), 
               m_interactionData.selectionPreviewPointWorld.x, 
               m_interactionData.selectionPreviewPointWorld.y,
               m_interactionData.selectionPreviewPointWorld.z);
    
    // 选择点数量
    ImGui::Text("  %s: %zu", 
               loc.get("window.inputContextInfo.selectionPointsCount").c_str(), 
               m_interactionData.selectionPointsWorld.size());
    
    ImGui::End();
}

// 获取交互数据
InteractionData& InputContext::getInteractionData() {
    return m_interactionData;
}

// 获取实时鼠标预览点的世界坐标
glm::dvec3 InputContext::getPreviewPoint() const {
    // 获取当前鼠标屏幕坐标
    glm::dvec2 screenPos = InputHandler::getCursorPosition();
    // 转换为世界坐标
    return DocManager::getCurrentDocument().getTransformManager().screenToWorld(screenPos);
}

// 更新输入上下文
void InputContext::onUpdate() {
    // 如果调用了waitForEntity来选择实体
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kEntitySelection) != m_allowedTypes.end()) {
        // 且选择任务还没有启动，那么此处启动选择任务
        if (!m_selectionTask.isSelecting()) {
            activateSelectionTask(SelectionMode::kSingle);
        }
    }
    
    // 更新选择任务
    if (m_selectionTask.isSelecting()) {
        m_selectionTask.onUpdate();
        if (m_selectionTask.isCompleted()) {
            // 选择任务结束后，重置输入上下文状态
            resetStatus();
            
            // TODO: 选择任务完成，处理选择结果
            std::vector<void*> selectedEntities;
            // 实际的实体选择逻辑需要在后续实现
            // 根据Shift状态决定是加选还是减选
            setSelectedEntities(selectedEntities);
            
            // 如果在命令执行中则需要获取选择返回的状态，以提供给命令来处理
            // 比如选择中Esc，命令可能直接结束
            if (m_inCommandExecution)
            {
                m_currentStatus = m_selectionTask.getInputStatus();
            }
            
            // 重置选择任务
            m_selectionTask.reset();
        }
    }
}

// 激活选择任务
void InputContext::activateSelectionTask(SelectionMode mode) {
    // 重置选择任务
    m_selectionTask.reset();
    
    // 开始选择任务，传递命令执行状态
    m_selectionTask.start(isInCommandExecution(), mode == SelectionMode::kSingle);
    
    // 设置选择模式
    m_interactionData.selectionMode = mode;
    m_interactionData.isSelectionActive = true;
}

} // namespace tch
