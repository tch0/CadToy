#pragma once

// C++ 标准库
#include <string>
#include <vector>

// 第三方库

// 项目头文件
#include "CommonTypes.h"
#include "Task.h"
#include "SelectionSet.h"

namespace tch {

// 选择任务类，统一管理所有选择模式
class SelectionTask : public Task {
public:
    SelectionTask();
    ~SelectionTask() override = default;
    
    // 开始选择任务
    void start(bool isCommandActive = false, bool isSingleSelect = false);
    // 更新状态机
    void onUpdate() override;
    // 选择任务已完成
    bool isCompleted() const override;
    // 重置选择任务状态
    void reset() override;
    // 是否正在选择
    bool isSelecting() const;
    // 获取输入状态，返回给InputContext处理
    InputStatus getInputStatus() const;
    
    // 获取选择结果
    const SelectionSet& getSelectionResult() const { return m_selectionResult; }

private:
    // 选择状态
    enum class SelectionState {
        kIdle,                          // 空闲状态
        kSingleSelectionEntry,          // 单选模式入口
        kSingleSelectionQuery,          // 单选模式输入查询
        kBoxSelectionEntry,             // 框选模式入口
        kBoxLassoSelectionChoice,       // 框选或套索选择决策
        kBoxSelectionQuery,             // 框选模式输入查询
        kLassoSelection,                // 套索选择
        kFWpCpFirstPointEntry,          // F/WP/CP 第一点入口
        kFWpCpFirstPointQuery,          // F/WP/CP 第一点查询
        kFWpCpLassoChoice,              // F/WP/CP 套索选择决策
        kFenceSelectionEntry,           // 栏选模式入口
        kFenceSelectionQuery,           // 栏选模式输入查询
        kPolygonSelectionEntry,         // 多边形选择入口
        kPolygonSelectionQuery,         // 多边形选择输入查询
        kCompleted                      // 完成
    };
    
    // 套索模式循环顺序
    enum class LassoModeCycle {
        kCrossing,
        kWindow,
        kFence
    };
    
    // 成员变量
    SelectionState m_state;                     // 当前选择状态
    SelectionMode m_selectionMode;              // 当前选择模式
    std::vector<glm::dvec3> m_selectionPointsWorld; // 选择点集合（世界坐标）
    glm::dvec3 m_initialPointWorld;             // 初始点（世界坐标）
    glm::dvec3 m_previewPointWorld;             // 预览点（世界坐标）
    glm::vec2 m_initialPointScreen;             // 初始点（屏幕坐标）
    glm::vec2 m_previewPointScreen;             // 预览点（屏幕坐标）
    glm::vec2 m_lastLassoPointScreen;           // 上一次套索的最后一点（屏幕坐标）
    glm::vec2 m_lastPreSelectScreenPos;         // 上次预选查询的屏幕坐标（用于预选查询优化，鼠标未移动则不会进行预选查询，初始值0,0）
    bool m_completed;                           // 任务是否完成
    LassoModeCycle m_lassoModeCycle;            // 套索模式循环状态
    std::string m_currentPrompt;                // 当前提示信息
    bool m_isCommandActive;                     // 是否在命令中执行
    InputStatus m_inputStatus;                  // 保存输入状态
    SelectionSet m_selectionResult;             // 选择结果
    
    // 内部方法
    void updateBoxSelection();          // 更新框选预览数据
    void handleBoxSelection();          // 框选分支处理
    void updateLassoSelection();        // 处理套索选择
    void finishSelection();             // 结束选择
    void cancelSelection();             // 取消选择
    
    // 静态工具方法
    static bool mouseMoved(const glm::vec2& current, const glm::vec2& last); // 判断两帧之间鼠标是否移动，用于优化
};

} // namespace tch