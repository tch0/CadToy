// 对应头文件
#include "SelectionTask.h"

// C++ 标准库

// 第三方库
#include <glm/glm.hpp>

// 项目头文件
#include "InputHandler.h"
#include "InputContext.h"
#include "Renderer.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "SelectionManager.h"

namespace tch {

// 静态工具方法：判断鼠标是否移动超过阈值，X+Y之和超过0.9像素，规避浮点误差，又确保移动了
bool SelectionTask::mouseMoved(const glm::vec2& current, const glm::vec2& last) {
    float dx = std::abs(current.x - last.x);
    float dy = std::abs(current.y - last.y);
    return (dx + dy) > 0.9f;
}

SelectionTask::SelectionTask()
    : m_state(SelectionState::kIdle),
      m_selectionMode(SelectionMode::kWindow),
      m_completed(false),
      m_lassoModeCycle(LassoModeCycle::kCrossing),
      m_isCommandActive(false) {
}

void SelectionTask::start(bool isCommandActive, bool isSingleSelect) {
    m_isCommandActive = isCommandActive;
    
    if (isSingleSelect) {
        // 单选
        m_state = SelectionState::kSingleSelectionEntry;
        m_selectionMode = SelectionMode::kSingle;
    }
    else {
        // 框选
        m_state = SelectionState::kBoxSelectionEntry;
        m_selectionMode = SelectionMode::kWindow;
        
        // 记录初始点
        glm::vec2 screenPos = InputHandler::getCursorPosition();
        m_initialPointScreen = screenPos;
        m_initialPointWorld = Renderer::getTransformManager().screenToWorld(screenPos);
        m_selectionPointsWorld.push_back(m_initialPointWorld);
        
        // 更新交互数据
        InteractionData& interactionData = InputContext::getInstance().getInteractionData();
        interactionData.isSelectionActive = true;
        interactionData.selectionMode = m_selectionMode;
        interactionData.selectionInitialPointWorld = m_initialPointWorld;
        interactionData.selectionPreviewPointWorld = m_initialPointWorld;
    }
    
    m_completed = false;
    m_inputStatus = InputStatus::kNone;
    m_lastPreSelectScreenPos = glm::vec2(0.0f, 0.0f);
}

void SelectionTask::onUpdate() {
    // 如果处于空闲或完成状态，直接返回
    if (m_state == SelectionState::kIdle || m_state == SelectionState::kCompleted) {
        return;
    }
    
    auto& loc = LocalizationManager::getInstance();
    
    // 获取交互数据
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    
    // 获取当前光标位置，更新预览点坐标
    m_previewPointScreen = InputHandler::getCursorPosition();
    m_previewPointWorld = InputContext::getInstance().getPreviewPoint();
    interactionData.selectionPreviewPointWorld = m_previewPointWorld;
    
    // 平移模式下选择相关的光标不会覆盖平移光标，平移结束后则会立即覆盖
    if (interactionData.cursorMode != CursorMode::kPanning) {
        // 根据选择模式设置光标模式
        switch (m_state) {
            case SelectionState::kSingleSelectionEntry:
            case SelectionState::kSingleSelectionQuery:
                // 单选模式下使用仅拾取框光标
                interactionData.cursorMode = CursorMode::kPickbox;
                // 单选模式下不显示任何标记
                interactionData.cursorMarker = CursorMarker::kNone;
                break;
            default:
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
                break;
        }
    }
    
    // 检查Shift状态，设置光标标记
    // TODO: 减选标记是位于实体上方才触发，Window和Crossing标记会覆盖加选减选标记
    // 暂时不处理减选加选的标记，预留注释
    // if (InputHandler::isShiftPressed()) {
    //     interactionData.cursorMarker = CursorMarker::kRemoveSelect;
    // }
    
    // 根据当前状态处理逻辑
    switch (m_state) {
        case SelectionState::kSingleSelectionEntry: {
            // 等待单选点，从命令中进入选择的入口
            InputContext::getInstance().waitForPoint(loc.get("selection.prompt.single"),
                {"F", "WP", "CP"}); // 选择对象:
            m_state = SelectionState::kSingleSelectionQuery;
            break;
        }
        case SelectionState::kSingleSelectionQuery: {
            // 鼠标移动时更新预选高亮（单选模式下实时预选）
            if (mouseMoved(m_previewPointScreen, m_lastPreSelectScreenPos)) {
                SelectionManager::getInstance().preSelectPick(m_previewPointWorld);
                m_lastPreSelectScreenPos = m_previewPointScreen;
            }

            // 检查输入状态
            InputStatus status = InputContext::getInstance().getCurrentStatus();
            // 点输入
            if (status == InputStatus::kPointInput) {
                glm::vec2 screenPos = InputHandler::getCursorPosition();
                m_initialPointScreen = screenPos;
                glm::dvec3 pickedPoint;
                InputContext::getInstance().getPickedPoint(pickedPoint);
                m_initialPointWorld = pickedPoint;

                // 使用选择管理器确认点选
                m_selectionResult = SelectionManager::getInstance().commitPick(m_initialPointWorld);
                if (!m_selectionResult.empty()) {
                    // 有实体，完成选择
                    m_selectionPointsWorld.push_back(m_initialPointWorld);
                    finishSelection();
                    m_inputStatus = InputStatus::kEntitySelection;
                } else {
                    // 没有实体，进入框选流程
                    m_state = SelectionState::kBoxSelectionEntry;
                    m_selectionMode = SelectionMode::kWindow;
                }
            }
            // 关键字输入
            else if (status == InputStatus::kKeywordInput) {
                std::string keyword;
                InputContext::getInstance().getKeyword(keyword);
                if (keyword == "F") {
                        m_selectionMode = SelectionMode::kFence;
                        interactionData.selectionMode = SelectionMode::kFence;
                        m_state = SelectionState::kFWpCpFirstPointEntry;
                    }
                    else if (keyword == "WP") {
                        m_selectionMode = SelectionMode::kWindowPolygon;
                        interactionData.selectionMode = SelectionMode::kWindowPolygon;
                        m_state = SelectionState::kFWpCpFirstPointEntry;
                    }
                    else if (keyword == "CP") {
                        m_selectionMode = SelectionMode::kCrossingPolygon;
                        interactionData.selectionMode = SelectionMode::kCrossingPolygon;
                        m_state = SelectionState::kFWpCpFirstPointEntry;
                    }
            }
            // Enter/Space 结束选择
            else if (status == InputStatus::kEnterInput) {
                finishSelection();
                m_inputStatus = InputStatus::kEnterInput;
            }
            // Esc
            else if (status == InputStatus::kCanceled) {
                cancelSelection();
                m_inputStatus = InputStatus::kCanceled;
            }
            break;
        }
        
        case SelectionState::kBoxSelectionEntry:
            // 等待初始点
            InputContext::getInstance().waitForPoint(loc.get("selection.prompt.box"),
                {"F", "WP", "CP"}); // 指定对角点或 [栏选(F)/圈围(WP)/圈交(CP)]:
            // 覆盖默认的错误提示
            InputContext::getInstance().setErrorPrompt(loc.get("selection.prompt.windowInvalid")); // 窗口说明无效。
            m_state = SelectionState::kBoxLassoSelectionChoice;
            break;
            
        case SelectionState::kBoxLassoSelectionChoice: {
            // 更新框选预览
            updateBoxSelection();

            // 鼠标移动时更新预选高亮（框选模式下实时预选）
            if (mouseMoved(m_previewPointScreen, m_lastPreSelectScreenPos)) {
                Geometry::AABB selectRect(m_initialPointWorld, m_previewPointWorld);
                if (m_selectionMode == SelectionMode::kWindow) {
                    SelectionManager::getInstance().preSelectWindow(selectRect);
                } else {
                    SelectionManager::getInstance().preSelectCrossing(selectRect);
                }
                m_lastPreSelectScreenPos = m_previewPointScreen;
            }

            // 处理框选到套索选择的切换
            if (InputHandler::isLeftMouseButtonPressed()) {
                float distance = glm::distance(m_previewPointScreen, m_initialPointScreen);
                const float lassoThreshold = 100.0f;
                if (distance > lassoThreshold) {
                    // 鼠标移动已经超过套索阈值，鼠标都还没有抬起，切换到套索选择
                    if (m_previewPointWorld.x < m_initialPointWorld.x) {
                        m_selectionMode = SelectionMode::kCrossingLasso;
                        m_lassoModeCycle = LassoModeCycle::kCrossing;
                        Utils::cmdLinePrint(loc.get("selection.prompt.lassoCrossing")); // 窗交(C) 套索  按空格键以循环选项
                    }
                    else {
                        m_selectionMode = SelectionMode::kWindowLasso;
                        m_lassoModeCycle = LassoModeCycle::kWindow;
                        Utils::cmdLinePrint(loc.get("selection.prompt.lassoWindow")); // 窗口(W) 套索  按空格键以循环选项
                    }
                    m_state = SelectionState::kLassoSelection;
                    m_selectionPointsWorld.clear();
                    m_selectionPointsWorld.push_back(m_initialPointWorld);
                    m_lastLassoPointScreen = m_initialPointScreen;
                    
                    // 更新交互数据
                    interactionData.selectionMode = m_selectionMode;
                    interactionData.selectionPointsWorld = m_selectionPointsWorld;
                    
                    // 套索模式是按住鼠标左键的状态下通过移动鼠标进入的，没有任何交互被接收，需要手动重置输入上下文状态
                    InputContext::getInstance().resetStatusExceptInteractionData();
                }
            }
            // 鼠标左键已经抬起，切换到框选
            else {
                m_state = SelectionState::kBoxSelectionQuery;
            }
            // 处理框选输入
            handleBoxSelection();
            break;
        }
        
        case SelectionState::kBoxSelectionQuery: {
            // 更新框选预览
            updateBoxSelection();
            
            // 鼠标移动时更新预选高亮（框选模式下实时预选）
            if (mouseMoved(m_previewPointScreen, m_lastPreSelectScreenPos)) {
                Geometry::AABB selectRect(m_initialPointWorld, m_previewPointWorld);
                if (m_selectionMode == SelectionMode::kWindow) {
                    SelectionManager::getInstance().preSelectWindow(selectRect);
                } else {
                    SelectionManager::getInstance().preSelectCrossing(selectRect);
                }
                m_lastPreSelectScreenPos = m_previewPointScreen;
            }
            
            // 处理框选输入
            handleBoxSelection();
            break;
        }
        
        case SelectionState::kLassoSelection: {
            // 鼠标左键处于按下状态，处理套索选择
            if (InputHandler::isLeftMouseButtonPressed()) {
                updateLassoSelection();
            }
            // 鼠标释放，完成套索选择
            else {
                // 添加最后一个预览点
                m_selectionPointsWorld.push_back(m_previewPointWorld);
                finishSelection();
                m_inputStatus = InputStatus::kEntitySelection;
            }
            
            // 回车空格循环切换套索模式
            InputStatus status = InputContext::getInstance().getCurrentStatus();
            if (status == InputStatus::kEnterInput) {
                switch (m_lassoModeCycle) {
                    case LassoModeCycle::kCrossing:
                        m_lassoModeCycle = LassoModeCycle::kWindow;
                        m_selectionMode = SelectionMode::kWindowLasso;
                        Utils::cmdLinePrint(loc.get("selection.prompt.lassoWindow")); // 窗口(W) 套索  按空格键以循环选项
                        break;
                    case LassoModeCycle::kWindow:
                        m_lassoModeCycle = LassoModeCycle::kFence;
                        m_selectionMode = SelectionMode::kFence;
                        Utils::cmdLinePrint(loc.get("selection.prompt.lassoFence")); // 栏选(F) 套索  按空格键以循环选项
                        break;
                    case LassoModeCycle::kFence:
                        m_lassoModeCycle = LassoModeCycle::kCrossing;
                        m_selectionMode = SelectionMode::kCrossingLasso;
                        Utils::cmdLinePrint(loc.get("selection.prompt.lassoCrossing")); // 窗交(C) 套索  按空格键以循环选项
                        break;
                }
                // 更新交互数据
                interactionData.selectionMode = m_selectionMode;
            }
            // Esc
            else if (status == InputStatus::kCanceled) {
                // TODO: Esc时套索选择选中0个实体，并返回
                cancelSelection();
                m_inputStatus = InputStatus::kEntitySelection;
            }
            break;
        }
        
        case SelectionState::kFWpCpFirstPointEntry: {
            // 清空选择点列表
            m_selectionPointsWorld.clear();
            
            // 根据选择模式设置不同的提示
            std::string prompt;
            if (m_selectionMode == SelectionMode::kFence) {
                prompt = loc.get("selection.prompt.fenceFirst"); // 指定第一个栏选点或拾取/拖动光标:
            } else if (m_selectionMode == SelectionMode::kWindowPolygon) {
                prompt = loc.get("selection.prompt.windowFirst"); // 指定第一个圈围点或拾取/拖动光标:
            } else if (m_selectionMode == SelectionMode::kCrossingPolygon) {
                prompt = loc.get("selection.prompt.crossingFirst"); // 指定第一个圈交点或拾取/拖动光标:
            }
            
            // 等待第一点
            InputContext::getInstance().waitForPoint(prompt, {});
            m_state = SelectionState::kFWpCpFirstPointQuery;
            break;
        }
        
        case SelectionState::kFWpCpFirstPointQuery: {
            // 检查输入状态
            InputStatus status = InputContext::getInstance().getCurrentStatus();
            // 点输入
            if (status == InputStatus::kPointInput) {
                // 获取第一点
                glm::dvec3 firstPoint;
                InputContext::getInstance().getPickedPoint(firstPoint);
                // 记录初始点
                m_initialPointWorld = firstPoint;
                m_initialPointScreen = InputHandler::getCursorPosition();
                m_selectionPointsWorld.push_back(firstPoint);
                
                // 更新交互数据
                interactionData.selectionInitialPointWorld = m_initialPointWorld;
                interactionData.selectionPointsWorld = m_selectionPointsWorld;
                
                // 进入套索选择决策状态
                m_state = SelectionState::kFWpCpLassoChoice;
            }
            // Enter/Space 结束选择
            else if (status == InputStatus::kEnterInput) {
                finishSelection();
                m_inputStatus = InputStatus::kEntitySelection;
            }
            // Esc
            else if (status == InputStatus::kCanceled) {
                cancelSelection();
                m_inputStatus = InputStatus::kCanceled;
            }
            break;
        }
        
        case SelectionState::kFWpCpLassoChoice: {
            // 更新预览点
            m_previewPointScreen = InputHandler::getCursorPosition();
            m_previewPointWorld = Renderer::getTransformManager().screenToWorld(m_previewPointScreen);
            interactionData.selectionPreviewPointWorld = m_previewPointWorld;
            
            // 处理套索选择的切换
            if (InputHandler::isLeftMouseButtonPressed()) {
                float distance = glm::distance(m_previewPointScreen, m_initialPointScreen);
                const float lassoThreshold = 100.0f;
                if (distance > lassoThreshold) {
                    // 切换到套索选择
                    if (m_selectionMode == SelectionMode::kFence) {
                        m_selectionMode = SelectionMode::kFence;
                        m_lassoModeCycle = LassoModeCycle::kFence;
                        Utils::cmdLinePrint(loc.get("selection.prompt.lassoFence")); // 栏选(F) 套索  按空格键以循环选项
                    } else if (m_selectionMode == SelectionMode::kWindowPolygon) {
                        m_selectionMode = SelectionMode::kWindowLasso;
                        m_lassoModeCycle = LassoModeCycle::kWindow;
                        Utils::cmdLinePrint(loc.get("selection.prompt.lassoWindow")); // 窗口(W) 套索  按空格键以循环选项
                    } else if (m_selectionMode == SelectionMode::kCrossingPolygon) {
                        m_selectionMode = SelectionMode::kCrossingLasso;
                        m_lassoModeCycle = LassoModeCycle::kCrossing;
                        Utils::cmdLinePrint(loc.get("selection.prompt.lassoCrossing")); // 窗交(C) 套索  按空格键以循环选项
                    }
                    
                    m_state = SelectionState::kLassoSelection;
                    m_selectionPointsWorld.clear();
                    m_selectionPointsWorld.push_back(m_initialPointWorld);
                    m_lastLassoPointScreen = m_initialPointScreen;
                    
                    // 更新交互数据
                    interactionData.selectionMode = m_selectionMode;
                    interactionData.selectionPointsWorld = m_selectionPointsWorld;
                    
                    // 套索模式是按住鼠标左键的状态下通过移动鼠标进入的，没有任何交互被接收，需要手动重置输入上下文状态
                    InputContext::getInstance().resetStatusExceptInteractionData();
                }
            }
            // 鼠标左键已经抬起，进入正常的选择流程
            else {
                if (m_selectionMode == SelectionMode::kFence) {
                    m_state = SelectionState::kFenceSelectionEntry;
                } else {
                    m_state = SelectionState::kPolygonSelectionEntry;
                }
            }
            break;
        }
        
        case SelectionState::kFenceSelectionEntry:
            // 等待栏选点，使用上一个点作为基点
            if (!m_selectionPointsWorld.empty()) {
                InputContext::getInstance().waitForPoint(loc.get("selection.prompt.fenceNext"),
                    m_selectionPointsWorld.back(), {"U"}); // 指定下一个栏选点或 [放弃(U)]:
            } else {
                InputContext::getInstance().waitForPoint(loc.get("selection.prompt.fenceNext"),
                    {"U"}); // 指定下一个栏选点或 [放弃(U)]:
            }
            m_state = SelectionState::kFenceSelectionQuery;
            break;
            
        case SelectionState::kFenceSelectionQuery: {
            // 检查输入状态
            InputStatus status = InputContext::getInstance().getCurrentStatus();
            // 点输入
            if (status == InputStatus::kPointInput) {
                // 添加栏选点
                glm::dvec3 newPointWorld;
                InputContext::getInstance().getPickedPoint(newPointWorld);
                m_selectionPointsWorld.push_back(newPointWorld);
                
                // 更新交互数据
                interactionData.selectionPointsWorld = m_selectionPointsWorld;
                // 继续选择下一点
                m_state = SelectionState::kFenceSelectionEntry;
            }
            // 关键字
            else if (status == InputStatus::kKeywordInput) {
                // 关键字输入
                std::string keyword;
                InputContext::getInstance().getKeyword(keyword);
                if (keyword == "U") {
                    // 撤销上一个点，初始点不进行撤销
                    if (m_selectionPointsWorld.size() > 1) {
                        m_selectionPointsWorld.pop_back();
                        interactionData.selectionPointsWorld = m_selectionPointsWorld;
                    }
                }
                // 继续选择下一点
                m_state = SelectionState::kFenceSelectionEntry;
            }
            // Enter/Space
            else if (status == InputStatus::kEnterInput) {
                // 完成栏选
                finishSelection();
                m_inputStatus = InputStatus::kEntitySelection;
            }
            // Esc
            else if (status == InputStatus::kCanceled) {
                cancelSelection();
                m_inputStatus = InputStatus::kCanceled;
            }
            break;
        }
        
        case SelectionState::kPolygonSelectionEntry:
            // 等待多边形点，使用上一个点作为基点
            if (!m_selectionPointsWorld.empty()) {
                InputContext::getInstance().waitForPoint(loc.get("selection.prompt.polygonNext"),
                    m_selectionPointsWorld.back(), {"U"}); // 指定直线的端点或 [放弃(U)]:
            } else {
                InputContext::getInstance().waitForPoint(loc.get("selection.prompt.polygonNext"),
                    {"U"}); // 指定直线的端点或 [放弃(U)]:
            }
            m_state = SelectionState::kPolygonSelectionQuery;
            break;
            
        case SelectionState::kPolygonSelectionQuery: {
            // 检查输入状态
            InputStatus status = InputContext::getInstance().getCurrentStatus();
            // 点输入
            if (status == InputStatus::kPointInput) {
                // 添加多边形点
                glm::dvec3 newPointWorld;
                InputContext::getInstance().getPickedPoint(newPointWorld);
                m_selectionPointsWorld.push_back(newPointWorld);
                
                // 更新交互数据
                interactionData.selectionPointsWorld = m_selectionPointsWorld;
                // 继续选择下一点
                m_state = SelectionState::kPolygonSelectionEntry;
            }
            // 处理关键字
            else if (status == InputStatus::kKeywordInput) {
                std::string keyword;
                InputContext::getInstance().getKeyword(keyword);
                if (keyword == "U") {
                    // 撤销上一个点，初始点不进行撤销
                    if (m_selectionPointsWorld.size() > 1) {
                        m_selectionPointsWorld.pop_back();
                        interactionData.selectionPointsWorld = m_selectionPointsWorld;
                    }
                }
                // 继续选择下一点
                m_state = SelectionState::kPolygonSelectionEntry;
            }
            // Enter/Space
            else if (status == InputStatus::kEnterInput) {
                finishSelection();
                m_inputStatus = InputStatus::kEntitySelection;
            }
            // Esc
            else if (status == InputStatus::kCanceled) {
                cancelSelection();
                m_inputStatus = InputStatus::kCanceled;
            }
            break;
        }
        
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
    m_inputStatus = InputStatus::kNone;
    m_selectionResult.clear();
}

bool SelectionTask::isSelecting() const {
    return m_state != SelectionState::kIdle && m_state != SelectionState::kCompleted;
}

InputStatus SelectionTask::getInputStatus() const {
    return m_inputStatus;
}

void SelectionTask::updateBoxSelection() {
    // 根据初始点和当前点的相对位置决定是窗口还是交叉选择（使用世界坐标）
    if (m_previewPointWorld.x < m_initialPointWorld.x) {
        m_selectionMode = SelectionMode::kCrossing;
    }
    else {
        m_selectionMode = SelectionMode::kWindow;
    }
    
    // 更新交互数据中的选择点和选择模式
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    interactionData.selectionMode = m_selectionMode;
    interactionData.selectionInitialPointWorld = m_initialPointWorld;
    interactionData.selectionPreviewPointWorld = m_previewPointWorld;
}

// 框选分支处理
void SelectionTask::handleBoxSelection()
{
    // 获取交互数据
    InteractionData& interactionData = InputContext::getInstance().getInteractionData();
    
    // 检查输入状态
    InputStatus status = InputContext::getInstance().getCurrentStatus();
    // 第二点输入
    if (status == InputStatus::kPointInput) {
        glm::dvec3 secondPoint;
        if (InputContext::getInstance().getPickedPoint(secondPoint)) {
            // 使用选择管理器确认框选
            Geometry::AABB selectRect(m_initialPointWorld, secondPoint);
            if (m_selectionMode == SelectionMode::kWindow) {
                m_selectionResult = SelectionManager::getInstance().commitWindow(selectRect);
            } else {
                m_selectionResult = SelectionManager::getInstance().commitCrossing(selectRect);
            }
            // 完成框选
            finishSelection();
            m_inputStatus = InputStatus::kEntitySelection;
        }
    }
    // 关键字输入
    else if (status == InputStatus::kKeywordInput) {
        
        std::string keyword;
        InputContext::getInstance().getKeyword(keyword);
        if (keyword == "F") {
            m_selectionMode = SelectionMode::kFence;
            interactionData.selectionMode = SelectionMode::kFence;
            m_state = SelectionState::kFenceSelectionEntry;
        }
        else if (keyword == "WP") {
            m_selectionMode = SelectionMode::kWindowPolygon;
            interactionData.selectionMode = SelectionMode::kWindowPolygon;
            m_state = SelectionState::kPolygonSelectionEntry;
        }
        else if (keyword == "CP") {
            m_selectionMode = SelectionMode::kCrossingPolygon;
            interactionData.selectionMode = SelectionMode::kCrossingPolygon;
            m_state = SelectionState::kPolygonSelectionEntry;
        }
        m_selectionPointsWorld.clear();
        m_selectionPointsWorld.push_back(m_initialPointWorld);
        
        // 更新交互数据
        interactionData.isSelectionActive = true;
        interactionData.selectionMode = m_selectionMode;
        interactionData.selectionPointsWorld = m_selectionPointsWorld;
    }
    // Enter
    else if (status == InputStatus::kEnterInput) {
        // Enter作为分支中进行处理的合法输入，InputContext不会输出错误提示，所以这里需要进行手动输出
        auto& loc = LocalizationManager::getInstance();
        Utils::cmdLinePrint(loc.get("selection.prompt.windowInvalid")); // 窗口说明无效。
        m_state = SelectionState::kBoxSelectionQuery;
    }
    // Esc
    else if (status == InputStatus::kCanceled) {
        cancelSelection();
        m_inputStatus = InputStatus::kCanceled;
    }
}

void SelectionTask::updateLassoSelection() {
    // 套索模式下，需要实时添加点，只在两点屏幕距离足够远时才添加
    const float minDistance = 10.0f; // 最小距离阈值
    if (m_selectionPointsWorld.empty()) {
        m_selectionPointsWorld.push_back(m_initialPointWorld);
        m_lastLassoPointScreen = m_initialPointScreen;
    }
    else {
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


// 结束选择，确认选择结果
void SelectionTask::finishSelection() {
    // 清除预选状态
    SelectionManager::getInstance().clearPreSelect();
    
    // 管理任务状态
    m_state = SelectionState::kCompleted;
    m_completed = true;
    
    // 重置输入上下文状态
    InputContext::getInstance().resetStatus();
}

// 取消选择，清理状态与数据
void SelectionTask::cancelSelection() {
    // 清除预选状态
    SelectionManager::getInstance().clearPreSelect();
    
    // 清空选择结果
    m_selectionResult.clear();
    
    // 管理任务状态
    m_state = SelectionState::kCompleted;
    m_completed = true;
    
    // 重置输入上下文状态
    InputContext::getInstance().resetStatus();
}

} // namespace tch