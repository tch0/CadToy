#pragma once

// C++ 标准库

// 第三方库
#include <glm/glm.hpp>

// 项目头文件
#include "Command.h"
#include "DbCommon.h"


namespace tch {

// 圆命令
class CommandCircle : public Command {
public:
    CommandCircle();

    void onUpdate() override;

private:
    enum CommandCircleState {
        kCenterEntry,           // 圆心入口
        kCenterQuery,           // 圆心输入查询

        kRadiusPointEntry,      // 半径点入口
        kRadiusPointQuery,      // 半径点输入查询

        kDiameterPointEntry,    // 直径点入口
        kDiameterPointQuery,    // 直径点输入查询

        k3P_FirstPointEntry,    // 3P第一点入口
        k3P_FirstPointQuery,    // 3P第一点输入查询
        k3P_SecondPointEntry,   // 3P第二点入口
        k3P_SecondPointQuery,   // 3P第二点输入查询
        k3P_ThirdPointEntry,    // 3P第三点入口
        k3P_ThirdPointQuery,    // 3P第三点输入查询

        k2P_FirstPointEntry,    // 2P第一点入口
        k2P_FirstPointQuery,    // 2P第一点输入查询
        k2P_SecondPointEntry,   // 2P第二点入口
        k2P_SecondPointQuery,   // 2P第二点输入查询

        kT_FirstTangentEntry,   // T第一点入口
        kT_FirstTangentQuery,   // T第一点输入查询
        kT_SecondTangentEntry,  // T第二点入口
        kT_SecondTangentQuery,  // T第二点输入查询
        kT_RadiusEntry,         // T半径入口
        kT_RadiusQuery,         // T半径输入查询

        kCompleted              // 结束状态
    };

    CommandCircleState m_state; // 命令状态
    glm::dvec3 m_center;        // 圆心
    glm::dvec3 m_firstPoint;    // 第一点（3P/2P/T模式用）
    glm::dvec3 m_secondPoint;   // 第二点（3P/2P/T模式用）
    double m_radius;            // 半径
    ObjectId m_previewCircleId; // 预览圆ID，0表示无预览，有预览时如果创建成功预览圆就是最终圆

    // 创建圆并入库（记录undo）
    void createCircle(const glm::dvec3& center, double radius);

    // 创建预览圆（记录undo，用于预览）
    void createPreviewCircle(const glm::dvec3& center, double radius);

    // 更新预览圆（不记录undo）
    void updatePreviewCircle(const glm::dvec3& center, double radius);

    // 删除预览圆（记录undo）
    void removePreviewCircle();

    // 输出无效提示并结束
    void failWithInvalid();

    // 静态默认半径（上次创建的圆的半径）
    static double s_defaultRadius;
};

} // namespace tch
