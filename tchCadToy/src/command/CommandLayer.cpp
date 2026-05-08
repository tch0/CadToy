// 对应头文件
#include "CommandLayer.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "LayerWindow.h"

namespace tch {

// 显式实例化模板
template class CommandLayer<true>;
template class CommandLayer<false>;

// 模板成员函数实现
template<bool Visible>
void CommandLayer<Visible>::onUpdate() {
    if (Visible) {
        LayerWindow::getInstance().show();
    } else {
        LayerWindow::getInstance().hide();
    }
    finish();
}

} // namespace tch
