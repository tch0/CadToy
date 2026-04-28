#pragma once

// C++ 标准库
#include <vector>

// 第三方库
#include <glm/glm.hpp>

// 项目头文件

namespace tch {

// 输入状态枚举
enum class InputStatus {
    kNone,              // 无输入
    kCanceled,          // 取消输入(Esc键)
    kEnterInput,        // 回车输入(空格和回车作用一致，也包括在其中)
    kIntegerInput,      // 整数输入
    kFloatInput,        // 浮点数输入
    kStringInput,       // 字符串输入
    kKeywordInput,      // 关键字输入
    kPointInput,        // 点坐标输入
    kEntitySelection    // 实体选择输入
};

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
    kNone,              // 无选择模式，未处于选择交互中
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
private:
    // 光标栈：用于临时切换光标，如平移时，因为平移不参与任何输入且可能在任何时候发生，所以以这种方式实现
    std::vector<std::pair<CursorMode, CursorMarker>> cursorStack;
    
public:
    // 光标相关数据
    CursorMode cursorMode = CursorMode::kDefault;       // 当前光标模式
    CursorMarker cursorMarker = CursorMarker::kNone;     // 当前光标标记
    
    // 选择相关数据
    bool isSelectionActive = false;                     // 是否正在进行选择
    SelectionMode selectionMode = SelectionMode::kNone; // 当前选择模式
    glm::dvec3 selectionInitialPointWorld = glm::dvec3(0.0, 0.0, 0.0); // 所有选择的初始点
    glm::dvec3 selectionPreviewPointWorld = glm::dvec3(0.0, 0.0, 0.0); // 所有选择的当前预览点
    std::vector<glm::dvec3> selectionPointsWorld; // 选择点集合（世界坐标，用于套索、多边形、栏选等）
    
    // RubberBand 橡皮线相关数据
    bool isRubberBandVisible = false;                           // 是否显示橡皮线
    glm::dvec3 rubberBandStartWorld = glm::dvec3(0.0);  // 橡皮线起点（世界坐标）
    glm::dvec3 rubberBandEndWorld = glm::dvec3(0.0);    // 橡皮线终点（世界坐标）
    
    // 后续可添加的其他数据
    // 例如：
    // - 捕捉相关数据
    // - 极轴追踪相关数据
    // - 命令预览相关数据
    // - 选择集相关数据
    
    // 更新橡皮线数据
    void updateRubberBand(const glm::dvec3& start, const glm::dvec3& end) {
        isRubberBandVisible = true;
        rubberBandStartWorld = start;
        rubberBandEndWorld = end;
    }
    
    // 清除橡皮线数据
    void clearRubberBand() {
        isRubberBandVisible = false;
        rubberBandStartWorld = glm::dvec3(0.0);
        rubberBandEndWorld = glm::dvec3(0.0);
    }
    
    // 压入当前光标状态并设置新光标
    void pushAndSetCursor(CursorMode mode, CursorMarker marker = CursorMarker::kNone) {
        cursorStack.push_back({cursorMode, cursorMarker});
        cursorMode = mode;
        cursorMarker = marker;
    }
    
    // 从栈弹出恢复光标状态
    void popCursor() {
        if (!cursorStack.empty()) {
            auto [mode, marker] = cursorStack.back();
            cursorMode = mode;
            cursorMarker = marker;
            cursorStack.pop_back();
        } else {
            cursorMode = CursorMode::kDefault;
            cursorMarker = CursorMarker::kNone;
        }
    }
    
    // 重置光标状态
    void resetCursor() {
        cursorMode = CursorMode::kDefault;
        cursorMarker = CursorMarker::kNone;
        cursorStack.clear();
    }
    
    // 重置选择状态
    void resetSelection() {
        isSelectionActive = false;
        selectionMode = SelectionMode::kNone;
        selectionInitialPointWorld = glm::dvec3(0.0, 0.0, 0.0);
        selectionPreviewPointWorld = glm::dvec3(0.0, 0.0, 0.0);
        selectionPointsWorld.clear();
    }
    
    // 重置所有状态
    void reset() {
        resetCursor();
        resetSelection();
        clearRubberBand();
    }
};

} // namespace tch