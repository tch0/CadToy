// 对应头文件
#include "CommandTest.h"

// C++ 标准库
#include <format>
#include <string>
#include <utility>
#include <vector>

// 第三方库
#include <glm/glm.hpp>

// 项目头文件
#include "CommonTypes.h"
#include "InputContext.h"
#include "GlobalUtils.h"
#include "DocManager.h"
#include "Database.h"
#include "DbEntity.h"
#include "DbLine.h"
#include "DbCircle.h"
#include "DbXLine.h"
#include "DbRay.h"
#include "DbArc.h"
#include "DbEllipse.h"
#include "UndoManager.h"
#include "SelectionSet.h"
#include "Geometry.h"

namespace tch {

// 测试程序列表
static const std::vector<std::pair<int, const char*>> s_programInfos = {
    {0, "测试0 - 实体选择测试，调用 waitForSelection 进行选择"},
    {1, "测试1 - 性能测试，创建2500个正方形（边长2到5000），1万个实体，2万个顶点"},
    {2, "测试2 - 性能测试，创建1000个圆（半径1到1000），1000个实体，12万8000个顶点"},
    {3, "测试3 - 清除数据库中所有实体"},
    {4, "测试4 - 创建多样化测试实体（XLine/Ray/Circle/Arc/Ellipse）"},
};

CommandTest::CommandTest()
    : m_state(kWaitForTestNumberEntry) {
}

void CommandTest::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    InputContext& inputContext = InputContext::getInstance();
    
    switch (m_state) {
        case kWaitForTestNumberEntry:
            // 进入等待测试程序编号状态
            m_state = kWaitForTestNumberQuery;
            // 等待整数输入，允许关键字 "?"
            inputContext.waitForInteger("请输入测试程序编号(?输出所有测试程序编号与说明)<?>:", 0, 1000, {"?"});
            break;
            
        case kWaitForTestNumberQuery: {
            // 检查输入状态
            InputStatus status = inputContext.getCurrentStatus();
            
            // 整数输入
            if (status == InputStatus::kIntegerInput) {
                int testNumber;
                inputContext.getInteger(testNumber);
                // 执行测试程序
                m_state = executeTestProgram(testNumber);
            }
            // 关键字输入
            else if (status == InputStatus::kKeywordInput) {
                std::string keyword;
                inputContext.getKeyword(keyword);
                if (keyword == "?") {
                    // 显示帮助信息
                    m_state = kDisplayHelp;
                }
            }
            // Enter/Space 输入，默认执行显示信息
            else if (status == InputStatus::kEnterInput) {
                // 默认执行显示信息
                m_state = kDisplayHelp;
            }
            // Esc 取消
            else if (status == InputStatus::kCanceled) {
                m_state = kCompleted;
            }
            break;
        }
        
        case kTest0: {
            // 执行测试程序 0：实体选择
            m_state = runTest0();
            break;
        }
        
        case kTest1: {
            // 执行测试程序 1：创建2500个正方形
            m_state = runTest1();
            break;
        }

        case kTest2: {
            // 执行测试程序 2：创建1000个圆
            m_state = runTest2();
            break;
        }

        case kTest3: {
            // 执行测试程序 3：清除所有实体
            m_state = runTest3();
            break;
        }

        case kTest4: {
            // 执行测试程序 4：创建多样化测试实体
            m_state = runTest4();
            break;
        }

        case kDisplayHelp:
            // 打印所有测试程序用途
            Utils::cmdLinePrint("测试程序编号与说明:");
            for (const auto& [number, description] : s_programInfos) {
                Utils::cmdLinePrint(std::format("{:>4}: {}", number, description));
            }
            Utils::cmdLinePrint("");
            // 回到初始状态
            m_state = kWaitForTestNumberEntry;
            break;
            
        case kCompleted:
            finish();
            break;
            
        default:
            break;
    }
}

CommandTest::TestState CommandTest::executeTestProgram(int testNumber) {
    InputContext& inputContext = InputContext::getInstance();
    
    switch (testNumber) {
        case 0:
            // 测试程序 0：实体选择
            inputContext.waitForSelection("选择对象:");
            return kTest0;
        case 1:
            // 测试程序 1：创建2500个正方形
            return kTest1;
        case 2:
            // 测试程序 2：创建1000个圆
            return kTest2;
        case 3:
            // 测试程序 3：清除所有实体
            return kTest3;
        case 4:
            // 测试程序 4：创建多样化测试实体
            return kTest4;
        default:
            // 无效的测试程序编号
            Utils::cmdLinePrint("无效的测试程序编号");
            return kWaitForTestNumberEntry;
    }
}

