#pragma once

namespace tch {

// 命令基类
class Command {
public:
    virtual ~Command() = default;
    
    // 命令更新方法，在主循环中每一帧调用
    virtual void onUpdate() = 0;
    
    // 命令是否完成
    virtual bool isCompleted() const = 0;
    
    // 取消命令
    virtual void cancel() = 0;
    
    // 绘制预览（基类方法，派生类可重写）
    virtual void drawPreview() {}
};

} // namespace tch
