#pragma once

#include <glm/glm.hpp>

namespace tch {

// 光标模式枚举
enum class CursorMode {
    kDefault,       // 拾取框加外部十字光标
    kCrosshair,     // 仅十字光标
    kPickbox,       // 仅拾取框
    kPanning        // 平移中，手掌
};

// 光标标记枚举（按优先级从高到低排列），优先级并非完全定死，只是大体上的优先级，
// 可能根据命令模式、交互状态等情况有一些例外需动态调整：例如erase命令框选时选择标记可在选择到了实体后切换为删除标记
enum class CursorMarker {
    kNone,              // 无标记
    // 第一梯队：禁止与限制
    kLocked,            // 锁定标记
    // 第二梯队：约束
    kOrthogonal,        // 正交标记(⊥)
    // 第三梯队：即时编辑动作
    kErase,             // 删除标记
    kCopy,              // 复制标记
    kMove,              // 移动标记
    kRotate,            // 旋转标记
    kScale,             // 缩放标记
    // 第四梯队：选择模式
    kAddSelect,         // 加选标记(+号)
    kRemoveSelect,      // 减选标记(-号)
    // 特殊梯队：框选模式，鼠标瞬时状态（拖动时覆盖其他标记）
    kCrossingSelect,    // 交叉选择（左拉）
    kWindowSelect,      // 窗口选择（右拉）
};

// 选择模式枚举
enum class SelectionMode {
    kNone,              // 无选择
    kSingle,            // 单点选择
    kWindow,            // 窗口选择（框选右拉）
    kCrossing,          // 交叉选择（框选左拉）
    kFence,             // 围栏选择
    kWindowLasso,       // 套索窗口选择
    kCrossingLasso,     // 套索交叉选择
    kWindowPolygon,     // 多边形窗口选择（圈围）
    kCrossingPolygon,   // 多边形交叉选择（圈交）
    kAll                // 全选
};

// 交互数据结构体，用于InputContext和Renderer交换瞬态数据
struct InteractionData {
    // 光标相关数据
    CursorMode cursorMode = CursorMode::kDefault;       // 当前光标模式
    CursorMarker cursorMarker = CursorMarker::kNone;     // 当前光标标记
    glm::vec2 cursorScreenPos = glm::vec2(0.0f, 0.0f);  // 光标屏幕坐标
    glm::dvec3 cursorWorldPos = glm::dvec3(0.0, 0.0, 0.0); // 光标世界坐标
    
    // 选择相关数据
    bool isSelectionActive = false;                     // 是否正在进行选择
    SelectionMode selectionMode = SelectionMode::kNone; // 当前选择模式
    // 框选
    glm::vec2 selectionBoxStart = glm::vec2(0.0f, 0.0f); // 选择框起点屏幕坐标
    glm::vec2 selectionBoxCurrent = glm::vec2(0.0f, 0.0f); // 选择框当前点屏幕坐标
    
    // 后续可添加的其他数据
    // 例如：
    // - 其他选择数据：栏选、套索、多边形
    // - 捕捉相关数据
    // - 极轴追踪相关数据
    // - 命令预览相关数据
    // - 选择集相关数据
    
    // 重置光标状态
    void resetCursor() {
        cursorMode = CursorMode::kDefault;
        cursorMarker = CursorMarker::kNone;
        cursorScreenPos = glm::vec2(0.0f, 0.0f);
        cursorWorldPos = glm::dvec3(0.0, 0.0, 0.0);
    }
    
    // 重置选择状态
    void resetSelection() {
        isSelectionActive = false;
        selectionMode = SelectionMode::kNone;
        selectionBoxStart = glm::vec2(0.0f, 0.0f);
        selectionBoxCurrent = glm::vec2(0.0f, 0.0f);
    }
    
    // 重置所有状态
    void reset() {
        resetCursor();
        resetSelection();
        // 后续添加其他数据的重置
    }
};

} // namespace tch