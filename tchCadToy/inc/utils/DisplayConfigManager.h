#pragma once

// C++ 标准库
#include <string>
#include <unordered_map>

// 第三方库
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace tch {

// 显示器配置管理器（单例）
// 管理每个显示器的独立配置，基于显示器分辨率+DPI作为Key
class DisplayConfigManager {
public:
    // 获取单例实例
    static DisplayConfigManager& getInstance();
    
    // 禁止拷贝和赋值
    DisplayConfigManager(const DisplayConfigManager&) = delete;
    DisplayConfigManager& operator=(const DisplayConfigManager&) = delete;
    
    // 初始化并注册窗口
    void initialize(GLFWwindow* window);
    
    // 获取当前窗口的字体大小
    int getCurrentFontSize() const { return m_currentFontSize; }
    
    // 设置窗口字体大小，范围18-50
    void setFontSize(int fontSize);
    
    // 析构时自动保存配置
    ~DisplayConfigManager();

private:
    // 私有构造函数
    DisplayConfigManager();
    
    // 配置Key结构（基于显示器分辨率 + DPI）
    struct ConfigKey {
        int screenWidth;   // 显示器宽度（物理分辨率）
        int screenHeight;  // 显示器高度（物理分辨率）
        float dpiScale;    // DPI缩放比例（精确到两位小数）
        
        bool operator==(const ConfigKey& other) const {
            return screenWidth == other.screenWidth && 
                   screenHeight == other.screenHeight && 
                   std::abs(dpiScale - other.dpiScale) < 0.01f;
        }
    };
    
    // ConfigKey哈希函数
    struct ConfigKeyHash {
        size_t operator()(const ConfigKey& key) const {
            return ((std::hash<int>()(key.screenWidth) ^ 
                     (std::hash<int>()(key.screenHeight) << 1)) >> 1) ^
                     (std::hash<float>()(key.dpiScale) << 1);
        }
    };
    
    // 生成配置Key（基于显示器信息）
    ConfigKey generateConfigKey(GLFWwindow* window);
    
    // 获取当前显示器信息（分辨率 + DPI）
    void getCurrentMonitorInfo(GLFWwindow* window, int& screenWidth, int& screenHeight, float& dpiScale);
    
    // 获取窗口中心点所在的显示器
    GLFWmonitor* getCurrentMonitor(GLFWwindow* window);
    
    // 获取显示器分辨率
    void getMonitorSize(GLFWmonitor* monitor, int& width, int& height);
    
    // 获取当前窗口的DPI缩放（精确到两位小数）
    float getCurrentDPIScale(GLFWwindow* window);
    
    // 生成默认字体大小（18 * DPI，向下取整，最小18最大50）
    int generateDefaultFontSize(float dpiScale);
    
    // 读取配置文件
    void loadFromFile();
    
    // 保存配置文件
    void saveToFile();
    
    // 应用配置（设置字体大小到ImGui）
    void applyConfig(int fontSize);
    
    // 处理配置变化（显示器或DPI改变时调用）
    void onConfigChanged();
    
    // GLFW回调函数（静态）
    static void onWindowPosCallback(GLFWwindow* window, int xpos, int ypos);           // 窗口移动，可能切换显示器
    static void onWindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale); // 系统DPI变化
    
    // 成员变量
    std::unordered_map<ConfigKey, int, ConfigKeyHash> m_configs;    // 配置映射表
    std::string m_configFilePath;                                   // 配置文件路径
    GLFWwindow* m_window;                                           // 关联的GLFW窗口
    int m_currentFontSize;                                          // 当前字体大小
    bool m_initialized;                                             // 是否已初始化
    bool m_dirty;                                                   // 配置是否已修改，需要保存
};

} // namespace tch