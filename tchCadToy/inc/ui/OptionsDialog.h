#pragma once

// C++ 标准库

// 第三方库

// 项目头文件

namespace tch {

// 选项对话框类 - 单例管理Options对话框的状态和渲染
class OptionsDialog {
public:
    static OptionsDialog& getInstance();
    
    // 显示并绘制对话框
    // 返回true表示对话框正在显示，返回false表示对话框已关闭
    bool show();
    
    // 初始化：从各系统获取当前配置到临时变量
    void initialize();
    
    // 写回：将临时变量写回各系统
    void writeBack();
    
private:
    OptionsDialog();
    ~OptionsDialog() = default;
    
    // 禁止拷贝
    OptionsDialog(const OptionsDialog&) = delete;
    OptionsDialog& operator=(const OptionsDialog&) = delete;
    
    // 对话框可见性（内部管理）
    bool m_visible;
    
    // 临时设置值（对话框内编辑用）
    bool m_tempShowGrid;
    bool m_tempShowAxes;
    int m_tempCrossCursorSize;
    int m_tempPickBoxSize;
    int m_tempFontSize;
};

} // namespace tch
