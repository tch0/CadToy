// 对应头文件
#include "SelectionSet.h"

// C++ 标准库

// 第三方库

// 项目头文件

namespace tch {

// ================================================================================================
// 基本集合操作
// ================================================================================================

void SelectionSet::add(ObjectId id) {
    if (id != 0) {
        m_ids.insert(id);
    }
}

void SelectionSet::remove(ObjectId id) {
    m_ids.erase(id);
}

bool SelectionSet::contains(ObjectId id) const {
    return m_ids.find(id) != m_ids.end();
}

void SelectionSet::clear() {
    m_ids.clear();
}

size_t SelectionSet::size() const {
    return m_ids.size();
}

bool SelectionSet::empty() const {
    return m_ids.empty();
}

// ================================================================================================
// 集合运算（就地修改）
// ================================================================================================ 

void SelectionSet::unite(const SelectionSet& other) {
    for (ObjectId id : other.m_ids) {
        m_ids.insert(id);
    }
}

void SelectionSet::intersect(const SelectionSet& other) {
    // 双指针算法，利用两个集合都是有序的特性
    // 复杂度: O(n + m)，其中 n = this->size(), m = other.size()
    // 注意: std::set::erase 返回下一个有效迭代器(C++11起)

    auto it = m_ids.begin();
    auto otherIt = other.m_ids.begin();

    while (it != m_ids.end() && otherIt != other.m_ids.end()) {
        if (*it < *otherIt) {
            // 当前元素只在 this 中，不在 other 中，移除
            it = m_ids.erase(it);
        } else if (*it > *otherIt) {
            // 当前元素在 other 中但不在 this 中，other 前进
            ++otherIt;
        } else {
            // 元素在两个集合中都存在，保留，双方前进
            ++it;
            ++otherIt;
        }
    }

    // 移除 this 中剩余的元素（这些元素都大于 other 中所有元素）
    if (it != m_ids.end()) {
        m_ids.erase(it, m_ids.end());
    }
}

void SelectionSet::subtract(const SelectionSet& other) {
    for (ObjectId id : other.m_ids) {
        m_ids.erase(id);
    }
}

} // namespace tch
