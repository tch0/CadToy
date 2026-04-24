// 对应头文件
#include "CommandTest.h"

// C++ 标准库
#include <format>
#include <string>
#include <utility>
#include <vector>

// 第三方库

// 项目头文件
#include "CommonTypes.h"
#include "InputContext.h"
#include "GlobalUtils.h"

namespace tch {

// 测试程序列表
static const std::vector<std::pair<int, const char*>> s_programInfos = {
    {0, "实体选择测试 - 调用 waitForEntity 进行选择"},
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
            inputContext.waitForEntity("选择对象:");
            return kTest0;
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
        // kEntitySelection状态时必选调用getSelectedEntities获取选择集并重置状态
        std::vector<void*> entities;
        inputContext.getSelectedEntities(entities);
        inputContext.waitForEntity("选择对象:");
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

} // namespace tch
