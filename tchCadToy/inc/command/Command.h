#pragma once

namespace tch {

// 命令基类
class Command {
protected:
    // 完成标志
    bool m_completed;

public:
    Command() : m_completed(false) {}
    virtual ~Command() = default;
    
    // 命令更新方法，在主循环中每一帧调用
    virtual void onUpdate() = 0;
    
    // 命令是否完成
    bool isCompleted() const;
    
    // 宣告命令结束，每个命令在完成所有流程后都应该调用此方法
    void finish();
    
    // 绘制预览（基类方法，派生类可重写）
    virtual void drawPreview() {} 
};

} // namespace tch
