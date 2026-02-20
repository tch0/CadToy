#include "input/InputHandler.h"
#include "render/Renderer.h"
#include "debug/Logger.h"
#include "command/CommandParser.h"
#include <imgui.h>

namespace tch {

// 静态成员初始化
GLFWwindow* InputHandler::s_window = nullptr;
glm::vec2 InputHandler::s_mousePosition(0.0f, 0.0f);
glm::vec2 InputHandler::s_lastMousePosition(0.0f, 0.0f); // 初始化上一次鼠标位置
bool InputHandler::s_mouseMiddleButtonPressedInDrawableArea = false; // 初始化鼠标中间按钮是否在可绘制区域内按下
bool InputHandler::s_mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1] = {false};
bool InputHandler::s_keys[GLFW_KEY_LAST + 1] = {false};
std::string InputHandler::s_commandInput = "";
std::unordered_map<InputEventType, std::function<void()>> InputHandler::s_callbacks;
std::vector<ShortcutItem> InputHandler::s_shortcuts;

// 注册默认快捷键
void InputHandler::registerDefaultShortcuts() {
    // Ctrl+N new
    registerCtrlShortcut(GLFW_KEY_N, "new", "New", "N");
    
    // Ctrl+O open
    registerCtrlShortcut(GLFW_KEY_O, "open", "Open", "O");
    
    // Ctrl+S save
    registerCtrlShortcut(GLFW_KEY_S, "save", "Save", "S");
    
    // Ctrl+Shift+S saveas
    registerCtrlShiftShortcut(GLFW_KEY_S, "saveas", "Save As", "S");
    
    // Ctrl+W close
    registerCtrlShortcut(GLFW_KEY_W, "close", "Close", "W");
    
    // Ctrl+Q quit
    registerCtrlShortcut(GLFW_KEY_Q, "exit", "Quit", "Q");
    
    // Ctrl+Z undo
    registerCtrlShortcut(GLFW_KEY_Z, "undo", "Undo", "Z");
    
    // Ctrl+Y redo
    registerCtrlShortcut(GLFW_KEY_Y, "redo", "Redo", "Y");
    
    // Ctrl+X cutclip
    registerCtrlShortcut(GLFW_KEY_X, "cut", "Cut", "X");
    
    // Ctrl+C copyclip
    registerCtrlShortcut(GLFW_KEY_C, "copy", "Copy", "C");
    
    // Ctrl+V pasteclip
    registerCtrlShortcut(GLFW_KEY_V, "paste", "Paste", "V");
    
    // Ctrl+A selectall
    registerCtrlShortcut(GLFW_KEY_A, "selectall", "Select All", "A");
    
    // DEL erase
    registerShortcut(GLFW_KEY_DELETE, "erase", "Erase", "Delete");
}

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
    
    // 注册默认快捷键
    registerDefaultShortcuts();
}

