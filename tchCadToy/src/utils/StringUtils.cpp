#include "utils/StringUtils.h"

namespace tch {

// 将字符串转换为大写
std::string StringUtils::toUpperCase(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        c = std::toupper(static_cast<unsigned char>(c));
    }
    return result;
}

// 将字符串转换为小写
std::string StringUtils::toLowerCase(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        c = std::tolower(static_cast<unsigned char>(c));
    }
    return result;
}

// 不区分大小写的字符串比较
bool StringUtils::equalsIgnoreCase(const std::string& str1, const std::string& str2) {
    if (str1.length() != str2.length()) {
        return false;
    }
    
    for (size_t i = 0; i < str1.length(); ++i) {
        if (std::toupper(static_cast<unsigned char>(str1[i])) != std::toupper(static_cast<unsigned char>(str2[i]))) {
            return false;
        }
    }
    
    return true;
}

// 不区分大小写的字符串比较（返回0表示相等，-1表示str1小于str2，1表示str1大于str2）
int StringUtils::compareIgnoreCase(const std::string& str1, const std::string& str2) {
    size_t minLength = std::min(str1.length(), str2.length());
    
    for (size_t i = 0; i < minLength; ++i) {
        char c1 = std::toupper(static_cast<unsigned char>(str1[i]));
        char c2 = std::toupper(static_cast<unsigned char>(str2[i]));
        
        if (c1 < c2) {
            return -1;
        }
        else if (c1 > c2) {
            return 1;
        }
    }
    
    if (str1.length() < str2.length()) {
        return -1;
    }
    else if (str1.length() > str2.length()) {
        return 1;
    }
    
    return 0;
}

} // namespace tch
