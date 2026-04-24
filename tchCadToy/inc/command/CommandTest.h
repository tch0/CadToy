#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"

namespace tch {

// 测试命令类
class CommandTest : public Command {
public:
    CommandTest();
    
    void onUpdate() override;

private:
    // Test命令状态
    enum TestState {
        kWaitForTestNumberEntry,    // 等待测试程序编号-入口
        kWaitForTestNumberQuery,    // 等待测试程序编号-输入查询
        kTest0,                     // 测试程序 0：实体选择
        kTest1,                     // 测试程序 1：创建500个正方形
        kTest2,                     // 测试程序 2：创建10000个圆
        kTest3,                     // 测试程序 3：清除所有实体
        kDisplayHelp,               // 显示帮助信息
        kCompleted                  // 完成
    };
    
    TestState m_state;     // 当前状态
    
    // 按照编号执行对应测试程序
    TestState executeTestProgram(int testNumber);
    // 运行测试程序 0
    TestState runTest0();
    // 运行测试程序 1：创建500个正方形
    TestState runTest1();
    // 运行测试程序 2：创建10000个圆
    TestState runTest2();
    // 运行测试程序 3：清除所有实体
    TestState runTest3();
};

} // namespace tch
