#pragma once

// C++ 标准库
#include <unordered_map>

// 第三方库
#include <glm/glm.hpp>

// 项目头文件
#include "SelectionSet.h"
#include "Geometry.h"

namespace tch {

// 前置声明
class Database;
class IGraphicsDataCache;

// ================================================================================================
// 选择管理器类
// 负责接收选择任务输出的选择模式与一系列选择点，去实时通知预选高亮，提供预选更新接口给选择任务调用
// 注意：不保存数据库和图形缓存指针，每次使用时从当前文档动态获取
// 作为全局单例使用
// ================================================================================================
class SelectionManager {
public:
    // 获取单例实例
    static SelectionManager& getInstance();

    // 禁止拷贝
    SelectionManager(const SelectionManager&) = delete;
    SelectionManager& operator=(const SelectionManager&) = delete;

    // ================================================================================================
    // 预选交互（每帧由 SelectionTask 调用，仅更新临时高亮）
    // ================================================================================================
    
    // 拾取框预选（点选预览，内部自动获取拾取框大小）
    void preSelectPick(const glm::dvec3& worldPos);
    
    // 窗选预选（实体必须完全在矩形内部）
    void preSelectWindow(const Geometry::AABB& rect);
    
    // 交叉窗选预选（实体与矩形相交即预选）
    void preSelectCrossing(const Geometry::AABB& rect);
    
    // 清除所有预选状态（选择任务结束时调用）
    void clearPreSelect();

    // ================================================================================================
    // 确认选择（生成干净的实体 ID 集合，不产生任何视觉变化）
    // ================================================================================================
    
    // 确认点选
    SelectionSet commitPick(const glm::dvec3& worldPos) const;
    
    // 确认窗选
    SelectionSet commitWindow(const Geometry::AABB& rect) const;
    
    // 确认交叉窗选
    SelectionSet commitCrossing(const Geometry::AABB& rect) const;
    
    // ================================================================================================
    // 查询接口
    // ================================================================================================
    
    // 查询当前是否为锁定单实体预览（用于光标显示锁定标记）
    bool isPreSelectEntityOnLockedLayer() const { return m_isPreSelectEntOnLockedLayer; }
    
private:
    // 私有构造函数
    SelectionManager();
    ~SelectionManager() = default;

    // ================================================================================================
    // 内部方法
    // ================================================================================================
    
    // 获取当前文档的数据库
    Database* getDatabase() const;
    
    // 获取当前文档的图形缓存
    IGraphicsDataCache* getGraphicsDataCache() const;
    
    // 预选状态帧间维护，鼠标位置发生变化才更新
    void updatePreSelectState(const SelectionSet& newIds);
    
    // 根据屏幕拾取框大小创建世界坐标包围盒
    Geometry::AABB createPickBoxAABB(const glm::dvec3& worldPos) const;
    
    // 查询实体是否在锁定图层上（带缓存）
    bool isEntityLocked(ObjectId id) const;
    
    // 点选：从候选集中按策略选出一个实体
    ObjectId pickOneFromRawSet(const SelectionSet& rawSet);

    // ================================================================================================
    // 成员变量
    // ================================================================================================

    // ----------所有模式的预选实体集合-------------------------
    SelectionSet m_prevPreSelectSet;                        // 上一帧的预选实体集合（包含锁定图层实体）
    SelectionSet m_currPreSelectSet;                        // 当前帧的预选实体集合（包含锁定图层实体）
    
    // ---------- 点选决策相关 -------------------------------
    SelectionSet m_prevRawPickSet;                          // 上一帧点选拾取框内全部候选实体（包含锁定图层实体）
    SelectionSet m_currRawPickSet;                          // 当前帧点选拾取框内全部候选实体（包含锁定图层实体）
    ObjectId m_currPickPreSelectId = 0;                     // 当前帧点选的预选实体（可能是锁定图层实体），0表示无
    ObjectId m_prevPickPreSelectId = 0;                     // 上一帧点选的预选实体（可能是锁定图层实体），0表示无
    
    // ---------- 实际预选高亮实体集合（过滤锁定图层实体后）-------
    SelectionSet m_prevHighlightSet;                        // 上一帧实际高亮的实体（预选集合排除锁定图层实体的结果）
    SelectionSet m_currHighlightSet;                        // 当前帧实际高亮的实体（预选集合排除锁定图层实体的结果）
    
    // ---------- 图层锁定缓存与锁定光标标记 -------------------
    mutable std::unordered_map<ObjectId, bool> m_lockCache; // 实体锁定状态缓存
    bool m_isPreSelectEntOnLockedLayer = false;             // 预选实体是否唯一且在锁定图层上，指示锁定光标标记的显示
};

} // namespace tch
