#pragma once

#include "command/Command.h"

namespace tch {

// 测试命令类
class CommandTest : public Command {
public:
    CommandTest();
    
    void onUpdate() override;

private:
    // Test命令状态
    enum class TestState {
        kWaitForTestNumberEntry,    // 等待测试程序编号-入口
        kWaitForTestNumberQuery,    // 等待测试程序编号-输入查询
        kTest0,                     // 测试程序 0：实体选择
        kDisplayHelp,               // 显示帮助信息
        kCompleted                  // 完成
    };
    
    TestState m_state;     // 当前状态
    
    // 按照编号执行对应测试程序
    TestState executeTestProgram(int testNumber);
    // 运行测试程序 0
    TestState runTest0();
};

} // namespace tch
