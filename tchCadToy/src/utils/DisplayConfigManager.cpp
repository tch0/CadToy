// 对应头文件
#include "utils/DisplayConfigManager.h"

// C++ 标准库
#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <imgui.h>

// 项目头文件
#include "debug/Logger.h"
#include "sys/Global.h"

namespace tch {

DisplayConfigManager& DisplayConfigManager::getInstance() {
    static DisplayConfigManager instance;
    return instance;
}

DisplayConfigManager::DisplayConfigManager()
    : m_window(nullptr)
    , m_currentFontSize(18)
    , m_initialized(false)
    , m_dirty(false) {
}

DisplayConfigManager::~DisplayConfigManager() {
    if (m_dirty) {
        saveToFile();
        LOG_INFO("DisplayConfigManager: Config saved on destruction");
    }
}

void DisplayConfigManager::initialize(GLFWwindow* window) {
    if (m_initialized) {
        LOG_WARNING("DisplayConfigManager: Already initialized");
        return;
    }
    
    if (!window) {
        LOG_ERROR("DisplayConfigManager: Invalid window handle");
        return;
    }
    
    // 构建配置文件路径
    m_configFilePath = (g_pathCwd / "config" / "DisplayConfig.json").string();
    
    // 创建config目录（如果不存在）
    std::filesystem::path configDir = g_pathCwd / "config";
    if (!std::filesystem::exists(configDir)) {
        std::filesystem::create_directories(configDir);
        LOG_INFO("DisplayConfigManager: Created config directory: {}", configDir.string());
    }
    
    // 读取配置文件
    loadFromFile();
    
    // 保存窗口句柄
    m_window = window;
    
    // 设置GLFW回调
    glfwSetWindowPosCallback(window, onWindowPosCallback);
    glfwSetWindowContentScaleCallback(window, onWindowContentScaleCallback);
    
    // 标记为已初始化（必须在调用 onConfigChanged 之前）
    m_initialized = true;
    
    // 立即应用当前配置
    onConfigChanged();
    LOG_INFO("DisplayConfigManager: Initialized with window, config file: {}", m_configFilePath);
}

// 设置窗口字体大小，范围18-50
void DisplayConfigManager::setFontSize(int fontSize, bool saveToConfig) {
    if (fontSize < 18) {
        fontSize = 18;
    }
    else if (fontSize > 50) {
        fontSize = 50;
    }
    
    
    if (saveToConfig && m_window) {
        ConfigKey key = generateConfigKey(m_window);
        // 如果字号和当前不同
        if (m_configs[key] != fontSize) {
            // 更新当前配置
            m_configs[key] = fontSize;
            m_dirty = true;
            LOG_INFO("DisplayConfigManager: User changed font size to {} for current config - screen: {}x{}, dpi: {:.2f}", 
                     fontSize, key.screenWidth, key.screenHeight, key.dpiScale);
            // 应用字体
            applyConfig(fontSize);
        }
    }
}

// 生成配置Key（基于显示器分辨率和DPI）
DisplayConfigManager::ConfigKey DisplayConfigManager::generateConfigKey(GLFWwindow* window) {
    ConfigKey key;
    getCurrentMonitorInfo(window, key.screenWidth, key.screenHeight, key.dpiScale);
    
    // DPI精确到两位小数
    key.dpiScale = std::round(key.dpiScale * 100) / 100.0f;
    return key;
}

// 获取窗口中心点所在的显示器
GLFWmonitor* DisplayConfigManager::getCurrentMonitor(GLFWwindow* window) {
    if (!window) {
        return glfwGetPrimaryMonitor();
    }
    
    // 获取窗口位置和大小
    int winX, winY, winW, winH;
    glfwGetWindowPos(window, &winX, &winY);
    glfwGetWindowSize(window, &winW, &winH);
    
    // 计算窗口中心点
    int centerX = winX + winW / 2;
    int centerY = winY + winH / 2;
    
    // 枚举所有显示器，找到包含中心点的那个
    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    
    for (int i = 0; i < monitorCount; ++i) {
        int monX, monY;
        glfwGetMonitorPos(monitors[i], &monX, &monY);
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
        
        if (centerX >= monX && centerX < monX + mode->width &&
            centerY >= monY && centerY < monY + mode->height) {
            return monitors[i];
        }
    }
    
    // 未找到则返回主显示器
    return glfwGetPrimaryMonitor();
}

// 获取显示器分辨率
void DisplayConfigManager::getMonitorSize(GLFWmonitor* monitor, int& width, int& height) {
    if (!monitor) {
        width = 1920;
        height = 1080;
        return;
    }
    
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (mode) {
        width = mode->width;
        height = mode->height;
    } else {
        width = 1920;
        height = 1080;
    }
}

// 获取当前窗口的DPI缩放（精确到两位小数）
float DisplayConfigManager::getCurrentDPIScale(GLFWwindow* window) {
    if (!window) {
        LOG_WARNING("DisplayConfigManager: No window, returning 1.0f");
        return 1.0f;
    }
    
    GLFWmonitor* monitor = getCurrentMonitor(window);
    
    float xscale, yscale;
    glfwGetMonitorContentScale(monitor, &xscale, &yscale);
    
    // 有效性检查，确保 >= 1.0
    float dpiScale = (xscale >= 0.95f && xscale <= 4.0f) ? std::max(1.0f, xscale) : 1.0f;
    // 精确到两位小数
    dpiScale = std::round(dpiScale * 100) / 100.0f;
    
    return dpiScale;
}

// 获取当前显示器信息（分辨率 + DPI）
void DisplayConfigManager::getCurrentMonitorInfo(GLFWwindow* window, int& screenWidth, int& screenHeight, float& dpiScale) {
    GLFWmonitor* monitor = getCurrentMonitor(window);
    getMonitorSize(monitor, screenWidth, screenHeight);
    dpiScale = getCurrentDPIScale(window);
}

// 生成默认字体大小：18 * DPI，向下取整，最小18
int DisplayConfigManager::generateDefaultFontSize(float dpiScale) {
    int fontSize = static_cast<int>(std::floor(18.0f * dpiScale));
    fontSize = std::max(18, fontSize);
    fontSize = std::min(50, fontSize);
    
    LOG_TRACE("DisplayConfigManager: Generated default font size: {} (dpi: {:.2f})", 
              fontSize, dpiScale);
    
    return fontSize;
}

// 读取配置文件
void DisplayConfigManager::loadFromFile() {
    std::ifstream file(m_configFilePath);
    if (!file.is_open()) {
        LOG_INFO("DisplayConfigManager: Config file not found, will create on save: {}", 
                 m_configFilePath);
        return;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    rapidjson::Document doc;
    if (doc.Parse(content.c_str()).HasParseError()) {
        LOG_ERROR("DisplayConfigManager: Failed to parse config file: {}", m_configFilePath);
        return;
    }
    
    if (!doc.IsObject()) {
        LOG_ERROR("DisplayConfigManager: Config file root is not an object");
        return;
    }
    
    int loadedCount = 0;
    for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
        if (!it->value.IsInt()) {
            LOG_WARNING("DisplayConfigManager: Skipping invalid config entry: {}", it->name.GetString());
            continue;
        }
        
        // 解析Key格式: "1920x1080_1.25"
        std::string keyStr = it->name.GetString();
        ConfigKey key;
        
        size_t underscorePos = keyStr.find('_');
        if (underscorePos == std::string::npos) {
            LOG_WARNING("DisplayConfigManager: Invalid key format: {}", keyStr);
            continue;
        }
        
        std::string sizeStr = keyStr.substr(0, underscorePos);
        std::string dpiStr = keyStr.substr(underscorePos + 1);
        
        size_t xPos = sizeStr.find('x');
        if (xPos == std::string::npos) {
            LOG_WARNING("DisplayConfigManager: Invalid size format: {}", sizeStr);
            continue;
        }
        
        try {
            key.screenWidth = std::stoi(sizeStr.substr(0, xPos));
            key.screenHeight = std::stoi(sizeStr.substr(xPos + 1));
            key.dpiScale = std::stof(dpiStr);
        } catch (const std::exception& e) {
            LOG_WARNING("DisplayConfigManager: Failed to parse config key: {} - {}", keyStr, e.what());
            continue;
        }
        
        int fontSize = it->value.GetInt();
        m_configs[key] = fontSize;
        loadedCount++;
        
        LOG_TRACE("DisplayConfigManager: Loaded config - {}x{} {:.2f} -> {}", 
                  key.screenWidth, key.screenHeight, key.dpiScale, fontSize);
    }
    
    LOG_INFO("DisplayConfigManager: Loaded {} configs from file", loadedCount);
}

// 保存配置文件
void DisplayConfigManager::saveToFile() {
    if (m_configFilePath.empty()) {
        LOG_ERROR("DisplayConfigManager: Config file path is empty");
        return;
    }
    
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    
    for (const auto& [key, fontSize] : m_configs) {
        // 构建Key: "1920x1080_1.25"（DPI精确到两位小数）
        std::string keyStr = std::format("{}x{}_{:.2f}", 
                                          key.screenWidth, key.screenHeight, key.dpiScale);
        
        rapidjson::Value keyVal(keyStr.c_str(), allocator);
        doc.AddMember(keyVal, fontSize, allocator);
    }
    
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    std::ofstream file(m_configFilePath);
    if (!file.is_open()) {
        LOG_ERROR("DisplayConfigManager: Failed to open config file for writing: {}", 
                  m_configFilePath);
        return;
    }
    
    file << buffer.GetString();
    file.close();
    
    LOG_INFO("DisplayConfigManager: Saved {} configs to file", m_configs.size());
}

// 应用字体大小到ImGui
void DisplayConfigManager::applyConfig(int fontSize) {
    if (m_currentFontSize == fontSize) {
        return;
    }
    
    m_currentFontSize = fontSize;
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.FontSizeBase = static_cast<float>(fontSize);
    // UNSTABLE: 从imgui_demo.cpp中拿过来的，修改字体大小必须这样写，后续版本可能会有修改，需要注意
    style._NextFrameFontSizeBase = style.FontSizeBase;
    
    LOG_INFO("DisplayConfigManager: Applied font size: {}", fontSize);
}

// 处理配置变化（显示器或DPI改变时调用）
void DisplayConfigManager::onConfigChanged() {
    if (!m_initialized || !m_window) {
        LOG_ERROR("DisplayConfigManager: Cannot process config change, not initialized");
        return;
    }
    
    ConfigKey key = generateConfigKey(m_window);
    
    auto it = m_configs.find(key);
    int fontSize;
    
    if (it != m_configs.end()) {
        fontSize = it->second;
    } else {
        fontSize = generateDefaultFontSize(key.dpiScale);
        m_configs[key] = fontSize;
        m_dirty = true;
        LOG_INFO("DisplayConfigManager: Generated new config - screen: {}x{}, dpi: {:.2f} -> font: {}", 
                 key.screenWidth, key.screenHeight, key.dpiScale, fontSize);
    }
    
    applyConfig(fontSize);
}

// GLFW窗口位置变化回调，窗口移动可能切换显示器，需要重新检查配置
void DisplayConfigManager::onWindowPosCallback(GLFWwindow* window, int xpos, int ypos) {
    (void)xpos;
    (void)ypos;
    
    DisplayConfigManager& instance = getInstance();
    if (window == instance.m_window && instance.m_initialized) {
        instance.onConfigChanged();
    }
}

// GLFW窗口DPI缩放变化回调，系统DPI改变需要重新检查配置
void DisplayConfigManager::onWindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale) {
    (void)xscale;
    (void)yscale;
    
    DisplayConfigManager& instance = getInstance();
    if (window == instance.m_window && instance.m_initialized) {
        LOG_TRACE("DisplayConfigManager: Window content scale changed, checking config");
        instance.onConfigChanged();
    }
}

} // namespace tch