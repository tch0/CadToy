// 对应头文件
#include "PlatformUtils.h"

// Windows 头文件
#include <windows.h>
#include <vector>

namespace tch {
namespace PlatformUtils {

std::string localToUtf8(const std::string& localStr) {
    if (localStr.empty()) {
        return localStr;
    }

    // 本地编码 -> Unicode (UTF-16)
    int wideLen = MultiByteToWideChar(CP_ACP, 0, localStr.c_str(), -1, nullptr, 0);
    if (wideLen <= 0) {
        return localStr;
    }

    std::vector<wchar_t> wideStr(wideLen);
    MultiByteToWideChar(CP_ACP, 0, localStr.c_str(), -1, wideStr.data(), wideLen);

    // Unicode (UTF-16) -> UTF-8
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.data(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Len <= 0) {
        return localStr;
    }

    std::vector<char> utf8Str(utf8Len);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.data(), -1, utf8Str.data(), utf8Len, nullptr, nullptr);

    return std::string(utf8Str.data());
}

std::string utf8ToLocal(const std::string& utf8Str) {
    if (utf8Str.empty()) {
        return utf8Str;
    }

    // UTF-8 -> Unicode (UTF-16)
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
    if (wideLen <= 0) {
        return utf8Str;
    }

    std::vector<wchar_t> wideStr(wideLen);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wideStr.data(), wideLen);

    // Unicode (UTF-16) -> 本地编码
    int localLen = WideCharToMultiByte(CP_ACP, 0, wideStr.data(), -1, nullptr, 0, nullptr, nullptr);
    if (localLen <= 0) {
        return utf8Str;
    }

    std::vector<char> localStr(localLen);
    WideCharToMultiByte(CP_ACP, 0, wideStr.data(), -1, localStr.data(), localLen, nullptr, nullptr);

    return std::string(localStr.data());
}

} // namespace PlatformUtils
} // namespace tch
