#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <functional>
#include <unordered_map>

namespace tch {

// 输入事件类型
enum class InputEventType {
    KEY_PRESS,
    KEY_RELEASE,
    MOUSE_PRESS,
    MOUSE_RELEASE,
    MOUSE_MOVE,
    MOUSE_SCROLL,
    COMMAND_ENTERED
};

// 输入处理器
class InputHandler {
public:
    // 初始化输入处理器
    static void initialize(GLFWwindow* window);
    
    // 处理键盘输入
    static void handleKeyPress(int key, int scancode, int action, int mods);
    
    // 处理鼠标输入
    static void handleMousePress(int button, int action, int mods);
    
    // 处理鼠标移动
    static void handleMouseMove(double xpos, double ypos);
    
    // 处理鼠标滚轮
    static void handleMouseScroll(double xoffset, double yoffset);
    
    // 处理鼠标进入/离开窗口
    static void handleMouseEnter(int entered);
    
    // 获取鼠标位置
    static glm::vec2 getMousePosition();
    
    // 检查鼠标按钮是否按下
    static bool isMouseButtonPressed(int button);
    
    // 检查键盘按键是否按下
    static bool isKeyPressed(int key);
    
    // 获取命令输入
    static std::string getCommandInput();
    
    // 清除命令输入
    static void clearCommandInput();
    
    // 注册事件回调
    static void registerCallback(InputEventType eventType, const std::function<void()>& callback);
    
    // 取消注册事件回调
    static void unregisterCallback(InputEventType eventType);
    
    // 设置鼠标指针可见性
    static void setMouseCursorVisible(bool visible);

private:
    // 窗口指针
    static GLFWwindow* s_window;
    // 静态成员变量
    static glm::vec2 s_mousePosition;
    static bool s_mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
    static bool s_keys[GLFW_KEY_LAST + 1];
    static std::string s_commandInput;
    static std::unordered_map<InputEventType, std::function<void()>> s_callbacks;
};

} // namespace tch