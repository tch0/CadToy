#include "input/InputHandler.h"
#include "render/Renderer.h"
#include <iostream>

namespace tch {

// 静态成员初始化
GLFWwindow* InputHandler::s_window = nullptr;
glm::vec2 InputHandler::s_mousePosition(0.0f, 0.0f);
bool InputHandler::s_mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1] = {false};
bool InputHandler::s_keys[GLFW_KEY_LAST + 1] = {false};
std::string InputHandler::s_commandInput = "";
std::unordered_map<InputEventType, std::function<void()>> InputHandler::s_callbacks;

// 初始化输入处理器
void InputHandler::initialize(GLFWwindow* window) {
    // 保存窗口指针
    s_window = window;
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
    
    // 鼠标进入/离开窗口的光标显示由ImGui控制，不再需要单独处理
    // glfwSetCursorEnterCallback(window, [](GLFWwindow* window, int entered) {
    //     InputHandler::handleMouseEnter(entered);
    // });
    
    // 注册窗口大小变化回调
    glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        InputHandler::handleWindowSize(width, height);
    });
}

// 处理键盘输入
void InputHandler::handleKeyPress(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        s_keys[key] = true;
        
        // 处理命令输入
        if (key >= GLFW_KEY_SPACE && key <= GLFW_KEY_GRAVE_ACCENT) {
            s_commandInput += static_cast<char>(key);
        } else if (key == GLFW_KEY_BACKSPACE && !s_commandInput.empty()) {
            s_commandInput.pop_back();
        } else if (key == GLFW_KEY_ENTER) {
            std::cout << "Command entered: " << s_commandInput << std::endl;
            // 触发命令输入回调
            if (s_callbacks.contains(InputEventType::COMMAND_ENTERED)) {
                s_callbacks[InputEventType::COMMAND_ENTERED]();
            }
            // 这里可以添加命令执行逻辑
            s_commandInput.clear();
        }
        
        // 触发按键按下回调
        if (s_callbacks.contains(InputEventType::KEY_PRESS)) {
            s_callbacks[InputEventType::KEY_PRESS]();
        }
    } else if (action == GLFW_RELEASE) {
        s_keys[key] = false;
        
        // 触发按键释放回调
        if (s_callbacks.contains(InputEventType::KEY_RELEASE)) {
            s_callbacks[InputEventType::KEY_RELEASE]();
        }
    }
}

// 处理鼠标输入
void InputHandler::handleMousePress(int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        s_mouseButtons[button] = true;
        
        // 触发鼠标按下回调
        if (s_callbacks.contains(InputEventType::MOUSE_PRESS)) {
            s_callbacks[InputEventType::MOUSE_PRESS]();
        }
    } else if (action == GLFW_RELEASE) {
        s_mouseButtons[button] = false;
        
        // 触发鼠标释放回调
        if (s_callbacks.contains(InputEventType::MOUSE_RELEASE)) {
            s_callbacks[InputEventType::MOUSE_RELEASE]();
        }
    }
}

// 处理鼠标移动
void InputHandler::handleMouseMove(double xpos, double ypos) {
    // 使用标准鼠标坐标系（Y轴向下，左上角为原点）
    s_mousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
    
    // 触发鼠标移动回调
    if (s_callbacks.contains(InputEventType::MOUSE_MOVE)) {
        s_callbacks[InputEventType::MOUSE_MOVE]();
    }
}

// 处理鼠标滚轮
void InputHandler::handleMouseScroll(double xoffset, double yoffset) {
    // 获取当前鼠标位置
    glm::vec2 mousePos = getMousePosition();
    
    // 根据滚轮方向进行缩放（反转方向：向上滚放大，向下滚缩小）
    if (yoffset > 0) {
        // 滚轮向上，缩小栅格
        Renderer::zoomOut(mousePos);
    } else if (yoffset < 0) {
        // 滚轮向下，放大栅格
        Renderer::zoomIn(mousePos);
    }
    
    // 触发鼠标滚轮回调
    if (s_callbacks.contains(InputEventType::MOUSE_SCROLL)) {
        s_callbacks[InputEventType::MOUSE_SCROLL]();
    }
}

// 处理鼠标进入/离开窗口
void InputHandler::handleMouseEnter(int entered) {
    if (entered) {
        // 鼠标进入窗口，隐藏系统光标
        setMouseCursorVisible(false);
    } else {
        // 鼠标离开窗口，显示系统光标
        setMouseCursorVisible(true);
    }
}

// 处理窗口大小变化
void InputHandler::handleWindowSize(int width, int height) {
    // 当窗口大小变化时，更新渲染器的视口大小
    Renderer::updateViewport(width, height);
    
    // 计算窗口中心坐标
    float centerX = width / 2.0f;
    float centerY = height / 2.0f;
    
    // 更新坐标原点为窗口中心
    Renderer::setOrigin(centerX, centerY);
}

// 获取鼠标位置
glm::vec2 InputHandler::getMousePosition() {
    return s_mousePosition;
}

// 检查鼠标按钮是否按下
bool InputHandler::isMouseButtonPressed(int button) {
    if (button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST) {
        return s_mouseButtons[button];
    }
    return false;
}

// 检查键盘按键是否按下
bool InputHandler::isKeyPressed(int key) {
    if (key >= 0 && key <= GLFW_KEY_LAST) {
        return s_keys[key];
    }
    return false;
}

// 获取命令输入
std::string InputHandler::getCommandInput() {
    return s_commandInput;
}

// 清除命令输入
void InputHandler::clearCommandInput() {
    s_commandInput.clear();
}

// 注册事件回调
void InputHandler::registerCallback(InputEventType eventType, const std::function<void()>& callback) {
    s_callbacks[eventType] = callback;
}

// 取消注册事件回调
void InputHandler::unregisterCallback(InputEventType eventType) {
    s_callbacks.erase(eventType);
}

// 设置鼠标指针可见性
void InputHandler::setMouseCursorVisible(bool visible) {
    if (s_window) {
        if (visible) {
            glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
    }
}

} // namespace tch