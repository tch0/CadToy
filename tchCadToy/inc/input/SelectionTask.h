#pragma once

#include "Task.h"
#include "common/CommonTypes.h"
#include <vector>
#include <string>

namespace tch {

// 选择任务类，统一管理所有选择模式
class SelectionTask : public Task {
public:
    SelectionTask();
    ~SelectionTask() override = default;
    void onUpdate() override;
    bool isCompleted() const override;
    void reset() override;
    // 开始选择任务
    void start(bool isCommandActive = false, bool isSingleSelect = false);
    // 处理鼠标左键点击
    void handleLeftMouseClick();
    // 处理键盘输入
    void handleKeyword(const std::string& keyword);
    // 处理回车和空格键
    void handleEnterSpace();
    // 处理Escape键
    void handleEscape();
    // 获取选择模式
    SelectionMode getSelectionMode() const;
    // 获取选择点
    const std::vector<glm::dvec3>& getSelectionPoints() const;
    // 是否正在选择
    bool isSelecting() const;
    
private:
    // 选择状态
    enum class SelectionState {
        kIdle,           // 空闲状态
        kInitialPoint,   // 等待初始点
        kSingleSelecting, // 单选模式
        kBoxSelecting,   // 框选模式
        kLassoSelecting, // 套索选择
        kPolygonSelecting, // 多边形选择
        kFenceSelecting,   // 栏选
        kCompleted       // 完成
    };
    
    // 套索模式循环顺序
    enum class LassoModeCycle {
        kCrossing,
        kWindow,
        kFence
    };
    
    // 成员变量
    SelectionState m_state;               // 当前选择状态
    SelectionMode m_selectionMode;         // 当前选择模式
    std::vector<glm::dvec3> m_selectionPointsWorld; // 选择点集合（世界坐标）
    glm::dvec3 m_initialPointWorld;             // 初始点（世界坐标）
    glm::dvec3 m_previewPointWorld;             // 预览点（世界坐标）
    glm::vec2 m_initialPointScreen;            // 初始点（屏幕坐标）
    glm::vec2 m_previewPointScreen;            // 预览点（屏幕坐标）
    glm::vec2 m_lastLassoPointScreen;          // 上一次套索的最后一点（屏幕坐标）
    bool m_completed;                     // 任务是否完成
    LassoModeCycle m_lassoModeCycle;      // 套索模式循环状态
    std::string m_currentPrompt;           // 当前提示信息
    bool m_isCommandActive;               // 是否在命令中执行
    
    // 内部方法
    void handleBoxSelection();            // 处理框选
    void handleLassoSelection();          // 处理套索选择
    void handlePolygonSelection();         // 处理多边形选择
    void handleFenceSelection();           // 处理栏选
    void switchToMode(SelectionMode mode); // 切换到指定选择模式
    void finishSelection();                // 结束选择
    void updatePrompt();                   // 更新提示信息
};

} // namespace tch