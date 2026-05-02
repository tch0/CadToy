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
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"
#include "DocManager.h"
#include "IGraphicsDataCache.h"

namespace tch {

// 构造函数
InputContext::InputContext() :
    m_bInCommandExecution(false),
    m_currentStatus(InputStatus::kNone),
    m_allowedTypes(),
    m_prompt(""),
    m_errorPrompt(""),
    m_pickedPoint(glm::dvec3(0, 0, 0)),
    m_bHasBasePoint(false),
    m_basePoint(glm::dvec3(0, 0, 0)),
    m_bDrawRubberBand(false),
    m_inputInteger(0),
    m_intLimitMin(INT_MIN),
    m_intLimitMax(INT_MAX),
    m_inputFloat(0.0),
    m_floatLimitMin(-DBL_MAX),
    m_floatLimitMax(DBL_MAX),
    m_inputString(""),
    m_inputKeyword(""),
    m_keywordOptions(),
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
    m_bInCommandExecution = inExecution;
    // 命令执行结束，重置输入上下文
    if (!inExecution) {
        resetStatus();
    }
}

bool InputContext::isInCommandExecution() const {
    return m_bInCommandExecution;
}

// 是否处于命令执行中或者任何任务(例如选择交互、夹点编辑交互)执行中
bool InputContext::isAnyCommandOrTaskRunning() const
{
    return m_bInCommandExecution || m_selectionTask.isSelecting();
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
        // 清除橡皮线
        if (m_bDrawRubberBand) {
            m_bDrawRubberBand = false;
            m_interactionData.clearRubberBand();
        }
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
    m_bDrawRubberBand = false;
    m_inputInteger = 0;
    m_intLimitMin = INT_MIN;
    m_intLimitMax = INT_MAX;
    m_inputFloat = 0.0;
    m_floatLimitMin = -DBL_MAX;
    m_floatLimitMax = DBL_MAX;
    m_inputString = "";
    m_inputKeyword = "";
    m_keywordOptions.clear();
    m_selectionResult.clear();
    // 注意: 先选选择集(m_priorSelectionSet)需要跨任务单独维护
    // 通过 clearPriorSelectionSet() 接口单独管理（包括取消高亮）
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
        // 清除橡皮线
        if (m_bDrawRubberBand) {
            m_bDrawRubberBand = false;
            m_interactionData.clearRubberBand();
        }
        return true;
    }
    return false;
}

