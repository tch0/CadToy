#include "input/InputContext.h"
#include "render/Renderer.h"
#include "command/CommandManager.h"
#include <glm/glm.hpp>
#include <memory>

namespace tch {

// 静态实例
std::shared_ptr<InputContext> InputContext::s_instance = nullptr;

// 构造函数
InputContext::InputContext() :
    m_inCommandExecution(false),
    m_shouldAbortCommand(false),
    m_currentStatus(InputStatus::kNone),
    m_allowedTypes(),
    m_prompt(""),
    m_pickedPoint(glm::dvec3(0, 0, 0)),
    m_inputInteger(0),
    m_inputFloat(0.0),
    m_inputString(""),
    m_inputKeyword(""),
    m_keywordOptions() {
}

// 获取单例实例
InputContext& InputContext::getInstance() {
    if (s_instance == nullptr) {
        s_instance = std::make_shared<InputContext>();
    }
    return *s_instance;
}

// 命令执行状态管理
void InputContext::setInCommandExecution(bool inExecution) {
    m_inCommandExecution = inExecution;
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
InputStatus InputContext::getCurrentStatus() const {
    return m_currentStatus;
}

void InputContext::resetStatus() {
    m_currentStatus = InputStatus::kNone;
    m_shouldAbortCommand = false;
    m_allowedTypes.clear();
    m_prompt = "";
    m_pickedPoint = glm::dvec3(0, 0, 0);
    m_inputInteger = 0;
    m_inputFloat = 0.0;
    m_inputString = "";
    m_inputKeyword = "";
    m_keywordOptions.clear();
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
        return true;
    }
    return false;
}

// 输入解析
void InputContext::parseInput(const std::string& input) {
    // 重置当前状态
    m_currentStatus = InputStatus::kNone;
    
    // 检查是否为空输入
    if (input.empty()) {
        return;
    }
    
    // 检查是否是取消命令
    if (input == "cancel" || input == "esc" || input == "q") {
        m_currentStatus = InputStatus::kCanceled;
        return;
    }
    
    // 检查是否是关键字（如果允许关键字输入）
    if (std::find(m_allowedTypes.begin(), m_allowedTypes.end(), InputType::kKeyword) != m_allowedTypes.end()) {
        for (const auto& option : m_keywordOptions) {
            if (input == option) {
                m_inputKeyword = input;
                m_currentStatus = InputStatus::kKeywordInput;
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
        return;
    }
    
    // 没有匹配的类型
    m_currentStatus = InputStatus::kNone;
}

// 取消操作
void InputContext::cancel() {
    m_currentStatus = InputStatus::kCanceled;
}

bool InputContext::isCanceled() const {
    return m_currentStatus == InputStatus::kCanceled;
}

// 中止操作（强制取消整个命令）
void InputContext::abort() {
    m_shouldAbortCommand = true;
}

bool InputContext::shouldAbortCommand() const {
    return m_shouldAbortCommand;
}

// 预览功能（暂时空实现）
void InputContext::drawRubberBand(const glm::dvec3& startPoint) {
    // 暂时空实现
}

// 处理命令输入
void InputContext::handleCommandInput(const std::string& input) {
    if (m_inCommandExecution) {
        // 如果处于命令执行中，将输入作为命令参数解析
        parseInput(input);
    } else {
        // 如果不处于命令执行中，解析为新命令
        CommandManager::getInstance().parseCommand(input);
    }
}

} // namespace tch
