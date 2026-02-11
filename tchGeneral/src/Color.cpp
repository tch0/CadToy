#include "Color.h"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace tch {

// 私有构造函数
ColorManager::ColorManager() {
    // 初始化预定义颜色
    m_colors["red"] = getRed();
    m_colors["green"] = getGreen();
    m_colors["blue"] = getBlue();
    m_colors["white"] = getWhite();
    m_colors["black"] = getBlack();
    m_colors["gray"] = getGray();
    m_colors["yellow"] = getYellow();
    m_colors["cyan"] = getCyan();
    m_colors["magenta"] = getMagenta();
}

// 获取单例实例
ColorManager& ColorManager::getInstance() {
    static ColorManager instance;
    return instance;
}

// 添加自定义颜色
void ColorManager::addColor(const std::string& name, const glm::vec3& color) {
    m_colors[name] = color;
}

// 获取颜色
glm::vec3 ColorManager::getColor(const std::string& name) const {
    auto it = m_colors.find(name);
    if (it != m_colors.end()) {
        return it->second;
    }
    return getWhite(); // 默认白色
}

// 检查颜色是否存在
bool ColorManager::hasColor(const std::string& name) const {
    return m_colors.find(name) != m_colors.end();
}

// 获取预定义颜色
glm::vec3 ColorManager::getRed() {
    return glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 ColorManager::getGreen() {
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 ColorManager::getBlue() {
    return glm::vec3(0.0f, 0.0f, 1.0f);
}

glm::vec3 ColorManager::getWhite() {
    return glm::vec3(1.0f, 1.0f, 1.0f);
}

glm::vec3 ColorManager::getBlack() {
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 ColorManager::getGray() {
    return glm::vec3(0.5f, 0.5f, 0.5f);
}

glm::vec3 ColorManager::getYellow() {
    return glm::vec3(1.0f, 1.0f, 0.0f);
}

glm::vec3 ColorManager::getCyan() {
    return glm::vec3(0.0f, 1.0f, 1.0f);
}

glm::vec3 ColorManager::getMagenta() {
    return glm::vec3(1.0f, 0.0f, 1.0f);
}

// 从RGB值创建颜色
glm::vec3 ColorManager::fromRGB(float r, float g, float b) {
    return glm::vec3(
        glm::clamp(r, 0.0f, 1.0f),
        glm::clamp(g, 0.0f, 1.0f),
        glm::clamp(b, 0.0f, 1.0f)
    );
}

// 从HSV值创建颜色
glm::vec3 ColorManager::fromHSV(float h, float s, float v) {
    h = glm::clamp(h, 0.0f, 360.0f);
    s = glm::clamp(s, 0.0f, 1.0f);
    v = glm::clamp(v, 0.0f, 1.0f);
    
    float c = v * s;
    float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r, g, b;
    
    if (h >= 0.0f && h < 60.0f) {
        r = c; g = x; b = 0.0f;
    } else if (h >= 60.0f && h < 120.0f) {
        r = x; g = c; b = 0.0f;
    } else if (h >= 120.0f && h < 180.0f) {
        r = 0.0f; g = c; b = x;
    } else if (h >= 180.0f && h < 240.0f) {
        r = 0.0f; g = x; b = c;
    } else if (h >= 240.0f && h < 300.0f) {
        r = x; g = 0.0f; b = c;
    } else {
        r = c; g = 0.0f; b = x;
    }
    
    return glm::vec3(r + m, g + m, b + m);
}

// 从十六进制字符串创建颜色
glm::vec3 ColorManager::fromHex(const std::string& hex) {
    std::string hexClean = hex;
    if (hexClean.front() == '#') {
        hexClean = hexClean.substr(1);
    }
    
    if (hexClean.length() != 6) {
        return getWhite();
    }
    
    try {
        int r = std::stoi(hexClean.substr(0, 2), nullptr, 16);
        int g = std::stoi(hexClean.substr(2, 2), nullptr, 16);
        int b = std::stoi(hexClean.substr(4, 2), nullptr, 16);
        
        return fromRGB(r / 255.0f, g / 255.0f, b / 255.0f);
    } catch (...) {
        return getWhite();
    }
}

} // namespace tch