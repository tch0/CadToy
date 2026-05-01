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
#include "UndoManager.h"
#include "SelectionSet.h"

namespace tch {

// 测试程序列表
static const std::vector<std::pair<int, const char*>> s_programInfos = {
    {0, "实体选择测试 - 调用 waitForSelection 进行选择"},
    {1, "压力测试1 - 创建2500个正方形（边长2到5000），1万个实体，2万个顶点"},
    {2, "压力测试2 - 创建1000个圆（半径1到1000），1000个实体，12万8000个顶点"},
    {3, "压力测试3 - 清除数据库中所有实体"},
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

} // namespace tch
