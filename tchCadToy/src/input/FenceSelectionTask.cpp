#include "input/FenceSelectionTask.h"
#include "input/InputHandler.h"

namespace tch {

FenceSelectionTask::FenceSelectionTask(InteractionData* interactionData)
    : m_state(SelectionState::kIdle), m_interactionData(interactionData), 
      m_enterPressed(false), m_escPressed(false) {}

void FenceSelectionTask::onUpdate() {
    switch (m_state) {
        case SelectionState::kSelecting:
            // 检查是否按下了Enter或Escape
            if (m_enterPressed) {
                // 结束栏选选择
                m_state = SelectionState::kCompleted;
                m_interactionData->isSelectionActive = false;
                // 恢复默认光标模式和标记
                m_interactionData->cursorMode = CursorMode::kDefault;
                m_interactionData->cursorMarker = CursorMarker::kNone;
                // 这里可以添加选择实体的逻辑
            } else if (m_escPressed) {
                // 取消栏选选择
                reset();
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

bool FenceSelectionTask::isCompleted() const {
    return m_state == SelectionState::kCompleted;
}

void FenceSelectionTask::reset() {
    m_state = SelectionState::kIdle;
    m_interactionData->isSelectionActive = false;
    m_interactionData->selectionMode = SelectionMode::kNone;
    m_interactionData->cursorMode = CursorMode::kDefault;
    m_interactionData->cursorMarker = CursorMarker::kNone;
    m_fencePoints.clear();
    m_interactionData->selectionPoints.clear();
    m_enterPressed = false;
    m_escPressed = false;
}

void FenceSelectionTask::onLeftMouseDown() {
    if (m_state == SelectionState::kIdle) {
        // 开始栏选选择
        m_state = SelectionState::kSelecting;
        m_interactionData->isSelectionActive = true;
        m_interactionData->selectionMode = SelectionMode::kFence;
        // 设置光标模式为十字
        m_interactionData->cursorMode = CursorMode::kCrosshair;
        m_fencePoints.clear();
        m_interactionData->selectionPoints.clear();
        glm::vec2 cursorPos = InputHandler::getCursorPosition();
        m_fencePoints.push_back(cursorPos);
        m_interactionData->selectionPoints.push_back(cursorPos);
    } else if (m_state == SelectionState::kSelecting) {
        // 添加栏选点
        glm::vec2 cursorPos = InputHandler::getCursorPosition();
        m_fencePoints.push_back(cursorPos);
        m_interactionData->selectionPoints.push_back(cursorPos);
    }
}

void FenceSelectionTask::onEnterSpace() {
    if (m_state == SelectionState::kSelecting) {
        m_enterPressed = true;
    }
}

void FenceSelectionTask::onEscape() {
    if (m_state == SelectionState::kSelecting) {
        m_escPressed = true;
    }
}

} // namespace tch
