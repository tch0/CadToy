#pragma once

#include "input/Task.h"
#include "common/CommonTypes.h"
#include <vector>

namespace tch {

// 套索选择任务类
class LassoSelectionTask : public Task {
private:
    // 选择状态
    enum class SelectionState {
        kIdle,           // 空闲状态
        kSelecting,      // 正在选择（已开始绘制套索）
        kCompleted       // 选择完成
    };
    
    SelectionState m_state;
    InteractionData* m_interactionData;
    std::vector<glm::vec2> m_lassoPoints;
    
public:
    LassoSelectionTask(InteractionData* interactionData);
    
    // 更新任务状态
    void onUpdate() override;
    
    // 检查任务是否完成
    bool isCompleted() const override;
    
    // 重置任务状态
    void reset() override;
};

} // namespace tch