CommandTest::TestState CommandTest::runTest0() {
    InputContext& inputContext = InputContext::getInstance();
    InputStatus status = inputContext.getCurrentStatus();

    // 实体选择完成
    if (status == InputStatus::kEntitySelection) {
        Utils::cmdLinePrint("实体选择完成，继续选择");
        // kEntitySelection状态时必选调用getSelectionSet获取选择集并重置状态
        SelectionSet selectionSet;
        inputContext.getSelectionSet(selectionSet);
        Utils::cmdLinePrint(std::format("选择了 {} 个实体", selectionSet.size()));

        // 输出所有选中实体的ID和类型
        Database* db = DocManager::getCurrentDocument().getDatabase();
        if (db) {
            for (ObjectId id : selectionSet) {
                DbEntity* entity = db->getEntity(id);
                if (entity) {
                    std::string typeName = entity->typeName();
                    Utils::cmdLinePrint(std::format("  ID: {}, 类型: {}", id, typeName));
                } else {
                    Utils::cmdLinePrint(std::format("  ID: {}, 类型: <无效实体>", id));
                }
            }
        }

        inputContext.waitForSelection("选择对象:");
        return kTest0;
    }
    // Enter/Space 结束选择
    else if (status == InputStatus::kEnterInput) {
        Utils::cmdLinePrint("选择结束");
        return kCompleted;
    }
    // Esc 取消选择
    else if (status == InputStatus::kCanceled) {
        Utils::cmdLinePrint("选择取消");
        return kCompleted;
    }
    
    return kTest0;
}

CommandTest::TestState CommandTest::runTest1() {
    // 测试程序1：创建2500个正方形，边长从2、4、6到5000，原点为中心
    auto* db = DocManager::getCurrentDocument().getDatabase();
    if (!db) {
        Utils::cmdLinePrint("数据库不可用");
        return kCompleted;
    }

    int count = 0;
    for (int sideLength = 2; sideLength <= 5000; sideLength += 2) {
        double half = sideLength * 0.5;
        glm::dvec3 p1(-half, -half, 0);
        glm::dvec3 p2(half, -half, 0);
        glm::dvec3 p3(half, half, 0);
        glm::dvec3 p4(-half, half, 0);

        // 创建正方形的四条边
        auto line1 = std::make_unique<DbLine>(p1, p2);
        auto line2 = std::make_unique<DbLine>(p2, p3);
        auto line3 = std::make_unique<DbLine>(p3, p4);
        auto line4 = std::make_unique<DbLine>(p4, p1);

        line1->setPropertiesFromDb();
        line2->setPropertiesFromDb();
        line3->setPropertiesFromDb();
        line4->setPropertiesFromDb();

        ObjectId id1 = db->addObject(std::move(line1));
        ObjectId id2 = db->addObject(std::move(line2));
        ObjectId id3 = db->addObject(std::move(line3));
        ObjectId id4 = db->addObject(std::move(line4));

        UndoManager::getInstance().recordAdd(id1);
        UndoManager::getInstance().recordAdd(id2);
        UndoManager::getInstance().recordAdd(id3);
        UndoManager::getInstance().recordAdd(id4);

        count++;
    }

    Utils::cmdLinePrint(std::format("测试程序1完成：创建了{}个正方形（共{}条线段）", count, count * 4));
    return kCompleted;
}

CommandTest::TestState CommandTest::runTest2() {
    // 测试程序2：创建1000个圆，半径从1到1000，原点为中心
    auto* db = DocManager::getCurrentDocument().getDatabase();
    if (!db) {
        Utils::cmdLinePrint("数据库不可用");
        return kCompleted;
    }

    glm::dvec3 center(0, 0, 0);
    int count = 0;
    for (int radius = 1; radius <= 1000; ++radius) {
        auto circle = std::make_unique<DbCircle>(center, radius * 1.0);
        circle->setPropertiesFromDb();

        ObjectId id = db->addObject(std::move(circle));
        UndoManager::getInstance().recordAdd(id);

        count++;
    }

    Utils::cmdLinePrint(std::format("测试程序2完成：创建了{}个圆", count));
    return kCompleted;
}

