#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace tch {

// 颜色模块
class ColorManager {
public:
    // 获取单例实例
    static ColorManager& getInstance();
    
    // 禁止拷贝和赋值
    ColorManager(const ColorManager&) = delete;
    ColorManager& operator=(const ColorManager&) = delete;
    
    // 添加自定义颜色
    void addColor(const std::string& name, const glm::vec3& color);
    
    // 获取颜色
    glm::vec3 getColor(const std::string& name) const;
    
    // 检查颜色是否存在
    bool hasColor(const std::string& name) const;
    
    // 获取预定义颜色
    static glm::vec3 getRed();
    static glm::vec3 getGreen();
    static glm::vec3 getBlue();
    static glm::vec3 getWhite();
    static glm::vec3 getBlack();
    static glm::vec3 getGray();
    static glm::vec3 getYellow();
    static glm::vec3 getCyan();
    static glm::vec3 getMagenta();
    
    // 从RGB值创建颜色
    static glm::vec3 fromRGB(float r, float g, float b);
    
    // 从HSV值创建颜色
    static glm::vec3 fromHSV(float h, float s, float v);
    
    // 从十六进制字符串创建颜色
    static glm::vec3 fromHex(const std::string& hex);

private:
    // 私有构造函数
    ColorManager();
    
    // 颜色映射
    std::unordered_map<std::string, glm::vec3> m_colors;
};

} // namespace tch