// 处理鼠标左键点击事件
void InputContext::handleLeftMouseClick() {
    // TODO: 命令中与选择中点输入流程完全一致，需不需要统一起来？
    // 检查是否有活动命令
    if (m_bInCommandExecution) {
        // 设置到输入上下文
        if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kPoint) != m_allowedTypes.end()) {
            m_pickedPoint = getPreviewPoint(); // 鼠标点世界坐标
            m_currentStatus = InputStatus::kPointInput;
            Utils::cmdLinePrint(m_prompt);
        }
    }
    else {
        // 处理选择任务中的点输入
        if (m_selectionTask.isSelecting()) {
            // 正在选择
            // 设置到输入上下文
            if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kPoint) != m_allowedTypes.end()) {
                m_pickedPoint = getPreviewPoint(); // 鼠标点世界坐标
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
    if (m_bInCommandExecution) {
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

// 选择集相关
bool InputContext::getSelectionSet(SelectionSet& selectionSet) {
    if (m_currentStatus == InputStatus::kEntitySelection) {
        // 通过move将选择结果移交给命令层，避免拷贝
        selectionSet = std::move(m_selectionResult);
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
    
    // 尝试解析为点坐标（如果允许点输入）- 支持多种格式：绝对/相对笛卡尔、绝对/相对极坐标、直接距离
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kPoint) != m_allowedTypes.end()) {
        // 确定基点：有基点时用基点，否则用LastPoint
        glm::dvec3 basePoint = m_bHasBasePoint ? m_basePoint : getLastPoint();
        
        // 检查是否以@开头（相对坐标）
        bool isRelative = false;
        std::string coordInput = input;
        if (!input.empty() && input[0] == '@') {
            isRelative = true;
            coordInput = input.substr(1);
        }
        
        // 检查是否包含<（极坐标）
        size_t anglePos = coordInput.find('<');
        if (anglePos != std::string::npos && anglePos > 0 && anglePos < coordInput.length() - 1) {
            // 极坐标解析: 距离<角度 或 @距离<角度
            try {
                std::string distStr = coordInput.substr(0, anglePos);
                std::string angleStr = coordInput.substr(anglePos + 1);
                
                size_t posDist, posAngle;
                double distance = std::stod(distStr, &posDist);
                double angleDeg = std::stod(angleStr, &posAngle);
                
                if (posDist == distStr.length() && posAngle == angleStr.length()) {
                    // 角度转换为弧度
                    double angleRad = glm::radians(angleDeg);
                    glm::dvec3 offset(distance * cos(angleRad), distance * sin(angleRad), 0.0);
                    if (isRelative) {
                        // 相对极坐标@dis<angle：从基点计算
                        m_pickedPoint = basePoint + offset;
                    } else {
                        // 绝对极坐标dis<angle：从原点计算
                        m_pickedPoint = offset;
                    }
                    m_currentStatus = InputStatus::kPointInput;
                    Utils::cmdLinePrint(inputPrompt);
                    return;
                }
            } catch (...) {
                // 解析失败，不是合法极坐标输入
            }
        }
        
        // 尝试解析为笛卡尔坐标: x,y 或 @x,y
        size_t pos = coordInput.find(',');
        if (pos != std::string::npos && pos > 0 && pos < coordInput.length() - 1) {
            try {
                std::string xStr = coordInput.substr(0, pos);
                std::string yStr = coordInput.substr(pos + 1);
                
                size_t posX, posY;
                double x = std::stod(xStr, &posX);
                double y = std::stod(yStr, &posY);
                
                if (posX == xStr.length() && posY == yStr.length()) {
                    if (isRelative) {
                        // 相对笛卡尔坐标@x,y：从基点计算
                        m_pickedPoint = basePoint + glm::dvec3(x, y, 0.0);
                    } else {
                        // 绝对笛卡尔坐标x,y
                        m_pickedPoint = glm::dvec3(x, y, 0.0);
                    }
                    m_currentStatus = InputStatus::kPointInput;
                    Utils::cmdLinePrint(inputPrompt);
                    return;
                }
            } catch (...) {
                // 解析失败，不是合法笛卡尔坐标输入
            }
        }
        
        // 直接距离输入（沿光标方向）：简化版的相对极坐标，角度使用光标预览点方向
        if (!isRelative) {
            try {
                double distance = stod(coordInput);
                m_pickedPoint = basePoint + glm::normalize(getPreviewPoint() - basePoint) * distance;
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

// 处理Enter/Space输入
void InputContext::handleEnterSpace(const std::string& input) {
    // 选择或者命令执行中，解析输入
    if (m_selectionTask.isSelecting() || m_bInCommandExecution) {
        parseInput(input);
    }
    // 什么任务、命令都没有在执行中，则解释为新命令去执行
    else {
        // 输入为空则执行上一条有效命令
        if (input.empty())
        {
            CommandManager::getInstance().executeLastCommand();
        }
        // 不为空则解析为新命令，输出提示之类的操作会在executeCommand中做
        else {
            CommandManager::getInstance().executeCommand(input);
        }
    }
}

// 处理Escape输入
void InputContext::handleEscape(const std::string& input) {
    if (m_selectionTask.isSelecting() || m_bInCommandExecution) {
        // 选择或者命令执行中，设置取消状态
        m_currentStatus = InputStatus::kCanceled;
        // 更新命令提示，添加取消标记和用户输入
        auto& loc = LocalizationManager::getInstance();
        std::string cancelPrompt = (m_prompt.empty() ? "" : m_prompt + " ") + input + loc.get("commandLine.prompt.cancel");
        Utils::cmdLinePrint(cancelPrompt);
    }
    else {
        // 没有命令执行、没有选择任务时按下Esc，清空先选选择集
        clearPriorSelectionSet();
        // 键入的字符串也会被输出到命令历史
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
void InputContext::waitForPoint(const std::string& prompt, const glm::dvec3& basePoint, const std::vector<std::string>& keywords, bool drawRubberBand) {
    setPrompt(prompt);
    setKeywordOptions(keywords);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kPoint, InputType::kKeyword});
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

    // 设置橡皮线
    m_bDrawRubberBand = drawRubberBand;
    if (drawRubberBand) {
        m_interactionData.updateRubberBand(basePoint, basePoint);
    }
    
    // 选择模式下由选择任务指定光标，不为点输入单独设置光标，除此之外点输入都设置为十字光标
    if (!m_selectionTask.isSelecting()) {
        m_interactionData.cursorMode = CursorMode::kCrosshair;
    }
}

// 等待点输入（无基点）
void InputContext::waitForPoint(const std::string& prompt, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    setKeywordOptions(keywords);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kPoint, InputType::kKeyword});
    }
    else {
        setAllowedTypes({InputType::kPoint});
    }
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidPoint"); // *无效点*
    // 没有基点
    m_bHasBasePoint = false;
    
    // 选择模式下由选择任务指定光标，不为点输入单独设置光标，除此之外点输入都设置为十字光标
    if (!m_selectionTask.isSelecting()) {
        m_interactionData.cursorMode = CursorMode::kCrosshair;
    }
}

// 等待数值输入
void InputContext::waitForNumber(const std::string& prompt, double min, double max) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kInteger, InputType::kFloat});
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.needNumber"); // 需要输入数值。
    
    // 数值范围
    m_floatLimitMin = min;
    m_floatLimitMax = max;
    
    // 设置光标模式为十字光标
    m_interactionData.cursorMode = CursorMode::kCrosshair;
}

