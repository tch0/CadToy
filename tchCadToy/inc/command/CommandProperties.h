#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"

namespace tch {

// 属性栏命令模板类
// Visible: true - 显示属性栏, false - 隐藏属性栏
template<bool Visible>
class CommandProperties : public Command {
public:
    CommandProperties();
    
    // 命令更新方法
    void onUpdate() override;
};

// 类型别名
using CommandPropertiesShow = CommandProperties<true>;
using CommandPropertiesClose = CommandProperties<false>;

} // namespace tch
