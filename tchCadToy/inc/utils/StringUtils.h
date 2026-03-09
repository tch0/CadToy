#pragma once

#include <string>

#include <format>
#include "debug/Logger.h"

namespace tch {

// 字符串处理工具类
class StringUtils {
public:
    // 将字符串转换为大写
    static std::string toUpperCase(const std::string& str);
    
    // 将字符串转换为小写
    static std::string toLowerCase(const std::string& str);
    
    // 不区分大小写的字符串比较
    static bool equalsIgnoreCase(const std::string& str1, const std::string& str2);
    
    // 不区分大小写的字符串比较（返回0表示相等，-1表示str1小于str2，1表示str1大于str2）
    static int compareIgnoreCase(const std::string& str1, const std::string& str2);
    
    // 格式化字符串，处理可能的异常
    template <typename... Args>
    static std::string format(const std::string& format_str, Args&&... args) {
        try {
            return std::vformat(format_str, std::make_format_args(args...));
        } catch (const std::format_error& e) {
            // 记录错误日志
            LOG_ERROR("Format error: {}", e.what());
            // 返回原始格式字符串
            return format_str;
        } catch (...) {
            // 记录错误日志
            LOG_ERROR("Unknown error during formatting");
            // 返回原始格式字符串
            return format_str;
        }
    }
};

} // namespace tch
