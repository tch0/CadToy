#pragma once

// C++ 标准库

// 第三方库

// 项目头文件

namespace tch {

// 属性栏内容管理类 - 单例
class PropertyBarContent {
public:
    static PropertyBarContent& getInstance();
    
    // 绘制属性栏内容
    void draw();

private:
    PropertyBarContent();
    ~PropertyBarContent() = default;
    
    // 禁止拷贝
    PropertyBarContent(const PropertyBarContent&) = delete;
    PropertyBarContent& operator=(const PropertyBarContent&) = delete;
};

} // namespace tch