// 处理键盘输入
void InputHandler::handleKeyPress(int key, int scancode, int action, int mods) {
    // 如果ImGui需要键盘，就不处理键盘事件
    if (ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }
    
    if (action == GLFW_PRESS) {
        s_keys[key] = true;
        
        // 优先匹配按键更多的模式
        // 1. 检查Ctrl+Shift+键快捷键
        bool ctrlShiftPressed = (mods & GLFW_MOD_CONTROL) && (mods & GLFW_MOD_SHIFT);
        if (ctrlShiftPressed) {
            for (const auto& shortcut : s_shortcuts) {
                if (shortcut.type == ShortcutType::CTRL_SHIFT_KEY && shortcut.key == key) {
                    LOG_INFO("Executing shortcut: Ctrl+Shift+{} ({}, command: {})", shortcut.keyString, shortcut.name, shortcut.commandName);
                    CommandParser::executeCommand(shortcut.commandName, {});
                    // 触发按键按下回调
                    if (s_callbacks.contains(InputEventType::KEY_PRESS)) {
                        s_callbacks[InputEventType::KEY_PRESS]();
                    }
                    return;
                }
            }
        }
        
        // 2. 检查Ctrl+键快捷键
        bool ctrlPressed = (mods & GLFW_MOD_CONTROL);
        if (ctrlPressed && !ctrlShiftPressed) {
            for (const auto& shortcut : s_shortcuts) {
                if (shortcut.type == ShortcutType::CTRL_KEY && shortcut.key == key) {
                    LOG_INFO("Executing shortcut: Ctrl+{} ({}, command: {})", shortcut.keyString, shortcut.name, shortcut.commandName);
                    CommandParser::executeCommand(shortcut.commandName, {});
                    // 触发按键按下回调
                    if (s_callbacks.contains(InputEventType::KEY_PRESS)) {
                        s_callbacks[InputEventType::KEY_PRESS]();
                    }
                    return;
                }
            }
        }
        
        // 3. 检查单个按键快捷键
        bool noModifiers = (mods == 0);
        if (noModifiers) {
            for (const auto& shortcut : s_shortcuts) {
                if (shortcut.type == ShortcutType::SINGLE_KEY && shortcut.key == key) {
                    LOG_INFO("Executing shortcut: {} ({}, command: {})", shortcut.keyString, shortcut.name, shortcut.commandName);
                    CommandParser::executeCommand(shortcut.commandName, {});
                    // 触发按键按下回调
                    if (s_callbacks.contains(InputEventType::KEY_PRESS)) {
                        s_callbacks[InputEventType::KEY_PRESS]();
                    }
                    return;
                }
            }
            
            // 如果没有匹配的单个按键快捷键，处理命令输入
            if (key >= GLFW_KEY_SPACE && key <= GLFW_KEY_GRAVE_ACCENT) {
                s_commandInput += static_cast<char>(key);
            } else if (key == GLFW_KEY_BACKSPACE && !s_commandInput.empty()) {
                s_commandInput.pop_back();
            } else if (key == GLFW_KEY_ENTER) {
                LOG_INFO("Command entered: {}", s_commandInput);
                // 触发命令输入回调
                if (s_callbacks.contains(InputEventType::COMMAND_ENTERED)) {
                    s_callbacks[InputEventType::COMMAND_ENTERED]();
                }
                // 这里可以添加命令执行逻辑
                s_commandInput.clear();
            }
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
    // 如果ImGui需要鼠标，就不处理鼠标按下/释放事件
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    
    if (action == GLFW_PRESS) {
        s_mouseButtons[button] = true;
        
        // 当鼠标中间按钮被按下时，记录当前鼠标位置作为上一次位置
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            s_lastMousePosition = s_mousePosition;
            // 检查鼠标是否在可绘制区域内
            s_mouseMiddleButtonPressedInDrawableArea = Renderer::getLogicalViewport().isPointInDrawableArea(s_mousePosition);
        }
        
        // 触发鼠标按下回调
        if (s_callbacks.contains(InputEventType::MOUSE_PRESS)) {
            s_callbacks[InputEventType::MOUSE_PRESS]();
        }
    } else if (action == GLFW_RELEASE) {
        // 当鼠标中间按钮被释放时，重置标志
        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            s_mouseMiddleButtonPressedInDrawableArea = false;
        }
        
        s_mouseButtons[button] = false;
        
        // 触发鼠标释放回调
        if (s_callbacks.contains(InputEventType::MOUSE_RELEASE)) {
            s_callbacks[InputEventType::MOUSE_RELEASE]();
        }
    }
}

