#pragma once

// C++ 标准库
#include <string>

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {
namespace PlatformUtils {

// 原则上来说项目中全部使用UTF-8编码，路径也全部使用filesystem其中自动处理了编码，不会调用到这两个函数才对
// 但还是将这两个函数留在这里备用
 
/**
 * @brief 将本地编码字符串转换为 UTF-8
 * @param localStr 本地编码字符串（Windows: GBK/GB2312, Unix: UTF-8）
 * @return UTF-8 编码字符串
 */
std::string localToUtf8(const std::string& localStr);

/**
 * @brief 将 UTF-8 字符串转换为本地编码
 * @param utf8Str UTF-8 编码字符串
 * @return 本地编码字符串（Windows: GBK/GB2312, Unix: UTF-8）
 */
std::string utf8ToLocal(const std::string& utf8Str);


} // namespace PlatformUtils
} // namespace tch
