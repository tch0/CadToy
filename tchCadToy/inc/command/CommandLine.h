#pragma once

// C++ 标准库
#include <vector>

// 第三方库
#include <glm/glm.hpp>

// 项目头文件
#include "Command.h"
#include "DbCommon.h"
#include "Database.h"


namespace tch {

// 线段命令
class CommandLine : public Command {
private:
    // line命令状态枚举
    enum CommandLineState {
        kStartPointEntry,       // 起点入口
        kStartPointQuery,       // 起点输入查询
        kNextPointEntry,        // 下一点入口（等待输入，同时更新预览）
        kNextPointQuery,        // 下一点输入查询
        kCompleted              // 结束状态
    };
    
    CommandLineState m_state;           // 命令状态
    glm::dvec3 m_firstPoint;            // 第一条线段的起点（用于闭合）
    glm::dvec3 m_startPoint;            // 当前线段的起点（最后确定的点）
    glm::dvec3 m_currentPoint;          // 鼠标当前位置（预览终点）
    std::vector<ObjectId> m_lineIds;    // 所有入库线段的ID
    Database* m_pDb;                    // 数据库指针（构造时获取）
    
    // 辅助方法
    void createNewLine(const glm::dvec3& start, const glm::dvec3& end);
    void updateLastLineEnd(const glm::dvec3& end);
    void removeLastLine();
    void finalizeLastLine();

public:
    CommandLine();
    
    // 命令更新方法
    void onUpdate() override;
};

} // namespace tch
