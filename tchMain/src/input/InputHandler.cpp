#include "InputHandler.h"
#include <iostream>

namespace tch {

// 静态成员初始化
glm::vec2 InputHandler::m_mousePosition(0.0f, 0.0f);
bool InputHandler::m_mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1] = {false};
bool InputHandler::m_keys[GLFW_KEY_LAST + 1] = {false};
std::string InputHandler::m_commandInput = "";
std::unordered_map<InputEventType, std::function<void()>> InputHandler::m_callbacks;

// 初始化输入处理器
void InputHandler::initialize(GLFWwindow* window) {
    // 设置GLFW回调函数
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        InputHandler::handleKeyPress(key, scancode, action, mods);
    });
    
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        InputHandler::handleMousePress(button, action, mods);
    });
    
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        InputHandler::handleMouseMove(xpos, ypos);
    });
    
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
        InputHandler::handleMouseScroll(xoffset, yoffset);
    });
}

// 处理键盘输入
void InputHandler::handleKeyPress(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        m_keys[key] = true;
        
        // 处理命令输入
        if (key >= GLFW_KEY_SPACE && key <= GLFW_KEY_GRAVE_ACCENT) {
            m_commandInput += static_cast<char>(key);
        } else if (key == GLFW_KEY_BACKSPACE && !m_commandInput.empty()) {
            m_commandInput.pop_back();
        } else if (key == GLFW_KEY_ENTER) {
            std::cout << "Command entered: " << m_commandInput << std::endl;
            // 触发命令输入回调
            if (m_callbacks.contains(InputEventType::COMMAND_ENTERED)) {
                m_callbacks[InputEventType::COMMAND_ENTERED]();
            }
            // 这里可以添加命令执行逻辑
            m_commandInput.clear();
        }
        
        // 触发按键按下回调
        if (m_callbacks.contains(InputEventType::KEY_PRESS)) {
            m_callbacks[InputEventType::KEY_PRESS]();
        }
    } else if (action == GLFW_RELEASE) {
        m_keys[key] = false;
        
        // 触发按键释放回调
        if (m_callbacks.contains(InputEventType::KEY_RELEASE)) {
            m_callbacks[InputEventType::KEY_RELEASE]();
        }
    }
}

// 处理鼠标输入
void InputHandler::handleMousePress(int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        m_mouseButtons[button] = true;
        
        // 触发鼠标按下回调
        if (m_callbacks.contains(InputEventType::MOUSE_PRESS)) {
            m_callbacks[InputEventType::MOUSE_PRESS]();
        }
    } else if (action == GLFW_RELEASE) {
        m_mouseButtons[button] = false;
        
        // 触发鼠标释放回调
        if (m_callbacks.contains(InputEventType::MOUSE_RELEASE)) {
            m_callbacks[InputEventType::MOUSE_RELEASE]();
        }
    }
}

// 处理鼠标移动
void InputHandler::handleMouseMove(double xpos, double ypos) {
    m_mousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
    
    // 触发鼠标移动回调
    if (m_callbacks.contains(InputEventType::MOUSE_MOVE)) {
        m_callbacks[InputEventType::MOUSE_MOVE]();
    }
}

// 处理鼠标滚轮
void InputHandler::handleMouseScroll(double xoffset, double yoffset) {
    // 触发鼠标滚轮回调
    if (m_callbacks.contains(InputEventType::MOUSE_SCROLL)) {
        m_callbacks[InputEventType::MOUSE_SCROLL]();
    }
}

// 获取鼠标位置
glm::vec2 InputHandler::getMousePosition() {
    return m_mousePosition;
}

// 检查鼠标按钮是否按下
bool InputHandler::isMouseButtonPressed(int button) {
    if (button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST) {
        return m_mouseButtons[button];
    }
    return false;
}

// 检查键盘按键是否按下
bool InputHandler::isKeyPressed(int key) {
    if (key >= 0 && key <= GLFW_KEY_LAST) {
        return m_keys[key];
    }
    return false;
}

// 获取命令输入
std::string InputHandler::getCommandInput() {
    return m_commandInput;
}

// 清除命令输入
void InputHandler::clearCommandInput() {
    m_commandInput.clear();
}

// 注册事件回调
void InputHandler::registerCallback(InputEventType eventType, const std::function<void()>& callback) {
    m_callbacks[eventType] = callback;
}

// 取消注册事件回调
void InputHandler::unregisterCallback(InputEventType eventType) {
    m_callbacks.erase(eventType);
}

} // namespace tch