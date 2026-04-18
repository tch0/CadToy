// 对应头文件
#include "CommandProperties.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "Renderer.h"

namespace tch {

// 显式实例化模板
template class CommandProperties<true>;
template class CommandProperties<false>;

template<bool Visible>
CommandProperties<Visible>::CommandProperties() {
}

template<bool Visible>
void CommandProperties<Visible>::onUpdate() {
    if (isCompleted()) {
        return;
    }
    
    // 设置属性栏可见性
    Renderer::setPropertyBarVisible(Visible);
    
    // 命令完成
    finish();
}

} // namespace tch