// 等待数值输入，同时允许关键字
void InputContext::waitForNumber(const std::string& prompt, double min, double max, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    setKeywordOptions(keywords);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kInteger, InputType::kFloat, InputType::kKeyword});
    }
    else {
        setAllowedTypes({InputType::kInteger, InputType::kFloat});
    }
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.needNumber"); // 需要输入数值。
    
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
    std::string errorTemplate = loc.get("inputContext.generalErrorPrompt.needInteger"); // 需要输入{0}到{1}的整数
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
    setKeywordOptions(keywords);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kInteger, InputType::kKeyword});
    }
    else {
        setAllowedTypes({InputType::kInteger});
    }
    // 设置错误提示 - 从本地化资源加载并格式化
    auto& loc = LocalizationManager::getInstance();
    std::string errorTemplate = loc.get("inputContext.generalErrorPrompt.needInteger"); // 需要输入{0}到{1}的整数
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
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.needNumber"); // 需要输入数值。
    
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
void InputContext::waitForKeyword(const std::string& prompt, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    setAllowedTypes({InputType::kKeyword});
    setKeywordOptions(keywords);
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

// 等待选择交互
void InputContext::waitForSelection(const std::string& prompt, const std::vector<std::string>& keywords) {
    setPrompt(prompt);
    setKeywordOptions(keywords);
    if (!keywords.empty()) {
        setAllowedTypes({InputType::kEntitySelection, InputType::kKeyword});
    }
    else {
        setAllowedTypes({InputType::kEntitySelection});
    }
    // 设置错误提示 - 从本地化资源加载
    auto& loc = LocalizationManager::getInstance();
    m_errorPrompt = loc.get("inputContext.generalErrorPrompt.invalidSelection"); // *无效选择*
    
    // 设置光标模式为拾取框
    m_interactionData.cursorMode = CursorMode::kPickbox;
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
               m_bInCommandExecution ? "true" : "false");
               
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
    
    // 基点（如果有）
    if (m_bHasBasePoint) {
        ImGui::Text("%s: (%.2f, %.2f, %.2f)",
                   loc.get("window.inputContextInfo.basePoint").c_str(),
                   m_basePoint.x, m_basePoint.y, m_basePoint.z);
    }

    // 预览点
    glm::dvec3 previewPoint = getPreviewPoint();
    ImGui::Text("%s: (%.2f, %.2f, %.2f)",
               loc.get("window.inputContextInfo.previewPoint").c_str(),
               previewPoint.x, previewPoint.y, previewPoint.z);

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
    
    // 橡皮线数据
    ImGui::Text("  %s: %s",
               loc.get("window.inputContextInfo.rubberBandVisible").c_str(),
               m_interactionData.isRubberBandVisible ? "true" : "false");
    if (m_interactionData.isRubberBandVisible) {
        ImGui::Text("    %s: (%.2f, %.2f, %.2f)",
                   loc.get("window.inputContextInfo.rubberBandStart").c_str(),
                   m_interactionData.rubberBandStartWorld.x,
                   m_interactionData.rubberBandStartWorld.y,
                   m_interactionData.rubberBandStartWorld.z);
        ImGui::Text("    %s: (%.2f, %.2f, %.2f)",
                   loc.get("window.inputContextInfo.rubberBandEnd").c_str(),
                   m_interactionData.rubberBandEndWorld.x,
                   m_interactionData.rubberBandEndWorld.y,
                   m_interactionData.rubberBandEndWorld.z);
    }
    
    ImGui::End();
}

