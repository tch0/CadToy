// 对应头文件
#include "PlatformUtils.h"

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {
namespace PlatformUtils {

// Unix/Linux 平台，假设系统使用 UTF-8

std::string localToUtf8(const std::string& localStr) {
    return localStr;
}

std::string utf8ToLocal(const std::string& utf8Str) {
    return utf8Str;
}

} // namespace PlatformUtils
} // namespace tch
