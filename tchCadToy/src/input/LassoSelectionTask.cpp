#include "input/LassoSelectionTask.h"
#include "input/InputHandler.h"

namespace tch {

LassoSelectionTask::LassoSelectionTask(InteractionData* interactionData)
    : m_state(SelectionState::kIdle), m_interactionData(interactionData) {}

void LassoSelectionTask::onUpdate() {
    switch (m_state) {
        case SelectionState::kIdle:
            // 检查鼠标左键是否按下
            if (InputHandler::isLeftMouseButtonPressed()) {
                // 开始套索选择
                m_state = SelectionState::kSelecting;
                m_interactionData->isSelectionActive = true;
                // 设置光标模式为十字
                m_interactionData->cursorMode = CursorMode::kCrosshair;
                // 根据鼠标位置确定选择模式
                glm::vec2 cursorPos = InputHandler::getCursorPosition();
                // 这里假设初始点为屏幕中心，实际应用中可能需要根据具体情况确定
                glm::vec2 center = glm::vec2(400.0f, 300.0f);
                if (cursorPos.x < center.x) {
                    // 向左移动，使用交叉选择
                    m_interactionData->selectionMode = SelectionMode::kCrossingLasso;
                    m_interactionData->cursorMarker = CursorMarker::kCrossingSelect;
                } else {
                    // 向右移动，使用窗口选择
                    m_interactionData->selectionMode = SelectionMode::kWindowLasso;
                    m_interactionData->cursorMarker = CursorMarker::kWindowSelect;
                }
                m_lassoPoints.clear();
                m_interactionData->selectionPoints.clear();
                m_lassoPoints.push_back(cursorPos);
                m_interactionData->selectionPoints.push_back(cursorPos);
            }
            break;
            
        case SelectionState::kSelecting:
            // 检查鼠标左键是否仍然按下
            if (InputHandler::isLeftMouseButtonPressed()) {
                // 添加套索点
                glm::vec2 cursorPos = InputHandler::getCursorPosition();
                m_lassoPoints.push_back(cursorPos);
                m_interactionData->selectionPoints.push_back(cursorPos);
            } else {
                // 鼠标释放，结束套索选择
                m_state = SelectionState::kCompleted;
                m_interactionData->isSelectionActive = false;
                // 恢复默认光标模式和标记
                m_interactionData->cursorMode = CursorMode::kDefault;
                m_interactionData->cursorMarker = CursorMarker::kNone;
                // 这里可以添加选择实体的逻辑
            }
            break;
            
        case SelectionState::kCompleted:
            // 选择完成，重置状态
            reset();
            break;
            
        default:
            break;
    }
}

bool LassoSelectionTask::isCompleted() const {
    return m_state == SelectionState::kCompleted;
}

void LassoSelectionTask::reset() {
    m_state = SelectionState::kIdle;
    m_interactionData->isSelectionActive = false;
    m_interactionData->selectionMode = SelectionMode::kNone;
    m_interactionData->cursorMode = CursorMode::kDefault;
    m_interactionData->cursorMarker = CursorMarker::kNone;
    m_lassoPoints.clear();
    m_interactionData->selectionPoints.clear();
}

} // namespace tch
