#pragma once

// C++ 标准库
#include <vector>

// 第三方库
#include <glm/glm.hpp>

// 项目头文件
#include "Command.h"

namespace tch {

// 线段命令
class CommandLine : public Command {
private:
    // line命令状态枚举
    enum class CommandLineState {
        kStartPointEntry,       // 起点入口
        kStartPointQuery,       // 起点输入查询
        kNextPointEntry,        // 下一点入口
        kNextPointQuery,        // 下一点输入查询
        kCompleted              // 结束状态
    };
    
    CommandLineState m_state;    // 命令状态
    glm::dvec3 m_startPoint;     // 起点
    glm::dvec3 m_currentPoint;   // 当前点（用于预览）
    std::vector<glm::dvec3> m_points; // 保存的点（用于绘制历史线段）

public:
    CommandLine();
    
    // 命令更新方法
    void onUpdate() override;
    
    // 绘制预览
    void drawPreview() override;
};

} // namespace tch