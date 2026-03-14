#include "input/PolygonSelectionTask.h"
#include "input/InputHandler.h"

namespace tch {

PolygonSelectionTask::PolygonSelectionTask(InteractionData* interactionData)
    : m_state(SelectionState::kIdle), m_interactionData(interactionData) {}

void PolygonSelectionTask::onUpdate() {
    switch (m_state) {
        case SelectionState::kSelecting:
            // 更新临时顶点，用于预览
            if (!m_polygonPoints.empty()) {
                // 注意：这里需要在InteractionData中添加临时顶点字段
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

bool PolygonSelectionTask::isCompleted() const {
    return m_state == SelectionState::kCompleted;
}

void PolygonSelectionTask::reset() {
    m_state = SelectionState::kIdle;
    m_interactionData->isSelectionActive = false;
    m_interactionData->selectionMode = SelectionMode::kNone;
    m_interactionData->cursorMode = CursorMode::kDefault;
    m_interactionData->cursorMarker = CursorMarker::kNone;
    m_polygonPoints.clear();
    m_interactionData->selectionPoints.clear();
}

void PolygonSelectionTask::onLeftMouseDown() {
    if (m_state == SelectionState::kIdle) {
        // 开始多边形选择
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
            m_interactionData->selectionMode = SelectionMode::kCrossingPolygon;
            m_interactionData->cursorMarker = CursorMarker::kCrossingSelect;
        } else {
            // 向右移动，使用窗口选择
            m_interactionData->selectionMode = SelectionMode::kWindowPolygon;
            m_interactionData->cursorMarker = CursorMarker::kWindowSelect;
        }
        m_polygonPoints.clear();
        m_interactionData->selectionPoints.clear();
        m_polygonPoints.push_back(cursorPos);
        m_interactionData->selectionPoints.push_back(cursorPos);
    } else if (m_state == SelectionState::kSelecting) {
        // 添加多边形顶点
        glm::vec2 cursorPos = InputHandler::getCursorPosition();
        m_polygonPoints.push_back(cursorPos);
        m_interactionData->selectionPoints.push_back(cursorPos);
    }
}

void PolygonSelectionTask::onEnterSpace() {
    if (m_state == SelectionState::kSelecting && m_polygonPoints.size() >= 3) {
        // 结束多边形选择
        m_state = SelectionState::kCompleted;
        m_interactionData->isSelectionActive = false;
        // 恢复默认光标模式和标记
        m_interactionData->cursorMode = CursorMode::kDefault;
        m_interactionData->cursorMarker = CursorMarker::kNone;
        // 这里可以添加选择实体的逻辑
    }
}

} // namespace tch
