// 对应头文件
#include "PropertyBarContent.h"

// C++ 标准库

// 第三方库
#include <imgui.h>

// 项目头文件

namespace tch {

PropertyBarContent::PropertyBarContent() {
}

PropertyBarContent& PropertyBarContent::getInstance() {
    static PropertyBarContent instance;
    return instance;
}

void PropertyBarContent::draw() {
    // TODO: 选择集实现后再来绘制属性
    ImGui::Text("TODO: Property Bar Content");
}

} // namespace tch
