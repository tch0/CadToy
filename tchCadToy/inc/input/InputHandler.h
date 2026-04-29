#pragma once

// C++ 标准库
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// 第三方库
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// 项目头文件

namespace tch {

// 输入事件类型
enum class InputEventType {
    KEY_PRESS,      // 按键按下
    KEY_RELEASE,    // 按键释放
    MOUSE_PRESS,    // 鼠标按下
    MOUSE_RELEASE,  // 鼠标释放
    MOUSE_MOVE,     // 鼠标移动
    MOUSE_SCROLL    // 鼠标滚轮
};

// 快捷键结构
enum class ShortcutType {
    SINGLE_KEY,     // 单个按键
    CTRL_KEY,       // Ctrl+键
    CTRL_SHIFT_KEY  // Ctrl+Shift+键
};

// 快捷键项结构
struct ShortcutItem {
    int key;                    // 按键
    ShortcutType type;          // 快捷键类型
    std::string commandName;    // 命令名称
    std::string name;           // 快捷键名称
    std::string keyString;      // 按键字符串表示
};

// 输入处理器类
class InputHandler {
public:
    // 初始化输入处理器
    static void initialize(GLFWwindow* window);
    
    // 获取光标位置
    static glm::vec2 getCursorPosition();
    
    // 检查鼠标左键是否按下
    static bool isLeftMouseButtonPressed();
    
    // 检查Shift键是否按住（用于正交模式等需要持续检测的场景）
    static bool isShiftHolding();

private:
    // 处理键盘输入
    static void handleKeyPress(int key, int scancode, int action, int mods);
    
    // 处理字符输入
    static void handleCharInput(unsigned int codepoint);
    
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
    
    // 检查鼠标按钮是否按下
    static bool isMouseButtonPressed(int button);
    
    // 检查键盘按键是否按下
    static bool isKeyPressed(int key);
    
    // 注册事件回调
    static void registerCallback(InputEventType eventType, const std::function<void()>& callback);
    
    // 取消注册事件回调
    static void unregisterCallback(InputEventType eventType);
    
    // 设置鼠标指针可见性
    static void setMouseCursorVisible(bool visible);
    
    // 注册单个按键快捷键
    static void registerShortcut(int key, const std::string& commandName, const std::string& name, const std::string& keyString);
    
    // 注册Ctrl+键快捷键
    static void registerCtrlShortcut(int key, const std::string& commandName, const std::string& name, const std::string& keyString);
    
    // 注册Ctrl+Shift+键快捷键
    static void registerCtrlShiftShortcut(int key, const std::string& commandName, const std::string& name, const std::string& keyString);
    
    // 取消注册快捷键
    static void unregisterShortcut(int key, ShortcutType type);
    
    // 清除所有快捷键
    static void clearAllShortcuts();
    
    // 注册默认快捷键
    static void registerDefaultShortcuts();

    // 执行快捷键（统一入口，内部处理 Ctrl+Z 的特殊情况）
    static void executeShortcut(const ShortcutItem& shortcut);

private:
    // 窗口指针
    static GLFWwindow* s_window;
    
    // 静态成员变量
    static glm::vec2 s_cursorPosition;                              // 光标位置
    static glm::vec2 s_lastCursorPosition;                          // 上一次光标位置（用于拖动计算）
    static bool s_mouseMiddleButtonPressedInViewport;               // 鼠标中键是否在视口内按下
    static bool s_mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];         // 鼠标按钮状态
    static bool s_keys[GLFW_KEY_LAST + 1];                          // 键盘按键状态
    static bool s_keyWasConsumedByShortcut;                         // 标记当前按键是否已被快捷键逻辑消耗
    static std::unordered_map<InputEventType, std::function<void()>> s_callbacks; // 事件回调映射
    static std::vector<ShortcutItem> s_shortcuts;                   // 快捷键列表
};

} // namespace tch