// 处理鼠标移动
void InputHandler::handleMouseMove(double xpos, double ypos) {
    // 如果ImGui需要鼠标，就不处理鼠标移动，也不更新鼠标位置
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    
    // 使用标准鼠标坐标系（Y轴向下，左上角为原点）
    glm::vec2 newMousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
    
    // 检查鼠标中间按钮是否被按下且按下时位于可绘制区域
    if (s_mouseButtons[GLFW_MOUSE_BUTTON_MIDDLE] && s_mouseMiddleButtonPressedInDrawableArea) {
        // 计算鼠标位移
        glm::vec2 deltaScreen = newMousePosition - s_lastMousePosition;
        
        // 即使鼠标不在可绘制区域内，只要按下时在可绘制区域内，就继续平移
        // 将屏幕位移转换为逻辑坐标
        // 计算位移的逻辑坐标：通过计算两个点的逻辑坐标之差
        glm::dvec3 logicPosNew = Renderer::getLogicalViewport().screenToLogic(newMousePosition);
        glm::dvec3 logicPosLast = Renderer::getLogicalViewport().screenToLogic(s_lastMousePosition);
        glm::dvec2 deltaLogic = glm::dvec2(logicPosNew.x - logicPosLast.x, logicPosNew.y - logicPosLast.y);
        
        // 调用Renderer的pan方法进行平移
        Renderer::pan(deltaLogic);
        
        // 更新上一次鼠标位置
        s_lastMousePosition = newMousePosition;
    }
    
    // 更新当前鼠标位置
    s_mousePosition = newMousePosition;
    
    // 触发鼠标移动回调
    if (s_callbacks.contains(InputEventType::MOUSE_MOVE)) {
        s_callbacks[InputEventType::MOUSE_MOVE]();
    }
}

// 处理鼠标滚轮
void InputHandler::handleMouseScroll(double xoffset, double yoffset) {
    // 如果ImGui需要鼠标，就不处理鼠标滚轮
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }
    
    // 获取当前鼠标位置
    glm::vec2 mousePos = getMousePosition();
    
    // 检查鼠标是否在可绘制区域内
    if (Renderer::getLogicalViewport().isPointInDrawableArea(mousePos)) {
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

// 注册单个按键快捷键
void InputHandler::registerShortcut(int key, const std::string& commandName, const std::string& name, const std::string& keyString) {
    // 先移除已存在的相同快捷键
    unregisterShortcut(key, ShortcutType::SINGLE_KEY);
    // 添加新的快捷键
    s_shortcuts.push_back({key, ShortcutType::SINGLE_KEY, commandName, name, keyString});
    LOG_INFO("Registered shortcut: {} (key: {}, keyString: {}, command: {})", name, key, keyString, commandName);
}

// 注册Ctrl+键快捷键
void InputHandler::registerCtrlShortcut(int key, const std::string& commandName, const std::string& name, const std::string& keyString) {
    // 先移除已存在的相同快捷键
    unregisterShortcut(key, ShortcutType::CTRL_KEY);
    // 添加新的快捷键
    s_shortcuts.push_back({key, ShortcutType::CTRL_KEY, commandName, name, keyString});
    LOG_INFO("Registered Ctrl shortcut: {} (key: {}, keyString: {}, command: {})", name, key, keyString, commandName);
}

// 注册Ctrl+Shift+键快捷键
void InputHandler::registerCtrlShiftShortcut(int key, const std::string& commandName, const std::string& name, const std::string& keyString) {
    // 先移除已存在的相同快捷键
    unregisterShortcut(key, ShortcutType::CTRL_SHIFT_KEY);
    // 添加新的快捷键
    s_shortcuts.push_back({key, ShortcutType::CTRL_SHIFT_KEY, commandName, name, keyString});
    LOG_INFO("Registered Ctrl+Shift shortcut: {} (key: {}, keyString: {}, command: {})", name, key, keyString, commandName);
}

// 取消注册快捷键
void InputHandler::unregisterShortcut(int key, ShortcutType type) {
    auto it = std::remove_if(s_shortcuts.begin(), s_shortcuts.end(),
        [key, type](const ShortcutItem& item) {
            return item.key == key && item.type == type;
        });
    if (it != s_shortcuts.end()) {
        std::string name = it->name;
        s_shortcuts.erase(it, s_shortcuts.end());
        LOG_INFO("Unregistered shortcut: {}", name);
    }
}

// 清除所有快捷键
void InputHandler::clearAllShortcuts() {
    LOG_INFO("Clearing all shortcuts");
    s_shortcuts.clear();
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