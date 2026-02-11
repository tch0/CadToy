#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <functional>
#include <unordered_map>

namespace tch {

// 输入事件类型
enum class InputEventType {
    KEY_PRESS,      // 按键按下
    KEY_RELEASE,    // 按键释放
    MOUSE_PRESS,    // 鼠标按下
    MOUSE_RELEASE,  // 鼠标释放
    MOUSE_MOVE,     // 鼠标移动
    MOUSE_SCROLL,   // 鼠标滚轮
    COMMAND_ENTERED // 命令输入
};

// 输入处理器类
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
    
    // 处理窗口大小变化
    static void handleWindowSize(int width, int height);
    
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
    static glm::vec2 s_mousePosition;                              // 鼠标位置
    static bool s_mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];         // 鼠标按钮状态
    static bool s_keys[GLFW_KEY_LAST + 1];                         // 键盘按键状态
    static std::string s_commandInput;                             // 命令输入
    static std::unordered_map<InputEventType, std::function<void()>> s_callbacks; // 事件回调映射
};

} // namespace tch