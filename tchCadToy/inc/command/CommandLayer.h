#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "Command.h"

namespace tch {

// ================================================================================================
// 图层窗口命令模板类
// Visible: true - 显示图层窗口, false - 隐藏图层窗口
// ================================================================================================
template<bool Visible>
class CommandLayer : public Command {
public:
    CommandLayer() = default;

    // 命令更新方法
    void onUpdate() override;
};

// 类型别名
using CommandLayerShow = CommandLayer<true>;
using CommandLayerClose = CommandLayer<false>;

} // namespace tch
