#include "input/SelectionTask.h"
#include "input/InputHandler.h"
#include "input/InputContext.h"
#include "render/Renderer.h"
#include <glm/glm.hpp>

namespace tch {

SelectionTask::SelectionTask()
    : m_state(SelectionState::kIdle),
      m_selectionMode(SelectionMode::kWindow),
      m_completed(false),
      m_lassoModeCycle(LassoModeCycle::kCrossing),
      m_isCommandActive(false) {
}

void SelectionTask::onUpdate() {
    // 如果处于空闲或完成状态，直接返回
    if (m_state == SelectionState::kIdle || m_state == SelectionState::kCompleted) {
        return;
    }
    
    // 获取当前光标位置（屏幕坐标）
    m_previewPointScreen = InputHandler::getCursorPosition();
    // 转换为世界坐标
    m_previewPointWorld = Renderer::getTransformManager().screenToWorld(m_previewPointScreen);
    
    // 获取交互数据
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    
    // 根据选择模式设置光标模式
    if (m_state == SelectionState::kSingleSelecting) {
        // 单选模式下使用仅拾取框光标
        interactionData.cursorMode = CursorMode::kPickbox;
        // 单选模式下不显示任何标记
        interactionData.cursorMarker = CursorMarker::kNone;
    } else {
        // 其他选择模式使用十字光标
        interactionData.cursorMode = CursorMode::kCrosshair;
        // 根据选择模式设置光标标记
        switch (m_selectionMode) {
            case SelectionMode::kWindow:
            case SelectionMode::kWindowLasso:
            case SelectionMode::kWindowPolygon:
                interactionData.cursorMarker = CursorMarker::kWindowSelect;
                break;
            case SelectionMode::kCrossing:
            case SelectionMode::kCrossingLasso:
            case SelectionMode::kCrossingPolygon:
                interactionData.cursorMarker = CursorMarker::kCrossingSelect;
                break;
            case SelectionMode::kFence:
                interactionData.cursorMarker = CursorMarker::kNone;
                break;
            default:
                interactionData.cursorMarker = CursorMarker::kNone;
                break;
        }
    }
    
    // 检查Shift状态，设置光标标记
    // TODO: 减选标记是位于实体上方才触发，Window和Crossing标记会覆盖加选减选标记
    // 暂时不处理减选加选的标记，预留注释
    // if (InputHandler::isShiftPressed()) {
    //     interactionData.cursorMarker = CursorMarker::kRemoveSelect;
    // }
    
    // 处理框选到套索选择的切换
    if (m_state == SelectionState::kBoxSelecting && InputHandler::isLeftMouseButtonPressed()) {
        // 计算鼠标移动距离（使用屏幕坐标）
        float distance = glm::distance(m_previewPointScreen, m_initialPointScreen);
        // 如果移动距离超过阈值，切换到套索选择
        const float lassoThreshold = 100.0f; // 切换阈值
        if (distance > lassoThreshold) {
            // 根据初始点和当前点的相对位置决定是窗口还是交叉选择（使用世界坐标）
            if (m_previewPointWorld.x < m_initialPointWorld.x) {
                m_selectionMode = SelectionMode::kCrossingLasso;
            } else {
                m_selectionMode = SelectionMode::kWindowLasso;
            }
            m_state = SelectionState::kLassoSelecting;
            // 重置选择点，只保留初始点
            m_selectionPointsWorld.clear();
            m_selectionPointsWorld.push_back(m_initialPointWorld);
            // 初始化套索最后一点屏幕坐标
            m_lastLassoPointScreen = m_initialPointScreen;
            // 更新交互数据
            interactionData.selectionMode = m_selectionMode;
            interactionData.selectionPointsWorld = m_selectionPointsWorld;
        }
    }
    
    switch (m_state) {
        case SelectionState::kBoxSelecting:
            handleBoxSelection();
            break;
        case SelectionState::kLassoSelecting:
            // 只有在鼠标持续按下时才处理套索选择
            if (InputHandler::isLeftMouseButtonPressed()) {
                handleLassoSelection();
            } else {
                // 鼠标释放，完成套索选择
                finishSelection();
            }
            break;
        case SelectionState::kPolygonSelecting:
            handlePolygonSelection();
            break;
        case SelectionState::kFenceSelecting:
            handleFenceSelection();
            break;
        default:
            break;
    }
}

bool SelectionTask::isCompleted() const {
    return m_completed;
}

void SelectionTask::reset() {
    m_state = SelectionState::kIdle;
    m_selectionMode = SelectionMode::kWindow;
    m_selectionPointsWorld.clear();
    m_initialPointWorld = glm::dvec3(0.0, 0.0, 0.0);
    m_previewPointWorld = glm::dvec3(0.0, 0.0, 0.0);
    m_initialPointScreen = glm::vec2(0.0f, 0.0f);
    m_previewPointScreen = glm::vec2(0.0f, 0.0f);
    m_lastLassoPointScreen = glm::vec2(0.0f, 0.0f);
    m_completed = false;
    m_lassoModeCycle = LassoModeCycle::kCrossing;
    m_currentPrompt = "";
    m_isCommandActive = false;
    
    // 重置光标状态
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    interactionData.cursorMode = CursorMode::kDefault;
    interactionData.cursorMarker = CursorMarker::kNone;
    interactionData.isSelectionActive = false;
}

void SelectionTask::start(bool isCommandActive, bool isSingleSelect) {
    m_state = isSingleSelect ? SelectionState::kSingleSelecting : SelectionState::kInitialPoint;
    m_completed = false;
    m_isCommandActive = isCommandActive;
    
    // 单选模式下设置选择模式为kSingle
    if (isSingleSelect) {
        m_selectionMode = SelectionMode::kSingle;
        // 更新交互数据
        InteractionData& interactionData = InputContext::getInstance().getInteractionData();
        interactionData.selectionMode = m_selectionMode;
    }
    
    // 更新提示
    updatePrompt();
}

void SelectionTask::handleLeftMouseClick() {
    // 获取交互数据
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    
    switch (m_state) {
        case SelectionState::kSingleSelecting: {
            // 单选模式下点击
            m_initialPointScreen = InputHandler::getCursorPosition();
            m_initialPointWorld = Renderer::getTransformManager().screenToWorld(m_initialPointScreen);
            
            // TODO: 检查点击位置是否有实体
            // 这里应该有实体检测逻辑，暂时简化处理
            // 如果有实体，直接完成选择
            // 如果没有实体，进入框选流程
            
            // 模拟有实体的情况，直接完成选择
            // 实际实现中需要替换为真实的实体检测
            bool hasEntity = false; // 假设没有实体
            
            if (hasEntity) {
                // 有实体，添加到选择集并完成选择
                m_selectionPointsWorld.push_back(m_initialPointWorld);
                finishSelection();
            } else {
                // 没有实体，进入框选流程
                m_selectionPointsWorld.push_back(m_initialPointWorld);
                m_state = SelectionState::kBoxSelecting;
                
                // 更新交互数据
                interactionData.isSelectionActive = true;
                interactionData.selectionMode = m_selectionMode;
                interactionData.selectionPointsWorld = m_selectionPointsWorld;
                interactionData.selectionBoxStartWorld = m_initialPointWorld;
                interactionData.selectionBoxCurrentWorld = m_initialPointWorld;
                
                // 更新提示
                updatePrompt();
            }
            break;
        }
        case SelectionState::kInitialPoint:
            // 记录初始点
            m_initialPointScreen = InputHandler::getCursorPosition();
            m_initialPointWorld = Renderer::getTransformManager().screenToWorld(m_initialPointScreen);
            m_selectionPointsWorld.push_back(m_initialPointWorld);
            m_state = SelectionState::kBoxSelecting;
            
            // 更新交互数据
            interactionData.isSelectionActive = true;
            interactionData.selectionMode = m_selectionMode;
            interactionData.selectionPointsWorld = m_selectionPointsWorld;
            interactionData.selectionBoxStartWorld = m_initialPointWorld;
            interactionData.selectionBoxCurrentWorld = m_initialPointWorld;
            
            // 检查Shift状态，设置光标标记
            // TODO: 减选标记是位于实体上方才触发，Window和Crossing标记会覆盖加选减选标记
            // 暂时不处理减选加选的标记，预留注释
            // if (InputHandler::isShiftPressed()) {
            //     interactionData.cursorMarker = CursorMarker::kRemoveSelect;
            // }
            
            // 更新提示
            updatePrompt();
            break;
        case SelectionState::kBoxSelecting:
            // 完成框选
            m_selectionPointsWorld.push_back(m_previewPointWorld);
            finishSelection();
            break;
        case SelectionState::kPolygonSelecting:
            // 添加多边形顶点
            m_selectionPointsWorld.push_back(m_previewPointWorld);
            interactionData.selectionPointsWorld = m_selectionPointsWorld;
            break;
        case SelectionState::kFenceSelecting:
            // 添加栏选线段端点
            m_selectionPointsWorld.push_back(m_previewPointWorld);
            interactionData.selectionPointsWorld = m_selectionPointsWorld;
            break;
        default:
            break;
    }
}

void SelectionTask::handleKeyword(const std::string& keyword) {
    if (keyword == "F" || keyword == "f") {
        // 切换到栏选模式
        switchToMode(SelectionMode::kFence);
    } else if (keyword == "WP" || keyword == "wp") {
        // 切换到窗口多边形模式
        switchToMode(SelectionMode::kWindowPolygon);
    } else if (keyword == "CP" || keyword == "cp") {
        // 切换到交叉多边形模式
        switchToMode(SelectionMode::kCrossingPolygon);
    }
}

void SelectionTask::updatePrompt() {
    // 根据当前状态生成选择任务的提示
    switch (m_state) {
        case SelectionState::kSingleSelecting:
            m_currentPrompt = "选择对象:";
            break;
        case SelectionState::kInitialPoint:
        case SelectionState::kBoxSelecting:
            m_currentPrompt = "指定对角点或 [栏选(F)/圈围(WP)/圈交(CP)]:";
            break;
        case SelectionState::kLassoSelecting:
            switch (m_lassoModeCycle) {
                case LassoModeCycle::kCrossing:
                    m_currentPrompt = "窗交(C) 套索  按空格键以循环选项";
                    break;
                case LassoModeCycle::kWindow:
                    m_currentPrompt = "窗口(W) 套索  按空格键以循环选项";
                    break;
                case LassoModeCycle::kFence:
                    m_currentPrompt = "栏选(F) 套索  按空格键以循环选项";
                    break;
            }
            break;
        case SelectionState::kPolygonSelecting:
            m_currentPrompt = "指定直线的端点或 [放弃(U)]:";
            break;
        case SelectionState::kFenceSelecting:
            m_currentPrompt = "指定下一个栏选点或 [放弃(U)]:";
            break;
        default:
            m_currentPrompt = "";
            break;
    }
    
    // 更新InputContext的提示
    // 命令中如果命令提示为空，则使用选择任务的提示
    // 命令中如果命令提示不为空，则覆盖选择任务的提示
    InputContext& inputContext = InputContext::getInstance();
    const std::string& commandPrompt = inputContext.getPrompt();
    
    if (m_isCommandActive && !commandPrompt.empty()) {
        // 命令中有提示，保持命令提示不变
        // 但选择分支（F/WP/CP）仍然可以进入
    } else {
        // 非命令中或命令中无提示，使用选择任务的提示
        inputContext.setPrompt(m_currentPrompt);
    }
}

void SelectionTask::handleEnterSpace() {
    if (m_state == SelectionState::kSingleSelecting) {
        // 单选模式下按回车或空格键结束选择
        finishSelection();
    } else if (m_state == SelectionState::kLassoSelecting) {
        // 套索模式Enter/Space将在交叉套索、窗口套索、栏选套索之间切换
        switch (m_lassoModeCycle) {
            case LassoModeCycle::kCrossing:
                m_lassoModeCycle = LassoModeCycle::kWindow;
                m_selectionMode = SelectionMode::kWindowLasso;
                break;
            case LassoModeCycle::kWindow:
                // 这里选择模式切换为栏选(Fence)，但是选择状态需要保持为套索，因为套索当中的栏选是按着鼠标左键交互的
                m_lassoModeCycle = LassoModeCycle::kFence;
                m_selectionMode = SelectionMode::kFence;
                m_state = SelectionState::kLassoSelecting;
                break;
        case LassoModeCycle::kFence:
            m_lassoModeCycle = LassoModeCycle::kCrossing;
            m_selectionMode = SelectionMode::kCrossingLasso;
            m_state = SelectionState::kLassoSelecting;
            break;
        }
        
        // 更新交互数据
        InteractionData& interactionData = InputContext::getInstance().getInteractionData();
        interactionData.selectionMode = m_selectionMode;
        
        // 更新提示
        updatePrompt();
    } else if (m_state == SelectionState::kBoxSelecting) {
        // 框选模式下按回车或空格键显示错误提示
        InputContext::getInstance().setPrompt("窗口说明无效。");
    } else if (m_state == SelectionState::kPolygonSelecting || 
               m_state == SelectionState::kFenceSelecting) {
        // 多边形和栏选模式下按回车或空格键完成选择
        finishSelection();
    }
}

void SelectionTask::handleEscape() {
    // 取消选择
    reset();
}

SelectionMode SelectionTask::getSelectionMode() const {
    return m_selectionMode;
}

const std::vector<glm::dvec3>& SelectionTask::getSelectionPoints() const {
    return m_selectionPointsWorld;
}

bool SelectionTask::isSelecting() const {
    return m_state != SelectionState::kIdle && m_state != SelectionState::kCompleted;
}

void SelectionTask::handleBoxSelection() {
    // 框选模式下，只需要初始点和当前光标位置
    if (m_selectionPointsWorld.size() > 1) {
        m_selectionPointsWorld.pop_back();
    }
    m_selectionPointsWorld.push_back(m_previewPointWorld);
    
    // 根据初始点和当前点的相对位置决定是窗口还是交叉选择（使用世界坐标）
    if (m_previewPointWorld.x < m_initialPointWorld.x) {
        m_selectionMode = SelectionMode::kCrossing;
    } else {
        m_selectionMode = SelectionMode::kWindow;
    }
    
    // 更新交互数据中的选择点和选择模式
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    interactionData.selectionMode = m_selectionMode;
    interactionData.selectionPointsWorld = m_selectionPointsWorld;
    interactionData.selectionBoxStartWorld = m_initialPointWorld;
    interactionData.selectionBoxCurrentWorld = m_previewPointWorld;
}

void SelectionTask::handleLassoSelection() {
    // 套索模式下，需要实时添加点，只在两点屏幕距离足够远时添加
    const float minDistance = 10.0f; // 最小距离阈值
    if (m_selectionPointsWorld.empty()) {
        m_selectionPointsWorld.push_back(m_initialPointWorld);
        m_lastLassoPointScreen = m_initialPointScreen;
    } else {
        // 使用屏幕坐标计算距离
        float distance = glm::distance(m_previewPointScreen, m_lastLassoPointScreen);
        if (distance >= minDistance) {
            m_selectionPointsWorld.push_back(m_previewPointWorld);
            m_lastLassoPointScreen = m_previewPointScreen;
        }
    }
    
    // 更新交互数据中的选择点
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    interactionData.selectionPointsWorld = m_selectionPointsWorld;
}

void SelectionTask::handlePolygonSelection() {
    // 多边形模式下，等待用户点击添加顶点
    // 最后按Enter键完成
}

void SelectionTask::handleFenceSelection() {
    // 栏选模式下，等待用户点击添加线段端点
    // 最后按Enter键完成
}

void SelectionTask::switchToMode(SelectionMode mode) {
    m_selectionMode = mode;
    
    // 更新交互数据中的选择模式
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    interactionData.selectionMode = mode;
    
    // 根据选择模式切换状态
    switch (mode) {
        case SelectionMode::kWindow:
        case SelectionMode::kCrossing:
            m_state = SelectionState::kBoxSelecting;
            break;
        case SelectionMode::kWindowLasso:
        case SelectionMode::kCrossingLasso:
            m_state = SelectionState::kLassoSelecting;
            break;
        case SelectionMode::kWindowPolygon:
        case SelectionMode::kCrossingPolygon:
            m_state = SelectionState::kPolygonSelecting;
            break;
        case SelectionMode::kFence:
            m_state = SelectionState::kFenceSelecting;
            break;
        default:
            break;
    }
}

void SelectionTask::finishSelection() {
    m_state = SelectionState::kCompleted;
    m_completed = true;
    
    // 重置光标状态
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    interactionData.cursorMode = CursorMode::kDefault;
    interactionData.cursorMarker = CursorMarker::kNone;
    interactionData.isSelectionActive = false;
    
    // 通知InputContext选择完成
    // 实际的选择结果处理会在InputContext中进行
    // 根据Shift状态决定是加选还是减选
    // 选择结果将由InputContext在onUpdate中处理
}

} // namespace tch