CommandTest::TestState CommandTest::runTest3() {
    // 测试程序3：清除数据库中所有实体
    auto* db = DocManager::getCurrentDocument().getDatabase();
    if (!db) {
        Utils::cmdLinePrint("数据库不可用");
        return kCompleted;
    }

    // 收集所有实体ID
    std::vector<ObjectId> allIds;
    db->forEachEntity([&allIds](DbEntity* entity) {
        if (entity) {
            allIds.push_back(entity->id());
        }
    });

    int count = 0;
    for (ObjectId id : allIds) {
        db->removeObject(id);
        UndoManager::getInstance().recordRemove(id);
        count++;
    }

    Utils::cmdLinePrint(std::format("测试程序3完成：清除了{}个实体", count));
    return kCompleted;
}

CommandTest::TestState CommandTest::runTest4() {
    // 测试程序4：创建多样化测试实体
    auto* pDb = DocManager::getCurrentDocument().getDatabase();
    if (!pDb) {
        Utils::cmdLinePrint("数据库不可用");
        return kCompleted;
    }

    std::vector<ObjectId> allIds;
    
    // ========== XLine (构造线) 参数定义 ==========
    struct XLineParam {
        glm::dvec3 origin;
        glm::dvec3 direction;
    };
    const XLineParam xlineParams[] = {
        {{0, 50, 0}, {1, 0, 0}},      // 第一象限 - 水平向右
        {{50, 0, 0}, {0, 1, 0}},      // 第一象限 - 垂直向上
        {{0, 0, 0}, {1, 1, 0}},       // 第一象限 - 45度斜向
        {{0, 200, 0}, {1, 0, 0}},     // 外围区域 - 水平
        {{200, 0, 0}, {0, 1, 0}},     // 外围区域 - 垂直
    };
    
    // ========== Ray (射线) 参数定义 ==========
    struct RayParam {
        glm::dvec3 origin;
        double angleDeg;  // 角度（度）
    };
    const RayParam rayParams[] = {
        {{20, 80, 0}, 0.0},           // 第一象限 - 水平向右
        {{80, 20, 0}, 90.0},          // 第一象限 - 垂直向上
        {{30, 30, 0}, -30.0},         // 第一象限 - -30度斜向
        {{250, 50, 0}, 120.0},        // 外围区域 - 120度方向
        {{50, 250, 0}, 60.0},         // 外围区域 - 60度方向
    };
    
    // ========== Circle (圆) 参数定义 ==========
    struct CircleParam {
        glm::dvec3 center;
        double radius;
    };
    const CircleParam circleParams[] = {
        {{-50, 50, 0}, 30.0},         // 第二象限
        {{-100, 100, 0}, 50.0},       // 第二象限
        {{-30, 120, 0}, 20.0},        // 第二象限
        {{200, 200, 0}, 40.0},        // 外围区域
        {{-200, -200, 0}, 60.0},      // 外围区域
        {{200, -200, 0}, 35.0},       // 外围区域
        {{-200, 200, 0}, 45.0},       // 外围区域
    };
    
    // ========== Arc (圆弧) 参数定义 ==========
    struct ArcParam {
        glm::dvec3 center;
        double radius;
        double startAngle;  // 弧度
        double endAngle;    // 弧度
    };
    const ArcParam arcParams[] = {
        {{-50, -50, 0}, 40.0, 0.0, Geometry::HALF_PI},                    // 第三象限 - 0~90度
        {{-100, -80, 0}, 35.0, Geometry::PI, 3.0 * Geometry::HALF_PI},    // 第三象限 - 180~270度
        {{-30, -120, 0}, 25.0, -Geometry::PI / 4.0, Geometry::PI / 4.0},  // 第三象限 - -45~45度(跨0度)
        {{-120, -30, 0}, 45.0, 3.0 * Geometry::PI / 4.0, 5.0 * Geometry::PI / 4.0}, // 第三象限 - 135~225度(跨180度)
        {{200, -200, 0}, 50.0, Geometry::PI / 4.0, 3.0 * Geometry::PI / 4.0},       // 外围区域
        {{-200, 200, 0}, 40.0, 0.0, Geometry::PI},                        // 外围区域
    };
    
    // ========== Ellipse (完整椭圆) 参数定义 ==========
    struct EllipseParam {
        glm::dvec3 center;
        double radiusX;
        double radiusY;
        double rotation;  // 弧度
    };
    const EllipseParam ellipseParams[] = {
        {{80, -80, 0}, 50.0, 30.0, 0.0},              // 第四象限 - 长轴水平
        {{120, -120, 0}, 40.0, 20.0, Geometry::HALF_PI}, // 第四象限 - 长轴垂直
        {{50, -150, 0}, 35.0, 17.5, Geometry::PI / 4.0}, // 第四象限 - 长轴倾斜45度
        {{250, 250, 0}, 45.0, 22.5, 0.0},             // 外围区域
        {{-250, -250, 0}, 40.0, 20.0, Geometry::HALF_PI}, // 外围区域
    };
    
    // ========== EllipseArc (椭圆弧) 参数定义 ==========
    struct EllipseArcParam {
        glm::dvec3 center;
        double radiusX;
        double radiusY;
        double rotation;   // 弧度
        double startParam; // 弧度
        double endParam;   // 弧度
    };
    const EllipseArcParam ellipseArcParams[] = {
        {{0, 0, 0}, 30.0, 15.0, 0.0, 0.0, Geometry::PI},                          // 中心区域 - 长轴水平 0~180度
        {{0, 0, 0}, 25.0, 12.5, Geometry::HALF_PI, Geometry::HALF_PI, 3.0 * Geometry::HALF_PI}, // 中心区域 - 长轴垂直 90~270度
        {{0, 0, 0}, 20.0, 10.0, Geometry::PI / 6.0, -Geometry::PI / 3.0, Geometry::PI / 3.0},  // 中心区域 - 长轴倾斜30度 -60~60度
    };
    
    // ========== 创建 XLine ==========
    for (const auto& param : xlineParams) {
        auto xline = std::make_unique<DbXLine>(param.origin, param.direction);
        xline->setPropertiesFromDb();
        allIds.push_back(pDb->addObject(std::move(xline)));
    }
    
    // ========== 创建 Ray ==========
    for (const auto& param : rayParams) {
        double angle = param.angleDeg * Geometry::PI / 180.0;
        glm::dvec3 dir(cos(angle), sin(angle), 0);
        auto ray = std::make_unique<DbRay>(param.origin, dir);
        ray->setPropertiesFromDb();
        allIds.push_back(pDb->addObject(std::move(ray)));
    }
    
    // ========== 创建 Circle ==========
    for (const auto& param : circleParams) {
        auto circle = std::make_unique<DbCircle>(param.center, param.radius);
        circle->setPropertiesFromDb();
        allIds.push_back(pDb->addObject(std::move(circle)));
    }
    
    // ========== 创建 Arc ==========
    for (const auto& param : arcParams) {
        auto arc = std::make_unique<DbArc>(param.center, param.radius, param.startAngle, param.endAngle);
        arc->setPropertiesFromDb();
        allIds.push_back(pDb->addObject(std::move(arc)));
    }
    
    // ========== 创建 Ellipse (完整椭圆) ==========
    for (const auto& param : ellipseParams) {
        auto ellipse = std::make_unique<DbEllipse>(param.center, param.radiusX, param.radiusY, param.rotation);
        ellipse->setPropertiesFromDb();
        allIds.push_back(pDb->addObject(std::move(ellipse)));
    }
    
    // ========== 创建 EllipseArc (椭圆弧) ==========
    for (const auto& param : ellipseArcParams) {
        auto ellipseArc = std::make_unique<DbEllipse>(param.center, param.radiusX, param.radiusY, 
                                                       param.rotation, param.startParam, param.endParam);
        ellipseArc->setPropertiesFromDb();
        allIds.push_back(pDb->addObject(std::move(ellipseArc)));
    }
    
    // 统一记录undo
    for (ObjectId id : allIds) {
        UndoManager::getInstance().recordAdd(id);
    }
    
    size_t count = allIds.size();
    Utils::cmdLinePrint(std::format("测试程序4完成：创建了{}个多样化测试实体", count));
    Utils::cmdLinePrint(std::format("  - XLine: {}个 (水平、垂直、斜向)", std::size(xlineParams)));
    Utils::cmdLinePrint(std::format("  - Ray: {}个 (不同方向)", std::size(rayParams)));
    Utils::cmdLinePrint(std::format("  - Circle: {}个 (不同位置)", std::size(circleParams)));
    Utils::cmdLinePrint(std::format("  - Arc: {}个 (不同角度范围，包含跨0度和跨180度)", std::size(arcParams)));
    Utils::cmdLinePrint(std::format("  - Ellipse: {}个 (长轴水平/垂直/倾斜)", std::size(ellipseParams)));
    Utils::cmdLinePrint(std::format("  - EllipseArc: {}个 (不同参数范围)", std::size(ellipseArcParams)));
    return kCompleted;
}

} // namespace tch
