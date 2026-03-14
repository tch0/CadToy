#include "input/WindowSelectionTask.h"
#include "input/InputHandler.h"

namespace tch {

WindowSelectionTask::WindowSelectionTask(InteractionData* interactionData)
    : m_state(SelectionState::kIdle), m_interactionData(interactionData), m_leftButtonPressed(false) {}

void WindowSelectionTask::onUpdate() {
    switch (m_state) {
        case SelectionState::kSelecting:
            // 更新当前光标位置
            m_interactionData->selectionBoxCurrent = InputHandler::getCursorPosition();
            
            // 根据光标位置与起点的关系确定选择模式
            if (m_interactionData->selectionBoxCurrent.x < m_interactionData->selectionBoxStart.x) {
                // 向左移动，使用交叉选择
                m_interactionData->selectionMode = SelectionMode::kCrossing;
                m_interactionData->cursorMarker = CursorMarker::kCrossingSelect;
            } else {
                // 向右移动，使用窗口选择
                m_interactionData->selectionMode = SelectionMode::kWindow;
                m_interactionData->cursorMarker = CursorMarker::kWindowSelect;
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

bool WindowSelectionTask::isCompleted() const {
    return m_state == SelectionState::kCompleted;
}

void WindowSelectionTask::reset() {
    m_state = SelectionState::kIdle;
    m_leftButtonPressed = false;
    m_interactionData->isSelectionActive = false;
    m_interactionData->selectionMode = SelectionMode::kNone;
    m_interactionData->cursorMode = CursorMode::kDefault;
    m_interactionData->cursorMarker = CursorMarker::kNone;
}

void WindowSelectionTask::onLeftMouseDown() {
    switch (m_state) {
        case SelectionState::kIdle:
            // 第一次点击，确定第一点
            m_state = SelectionState::kSelecting;
            m_interactionData->isSelectionActive = true;
            m_interactionData->selectionBoxStart = InputHandler::getCursorPosition();
            m_interactionData->selectionBoxCurrent = InputHandler::getCursorPosition();
            // 默认设置为窗口选择
            m_interactionData->selectionMode = SelectionMode::kWindow;
            // 设置光标模式为十字
            m_interactionData->cursorMode = CursorMode::kCrosshair;
            // 设置光标标记为窗口选择
            m_interactionData->cursorMarker = CursorMarker::kWindowSelect;
            m_leftButtonPressed = true;
            break;
            
        case SelectionState::kSelecting:
            // 第二次点击，确定第二点，完成选择
            m_state = SelectionState::kCompleted;
            m_interactionData->isSelectionActive = false;
            // 恢复默认光标模式和标记
            m_interactionData->cursorMode = CursorMode::kDefault;
            m_interactionData->cursorMarker = CursorMarker::kNone;
            m_leftButtonPressed = false;
            // 这里可以添加选择实体的逻辑
            break;
            
        default:
            break;
    }
}

void WindowSelectionTask::onLeftMouseUp() {
    // 框选任务中，鼠标释放不需要特殊处理
    m_leftButtonPressed = false;
}

} // namespace tch