// 获取交互数据
InteractionData& InputContext::getInteractionData() {
    return m_interactionData;
}

// 正交模式是否激活（数据库ORTHOMODE与Shift状态共同决定，且需要基点才有效）
bool InputContext::isOrthoActive() const {
    // 检查是否有基点，没有基点正交模式无效
    if (!m_bHasBasePoint) {
        return false;
    }

    // 获取数据库ORTHOMODE设置
    Database* pDb = DocManager::getCurrentDocument().getDatabase();
    if (!pDb) {
        return false;
    }
    bool orthoMode = pDb->orthoMode();

    // Shift状态临时反转正交模式
    bool shiftHolding = InputHandler::isShiftHolding();
    bool effectiveOrtho = orthoMode ^ shiftHolding;

    return effectiveOrtho;
}

// 将点约束到正交方向（水平或垂直）
static glm::dvec3 constrainToOrtho(const glm::dvec3& point, const glm::dvec3& basePoint) {
    glm::dvec3 delta = point - basePoint;
    double dx = std::abs(delta.x);
    double dy = std::abs(delta.y);

    // 判断更接近水平还是垂直
    if (dx > dy) {
        // 水平方向，保持x，y与基点相同
        return glm::dvec3(point.x, basePoint.y, point.z);
    } else {
        // 垂直方向，保持y，x与基点相同
        return glm::dvec3(basePoint.x, point.y, point.z);
    }
}

// 获取实时预览点的世界坐标
glm::dvec3 InputContext::getPreviewPoint() const {
    // 获取当前鼠标屏幕坐标
    glm::dvec2 screenPos = InputHandler::getCursorPosition();
    // 转换为世界坐标
    glm::dvec3 worldPos = DocManager::getCurrentDocument().getTransformManager().screenToWorld(screenPos);

    // 如果正交模式激活，约束到正交方向
    if (isOrthoActive()) {
        worldPos = constrainToOrtho(worldPos, m_basePoint);
    }

    return worldPos;
}

// 设置LastPoint
void InputContext::setLastPoint(const glm::dvec3& point) {
    DocManager::getCurrentDocument().setLastPoint(point);
}

// 获取LastPoint
glm::dvec3 InputContext::getLastPoint() const {
    return DocManager::getCurrentDocument().getLastPoint();
}

