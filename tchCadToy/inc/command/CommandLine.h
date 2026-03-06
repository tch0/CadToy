#pragma once

#include "command/Command.h"
#include <glm/glm.hpp>
#include <vector>

namespace tch {

// 线段命令状态枚举
enum class CommandLineState {
    kWaitForStartPoint,  // 等待起点
    kWaitForNextPoint    // 等待下一点
};

// 线段命令
class CommandLine : public Command {
private:
    CommandLineState m_state;    // 命令状态
    glm::dvec3 m_startPoint;     // 起点
    glm::dvec3 m_currentPoint;   // 当前点（用于预览）
    std::vector<glm::dvec3> m_points; // 保存的点（用于绘制历史线段）
    bool m_completed;            // 命令是否完成
    bool m_canceled;             // 命令是否被取消

public:
    CommandLine();
    
    // 命令更新方法
    void onUpdate() override;
    
    // 命令是否完成
    bool isCompleted() const override;
    
    // 取消命令
    void cancel() override;
    
    // 绘制预览
    void drawPreview() override;
};

} // namespace tch