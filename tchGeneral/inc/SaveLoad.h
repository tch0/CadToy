#pragma once
#include <string>

namespace tch {

// 保存/加载模块
class SaveLoad {
public:
    // 保存图形到文件
    static bool saveToFile(const std::string& filePath);
    
    // 从文件加载图形
    static bool loadFromFile(const std::string& filePath);
    
    // 导出为SVG格式
    static bool exportToSVG(const std::string& filePath);
    
    // 导出为PNG格式
    static bool exportToPNG(const std::string& filePath);

private:
    // 私有构造函数
    SaveLoad() = default;
};

} // namespace tch