// 更新输入上下文
void InputContext::onUpdate() {
    // 如果调用了waitForSelection来选择实体
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
            // 获取选择结果
            SelectionSet result = m_selectionTask.getSelectionResult();
            // 如果在命令执行中
            if (m_bInCommandExecution) {
                // 保存选择结果，命令层通过getSelectionSet来获取时移交给命令层
                m_selectionResult = std::move(result);
                // 如果任务返回了取消状态
                if (m_selectionTask.getInputStatus() == InputStatus::kCanceled) {
                    m_currentStatus = InputStatus::kCanceled;
                    m_selectionResult.clear();
                } else {
                    m_currentStatus = InputStatus::kEntitySelection;
                }
            }
            // 非命令执行中，选到了实体，则合并到先选选择集中
            else if (m_selectionTask.getInputStatus() == InputStatus::kEntitySelection) {
                addToPriorSelectionSet(result);
            }
            // 重置选择任务
            m_selectionTask.reset();
        }
    }
    
    // 更新橡皮线（起点为基点，终点为鼠标位置）
    if (m_bDrawRubberBand) {
        m_interactionData.updateRubberBand(m_basePoint, getPreviewPoint());
    }

    // 更新正交光标标记
    if (isOrthoActive()) {
        // 正交模式激活，设置正交标记
        m_interactionData.cursorMarker = CursorMarker::kOrthogonal;
    } else if (m_interactionData.cursorMarker == CursorMarker::kOrthogonal) {
        // 非正交模式且当前是正交标记，清除标记
        m_interactionData.cursorMarker = CursorMarker::kNone;
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

// ============================================================================
// 选择集相关接口
// ============================================================================

// 获取当前文档的图形缓存
static IGraphicsDataCache* getCurrentGraphicsCache() {
    return DocManager::getCurrentDocument().getGraphicsDataCache();
}

// 设置先选选择集并更新高亮
void InputContext::setPriorSelectionSet(const SelectionSet& selectionSet) {
    // 取消旧的高亮
    if (auto* pCache = getCurrentGraphicsCache()) {
        for (ObjectId id : m_priorSelectionSet) {
            pCache->onEntityUnSelected(id);
        }
    }
    // 设置新的选择集
    m_priorSelectionSet = selectionSet;
    // 高亮新的选择集
    if (auto* pCache = getCurrentGraphicsCache()) {
        for (ObjectId id : m_priorSelectionSet) {
            pCache->onEntitySelected(id);
        }
    }
}

// 添加到先选选择集并高亮新增
void InputContext::addToPriorSelectionSet(const SelectionSet& selectionSet) {
    // 只高亮新增的实体
    SelectionSet newIds = selectionSet - m_priorSelectionSet;
    if (auto* pCache = getCurrentGraphicsCache()) {
        for (ObjectId id : newIds) {
            pCache->onEntitySelected(id);
        }
    }
    // 合并到先选选择集
    m_priorSelectionSet += selectionSet;
}

// 清空先选选择集并取消高亮
void InputContext::clearPriorSelectionSet() {
    // 取消高亮
    if (auto* pCache = getCurrentGraphicsCache()) {
        for (ObjectId id : m_priorSelectionSet) {
            pCache->onEntityUnSelected(id);
        }
    }
    m_priorSelectionSet.clear();
}

// 选中高亮选择集中的所有实体（提供给命令层调用）
void InputContext::highlightSelectionSet(const SelectionSet& selectionSet) {
    if (auto* pCache = getCurrentGraphicsCache()) {
        for (ObjectId id : selectionSet) {
            pCache->onEntitySelected(id);
        }
    }
}

// 取消选择集中所有实体的选中高亮（提供给命令层调用）
void InputContext::unhighlightSelectionSet(const SelectionSet& selectionSet) {
    if (auto* pCache = getCurrentGraphicsCache()) {
        for (ObjectId id : selectionSet) {
            pCache->onEntityUnSelected(id);
        }
    }
}

// 暗显选择集中的所有实体（命令层临时暗显，如 Move 命令激活时暗显源实体）
void InputContext::dimSelectionSet(const SelectionSet& selectionSet) {
    if (auto* pCache = getCurrentGraphicsCache()) {
        for (ObjectId id : selectionSet) {
            pCache->onEntityTempDimmed(id);
        }
    }
}

// 取消选择集中所有实体的暗显（提供给命令层调用）
void InputContext::undimSelectionSet(const SelectionSet& selectionSet) {
    if (auto* pCache = getCurrentGraphicsCache()) {
        for (ObjectId id : selectionSet) {
            pCache->onEntityUnTempDimmed(id);
        }
    }
}

} // namespace tch
