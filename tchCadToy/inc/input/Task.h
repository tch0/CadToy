#pragma once

namespace tch {

// 任务接口
class Task {
public:
    virtual ~Task() = default;
    
    // 更新任务状态
    virtual void onUpdate() = 0;
    
    // 检查任务是否完成
    virtual bool isCompleted() const = 0;
    
    // 重置任务状态
    virtual void reset() = 0;
};

} // namespace tch
