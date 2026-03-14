#pragma once

#include "input/Task.h"
#include "common/CommonTypes.h"
#include <vector>

namespace tch {

// 栏选任务类
class FenceSelectionTask : public Task {
private:
    // 选择状态
    enum class SelectionState {
        kIdle,           // 空闲状态
        kSelecting,      // 正在选择（已开始绘制栏选）
        kCompleted       // 选择完成
    };
    
    SelectionState m_state;
    InteractionData* m_interactionData;
    std::vector<glm::vec2> m_fencePoints;
    bool m_enterPressed;
    bool m_escPressed;
    
public:
    FenceSelectionTask(InteractionData* interactionData);
    
    // 更新任务状态
    void onUpdate() override;
    
    // 检查任务是否完成
    bool isCompleted() const override;
    
    // 重置任务状态
    void reset() override;
    
    // 处理鼠标左键按下事件
    void onLeftMouseDown();
    
    // 处理回车/空格事件
    void onEnterSpace();
    
    // 处理Escape事件
    void onEscape();
};

} // namespace tch
