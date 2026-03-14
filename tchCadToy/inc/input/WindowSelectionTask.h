#pragma once

#include "input/Task.h"
#include "common/CommonTypes.h"

namespace tch {

// 框选任务类
class WindowSelectionTask : public Task {
private:
    // 选择状态
    enum class SelectionState {
        kIdle,           // 空闲状态
        kSelecting,      // 正在选择（已确定第一点）
        kCompleted       // 选择完成
    };
    
    SelectionState m_state;
    InteractionData* m_interactionData;
    bool m_leftButtonPressed;
    
public:
    WindowSelectionTask(InteractionData* interactionData);
    
    // 更新任务状态
    void onUpdate() override;
    
    // 检查任务是否完成
    bool isCompleted() const override;
    
    // 重置任务状态
    void reset() override;
    
    // 处理鼠标左键按下事件
    void onLeftMouseDown();
    
    // 处理鼠标左键释放事件
    void onLeftMouseUp();
};

} // namespace tch
