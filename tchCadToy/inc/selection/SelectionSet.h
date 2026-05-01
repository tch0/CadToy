#pragma once

// C++ 标准库
#include <set>
#include <cstddef>

// 第三方库

// 项目头文件
#include "DbObject.h"

namespace tch {

// ================================================================================================
// 选择集类
// 纯粹容器类，不涉及数据库状态，提供排好序的实体ID列表，内部std::set实现
// ================================================================================================
class SelectionSet {
public:
    // ================================================================================================
    // 构造与赋值
    // ================================================================================================
    
    // 默认构造
    SelectionSet() = default;
    
    // 拷贝构造
    SelectionSet(const SelectionSet&) = default;
    
    // 拷贝赋值
    SelectionSet& operator=(const SelectionSet&) = default;
    
    // 移动构造
    SelectionSet(SelectionSet&&) = default;
    
    // 移动赋值
    SelectionSet& operator=(SelectionSet&&) = default;
    
    // 从迭代器范围构造
    template<typename InputIt>
    SelectionSet(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            add(*it);
        }
    }
    
    // 从初始化列表构造
    SelectionSet(std::initializer_list<ObjectId> init) {
        for (ObjectId id : init) {
            add(id);
        }
    }

    // ================================================================================================
    // 基本集合操作
    // ================================================================================================
    
    // 添加单个实体ID
    void add(ObjectId id);
    
    // 移除单个实体ID
    void remove(ObjectId id);
    
    // 检查是否包含实体ID
    bool contains(ObjectId id) const;
    
    // 清空选择集
    void clear();
    
    // 获取实体数量
    size_t size() const;
    
    // 是否为空
    bool empty() const;
    
    // ================================================================================================
    // 批量操作（迭代器版本）
    // ================================================================================================

    // 从迭代器范围添加
    template<typename InputIt>
    void add(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            add(*it);
        }
    }

    // 从迭代器范围移除
    template<typename InputIt>
    void remove(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            remove(*it);
        }
    }

    // ================================================================================================
    // 集合运算（就地修改）
    // ================================================================================================
    
    // 并集：this = this ∪ other
    void unite(const SelectionSet& other);
    
    // 交集：this = this ∩ other
    void intersect(const SelectionSet& other);
    
    // 差集：this = this \ other
    void subtract(const SelectionSet& other);

    // ================================================================================================
    // 运算符重载（就地修改）
    // ================================================================================================
    
    // 交集：this ^= other 相当于 this = this ∩ other
    SelectionSet& operator^=(const SelectionSet& other) {
        intersect(other);
        return *this;
    }
    
    // 并集：this += other 相当于 this = this ∪ other
    SelectionSet& operator+=(const SelectionSet& other) {
        unite(other);
        return *this;
    }
    
    // 差集：this -= other 相当于 this = this \ other
    SelectionSet& operator-=(const SelectionSet& other) {
        subtract(other);
        return *this;
    }

    // ================================================================================================
    // 运算符重载（新构造选择集）
    // ================================================================================================
    
    // 交集：result = a ^ b 相当于 result = a ∩ b
    SelectionSet operator^(const SelectionSet& other) const {
        SelectionSet result = *this;
        result ^= other;
        return result;
    }
    
    // 并集：result = a + b 相当于 result = a ∪ b
    SelectionSet operator+(const SelectionSet& other) const {
        SelectionSet result = *this;
        result += other;
        return result;
    }
    
    // 差集：result = a - b 相当于 result = a \ b
    SelectionSet operator-(const SelectionSet& other) const {
        SelectionSet result = *this;
        result -= other;
        return result;
    }

    // ================================================================================================
    // 比较运算符
    // ================================================================================================
    
    // 相等比较
    bool operator==(const SelectionSet& other) const {
        return m_ids == other.m_ids;
    }
    
    // 不等比较
    bool operator!=(const SelectionSet& other) const {
        return !(*this == other);
    }

    // ================================================================================================
    // 迭代器接口
    // ================================================================================================

    // 迭代器类型定义
    using iterator = std::set<ObjectId>::iterator;
    using const_iterator = std::set<ObjectId>::const_iterator;
    using reverse_iterator = std::set<ObjectId>::reverse_iterator;
    using const_reverse_iterator = std::set<ObjectId>::const_reverse_iterator;

    // 正向迭代器
    iterator begin() { return m_ids.begin(); }
    iterator end() { return m_ids.end(); }
    const_iterator begin() const { return m_ids.begin(); }
    const_iterator end() const { return m_ids.end(); }

    // const迭代器（C++11风格）
    const_iterator cbegin() const { return m_ids.cbegin(); }
    const_iterator cend() const { return m_ids.cend(); }

    // 反向迭代器
    reverse_iterator rbegin() { return m_ids.rbegin(); }
    reverse_iterator rend() { return m_ids.rend(); }
    const_reverse_iterator rbegin() const { return m_ids.rbegin(); }
    const_reverse_iterator rend() const { return m_ids.rend(); }
    const_reverse_iterator crbegin() const { return m_ids.crbegin(); }
    const_reverse_iterator crend() const { return m_ids.crend(); }

    // ================================================================================================
    // 访问器
    // ================================================================================================

    // 获取有序实体ID集合的只读引用（如果需要直接操作底层set）
    const std::set<ObjectId>& asSet() const { return m_ids; }

private:
    std::set<ObjectId> m_ids;  // 红黑树实现，天然排序且唯一
};

} // namespace tch
