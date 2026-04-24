// 对应头文件
#include "CommandCircle.h"

// C++ 标准库
#include <cfloat>

// 第三方库
#include <glm/glm.hpp>

// 项目头文件
#include "InputContext.h"
#include "InputHandler.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "DocManager.h"
#include "Database.h"
#include "DbCircle.h"
#include "UndoManager.h"
#include "Geometry.h"
#include "StringUtils.h"


namespace tch {

// 静态默认半径初始化
double CommandCircle::s_defaultRadius = 100.0;

CommandCircle::CommandCircle()
    : m_state(kCenterEntry)
    , m_center(0, 0, 0)
    , m_firstPoint(0, 0, 0)
    , m_secondPoint(0, 0, 0)
    , m_radius(0)
    , m_previewCircleId(0) {
}

// 更新预览圆（ID无效时创建，有效时更新；预览圆就是最终圆）
void CommandCircle::updatePreviewCircle(const glm::dvec3& center, double radius) {
    auto& db = *DocManager::getCurrentDocument().getDatabase();

    if (m_previewCircleId == 0) {
        // ID无效，创建新圆
        auto circle = std::make_unique<DbCircle>(center, radius);
        circle->setPropertiesFromDb();
        m_previewCircleId = db.addObject(std::move(circle));
        // 记录实体添加
        UndoManager::getInstance().recordAdd(m_previewCircleId);
    } else {
        // ID有效，更新现有圆
        auto* pEntity = db.getEntity(m_previewCircleId);
        if (pEntity) {
            auto* pCircle = pEntity->as<DbCircle>();
            if (pCircle) {
                pCircle->setCenter(center);
                pCircle->setRadius(radius);
            }
        }
    }
}

// 删除预览圆（记录undo，在kCreateFailed状态统一调用）
void CommandCircle::removePreviewCircle() {
    if (m_previewCircleId == 0) {
        return;
    }

    auto& db = *DocManager::getCurrentDocument().getDatabase();
    db.removeObject(m_previewCircleId);
    // 记录实体删除
    UndoManager::getInstance().recordRemove(m_previewCircleId);
    m_previewCircleId = 0;
}

// 输出无效提示，切换到kCreateFailed状态
void CommandCircle::failWithInvalid() {
    auto& loc = LocalizationManager::getInstance();
    Utils::cmdLinePrint(loc.get("command.circle.invalid")); // 圆不存在。
    m_state = kCreateFailed;
}

void CommandCircle::onUpdate() {
    if (isCompleted()) {
        return;
    }

    auto& ctx = InputContext::getInstance();
    auto& loc = LocalizationManager::getInstance();

    switch (m_state) {
        // ============================================================================
        // 主入口：获取圆心或选择模式
        // ============================================================================
        case kCenterEntry:
            m_state = kCenterQuery;
            // 指定圆的圆心或 [三点(3P)/两点(2P)/切点、切点、半径(T)]:
            ctx.waitForPoint(loc.get("command.circle.centerPrompt"), {}, {"3P", "2P", "T"});
            break;

        case kCenterQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：等待中，继续等待
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc：取消命令，结束（预览圆尚未创建）
            else if (status == InputStatus::kCanceled) {
                m_state = kCompleted;
            }
            // Enter/Space：结束命令（无圆心，直接结束）
            else if (status == InputStatus::kEnterInput) {
                m_state = kCompleted;
            }
            // 点输入：获取圆心，进入半径输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_center)) {
                    m_state = kRadiusPointEntry;
                }
            }
            // 关键字输入：选择3P/2P/T模式
            else if (status == InputStatus::kKeywordInput) {
                std::string keyword;
                ctx.getKeyword(keyword);
                if (keyword == "3P") {
                    m_state = k3P_FirstPointEntry;
                }
                else if (keyword == "2P") {
                    m_state = k2P_FirstPointEntry;
                }
                else if (keyword == "T") {
                    m_state = kT_FirstTangentEntry;
                }
            }
            break;
        }

        // ============================================================================
        // 圆心+半径模式
        // ============================================================================
        case kRadiusPointEntry:
            m_state = kRadiusPointQuery;
            // 指定圆的半径或 [直径(D)] <默认值>:
            ctx.waitForPoint(StringUtils::format(loc.get("command.circle.radiusPrompt"), s_defaultRadius), m_center, {"D"});
            break;

        case kRadiusPointQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：鼠标移动中，更新预览圆半径
            if (status == InputStatus::kNone) {
                double radius = glm::distance(m_center, ctx.getPreviewPoint());
                updatePreviewCircle(m_center, radius);
                break;
            }
            // Esc：取消命令，切换到创建失败状态（统一清理预览圆）
            else if (status == InputStatus::kCanceled) {
                m_state = kCreateFailed;
            }
            // Enter/Space：使用默认半径更新预览圆（预览圆就是最终圆），结束
            else if (status == InputStatus::kEnterInput) {
                updatePreviewCircle(m_center, s_defaultRadius);
                m_state = kCompleted;
            }
            // 点输入：计算半径，检查是否重合（半径为0），更新预览圆（预览圆就是最终圆）或提示错误
            else if (status == InputStatus::kPointInput) {
                glm::dvec3 point;
                if (ctx.getPickedPoint(point)) {
                    // 检查点是否与圆心重合（半径为0）
                    if (Geometry::isCoincident(m_center, point)) {
                        // 半径为0，输出错误提示，继续等待输入
                        Utils::cmdLinePrint(loc.get("command.circle.radiusZero")); // 半径为0，无效圆，重新输入正值或指定点。
                        // 重新进入半径输入状态
                        m_state = kRadiusPointEntry;
                    }
                    else {
                        m_radius = glm::distance(m_center, point);
                        updatePreviewCircle(m_center, m_radius);
                        s_defaultRadius = m_radius;
                        m_state = kCompleted;
                    }
                }
            }
            // 关键字D：进入直径模式
            else if (status == InputStatus::kKeywordInput) {
                std::string keyword;
                ctx.getKeyword(keyword);
                if (keyword == "D") {
                    m_state = kDiameterPointEntry;
                }
            }
            break;
        }

        // ============================================================================
        // 圆心+直径模式
        // ============================================================================
        case kDiameterPointEntry:
            m_state = kDiameterPointQuery;
            // 指定圆的直径 <默认值>:
            ctx.waitForPoint(StringUtils::format(loc.get("command.circle.diameterPrompt"), s_defaultRadius * 2), m_center, {});
            break;

        case kDiameterPointQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：鼠标移动中，更新预览圆半径
            if (status == InputStatus::kNone) {
                // 点到圆心的距离是直径，半径需要除以2
                double radius = glm::distance(m_center, ctx.getPreviewPoint()) / 2.0;
                updatePreviewCircle(m_center, radius);
                break;
            }
            // Esc：取消命令，切换到创建失败状态（统一清理预览圆）
            else if (status == InputStatus::kCanceled) {
                m_state = kCreateFailed;
            }
            // Enter/Space：使用默认直径（默认半径*2）更新预览圆（预览圆就是最终圆），结束
            else if (status == InputStatus::kEnterInput) {
                updatePreviewCircle(m_center, s_defaultRadius);
                m_state = kCompleted;
            }
            // 点输入：计算半径（点到圆心距离是直径，除以2），检查是否重合（直径为0），更新预览圆（预览圆就是最终圆）或提示错误
            else if (status == InputStatus::kPointInput) {
                glm::dvec3 point;
                if (ctx.getPickedPoint(point)) {
                    // 检查点是否与圆心重合（直径为0）
                    if (Geometry::isCoincident(m_center, point)) {
                        // 直径为0，输出错误提示，继续等待输入
                        Utils::cmdLinePrint(loc.get("command.circle.diameterZero")); // 直径为0，无效圆，重新输入正值或指定点。
                        // 重新进入直径输入状态
                        m_state = kDiameterPointEntry;
                    }
                    else {
                        // 点到圆心的距离是直径，半径需要除以2
                        double radius = glm::distance(m_center, point) / 2.0;
                        updatePreviewCircle(m_center, radius);
                        s_defaultRadius = radius;
                        m_state = kCompleted;
                    }
                }
            }
            break;
        }

        // ============================================================================
        // 三点模式 (3P)
        // ============================================================================
        case k3P_FirstPointEntry:
            m_state = k3P_FirstPointQuery;
            // 指定圆上的第一个点:
            ctx.waitForPoint(loc.get("command.circle.3p.firstPoint"));
            break;

        case k3P_FirstPointQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：等待中
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc：取消命令，结束（预览圆尚未创建）
            else if (status == InputStatus::kCanceled) {
                m_state = kCompleted;
            }
            // Enter/Space：结束命令
            else if (status == InputStatus::kEnterInput) {
                m_state = kCompleted;
            }
            // 点输入：获取第一点，进入第二点输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_firstPoint)) {
                    m_state = k3P_SecondPointEntry;
                }
            }
            break;
        }

        case k3P_SecondPointEntry:
            m_state = k3P_SecondPointQuery;
            // 指定圆上的第二个点:
            ctx.waitForPoint(loc.get("command.circle.3p.secondPoint"));
            break;

        case k3P_SecondPointQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：等待中
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc：取消命令，结束（预览圆尚未创建）
            else if (status == InputStatus::kCanceled) {
                m_state = kCompleted;
            }
            // Enter/Space：结束命令
            else if (status == InputStatus::kEnterInput) {
                m_state = kCompleted;
            }
            // 点输入：获取第二点，检查与第一点重合，进入第三点输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_secondPoint)) {
                    // 检查是否与第一点重合
                    if (Geometry::isCoincident(m_firstPoint, m_secondPoint)) {
                        failWithInvalid();
                    }
                    else {
                        m_state = k3P_ThirdPointEntry;
                    }
                }
            }
            break;
        }

        case k3P_ThirdPointEntry:
            m_state = k3P_ThirdPointQuery;
            // 指定圆上的第三个点:
            ctx.waitForPoint(loc.get("command.circle.3p.thirdPoint"));
            break;

        case k3P_ThirdPointQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：鼠标移动中，更新预览圆
            if (status == InputStatus::kNone) {
                auto [success, circle] = Geometry::Circle::fromThreePoints(
                    m_firstPoint, m_secondPoint, ctx.getPreviewPoint());
                if (success) {
                    updatePreviewCircle(circle.center, circle.radius);
                }
                break;
            }
            // Esc：取消命令，切换到创建失败状态（统一清理预览圆）
            else if (status == InputStatus::kCanceled) {
                m_state = kCreateFailed;
            }
            // Enter/Space：结束命令
            else if (status == InputStatus::kEnterInput) {
                m_state = kCreateFailed;
            }
            // 点输入：获取第三点，检查重合，计算圆，更新预览圆（预览圆就是最终圆）或提示错误
            else if (status == InputStatus::kPointInput) {
                glm::dvec3 thirdPoint;
                if (ctx.getPickedPoint(thirdPoint)) {
                    // 检查是否与前两点重合
                    if (Geometry::isCoincident(m_firstPoint, thirdPoint) ||
                        Geometry::isCoincident(m_secondPoint, thirdPoint)) {
                        failWithInvalid();
                    }
                    else {
                        // 计算圆
                        auto [success, circle] = Geometry::Circle::fromThreePoints(
                            m_firstPoint, m_secondPoint, thirdPoint);
                        if (success) {
                            updatePreviewCircle(circle.center, circle.radius);
                            s_defaultRadius = circle.radius;
                            m_state = kCompleted;
                        }
                        else {
                            failWithInvalid();
                        }
                    }
                }
            }
            break;
        }

        // ============================================================================
        // 两点模式 (2P)
        // ============================================================================
        case k2P_FirstPointEntry:
            m_state = k2P_FirstPointQuery;
            // 指定圆直径的第一个端点:
            ctx.waitForPoint(loc.get("command.circle.2p.firstPoint"));
            break;

        case k2P_FirstPointQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：等待中
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc：取消命令，结束（预览圆尚未创建）
            else if (status == InputStatus::kCanceled) {
                m_state = kCompleted;
            }
            // Enter/Space：结束命令
            else if (status == InputStatus::kEnterInput) {
                m_state = kCompleted;
            }
            // 点输入：获取第一点，进入第二点输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_firstPoint)) {
                    m_state = k2P_SecondPointEntry;
                }
            }
            break;
        }

        case k2P_SecondPointEntry:
            m_state = k2P_SecondPointQuery;
            // 指定圆直径的第二个端点:
            ctx.waitForPoint(loc.get("command.circle.2p.secondPoint"));
            break;

        case k2P_SecondPointQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：鼠标移动中，更新预览圆
            if (status == InputStatus::kNone) {
                glm::dvec3 currentPos = ctx.getPreviewPoint();
                glm::dvec3 center = (m_firstPoint + currentPos) * 0.5;
                double radius = glm::distance(m_firstPoint, currentPos) * 0.5;
                updatePreviewCircle(center, radius);
                break;
            }
            // Esc：取消命令，切换到创建失败状态（统一清理预览圆）
            else if (status == InputStatus::kCanceled) {
                m_state = kCreateFailed;
            }
            // Enter/Space：结束命令
            else if (status == InputStatus::kEnterInput) {
                m_state = kCreateFailed;
            }
            // 点输入：获取第二点，检查重合，计算圆心和半径，更新预览圆（预览圆就是最终圆）或提示错误
            else if (status == InputStatus::kPointInput) {
                glm::dvec3 secondPoint;
                if (ctx.getPickedPoint(secondPoint)) {
                    // 检查是否与第一点重合
                    if (Geometry::isCoincident(m_firstPoint, secondPoint)) {
                        failWithInvalid();
                    }
                    else {
                        // 直径端点计算圆心和半径
                        glm::dvec3 center = (m_firstPoint + secondPoint) * 0.5;
                        double radius = glm::distance(m_firstPoint, secondPoint) * 0.5;
                        updatePreviewCircle(center, radius);
                        s_defaultRadius = radius;
                        m_state = kCompleted;
                    }
                }
            }
            break;
        }

        // ============================================================================
        // 切点切点半径模式 (T) - 仅实现壳
        // ============================================================================
        case kT_FirstTangentEntry:
            m_state = kT_FirstTangentQuery;
            // 指定对象与圆的第一个切点:
            ctx.waitForPoint(loc.get("command.circle.t.firstTangent"));
            break;

        case kT_FirstTangentQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：等待中
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc：取消命令，结束（预览圆尚未创建）
            else if (status == InputStatus::kCanceled) {
                m_state = kCompleted;
            }
            // Enter/Space：结束命令
            else if (status == InputStatus::kEnterInput) {
                m_state = kCompleted;
            }
            // 点输入：获取第一切点，进入第二切点输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_firstPoint)) {
                    // TODO: 后续实现真正的切点捕捉
                    m_state = kT_SecondTangentEntry;
                }
            }
            break;
        }

        case kT_SecondTangentEntry:
            m_state = kT_SecondTangentQuery;
            // 指定对象与圆的第二个切点:
            ctx.waitForPoint(loc.get("command.circle.t.secondTangent"));
            break;

        case kT_SecondTangentQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：等待中
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc：取消命令，结束（预览圆尚未创建）
            else if (status == InputStatus::kCanceled) {
                m_state = kCompleted;
            }
            // Enter/Space：结束命令
            else if (status == InputStatus::kEnterInput) {
                m_state = kCompleted;
            }
            // 点输入：获取第二切点，进入半径输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_secondPoint)) {
                    // TODO: 后续实现真正的切点捕捉
                    m_state = kT_RadiusEntry;
                }
            }
            break;
        }

        case kT_RadiusEntry:
            m_state = kT_RadiusQuery;
            // 指定圆的半径 <默认值>:
            ctx.waitForNumber(StringUtils::format(loc.get("command.circle.t.radius"), s_defaultRadius), 0, DBL_MAX);
            // T模式半径是数值输入，不创建预览圆，直接根据数据创建
            break;

        case kT_RadiusQuery: {
            InputStatus status = ctx.getCurrentStatus();

            // 无输入：等待中
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc：取消命令，结束（预览圆尚未创建）
            else if (status == InputStatus::kCanceled) {
                m_state = kCompleted;
            }
            // Enter/Space：使用默认半径创建圆，结束
            else if (status == InputStatus::kEnterInput) {
                // TODO: 根据两切点和默认半径计算圆心（可能有两个解）
                glm::dvec3 center = (m_firstPoint + m_secondPoint) * 0.5;
                updatePreviewCircle(center, s_defaultRadius);
                m_state = kCompleted;
            }
            // 数值输入：获取半径，创建圆，更新默认值，结束
            else if (status == InputStatus::kFloatInput) {
                double radius;
                if (ctx.getNumber(radius)) {
                    if (radius <= 0) {
                        auto& loc = LocalizationManager::getInstance();
                        Utils::cmdLinePrint(loc.get("command.circle.invalid")); // 圆不存在。
                        m_state = kCompleted;
                    }
                    else {
                        // TODO: 根据两切点和半径计算圆心（可能有两个解）
                        glm::dvec3 center = (m_firstPoint + m_secondPoint) * 0.5;
                        updatePreviewCircle(center, radius);
                        s_defaultRadius = radius;
                        m_state = kCompleted;
                    }
                }
            }
            break;
        }

        // ============================================================================
        // 创建失败状态：统一清理预览圆，然后结束
        // ============================================================================
        case kCreateFailed:
            removePreviewCircle();
            m_state = kCompleted;
            break;

        // ============================================================================
        // 结束状态
        // ============================================================================
        case kCompleted:
            finish();
            break;
    }
}

} // namespace tch
