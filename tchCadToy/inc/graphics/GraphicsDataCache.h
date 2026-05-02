#pragma once

// C++ 标准库
#include <unordered_map>
#include <unordered_set>
#include <vector>

// 第三方库

// 项目头文件
#include "IGraphicsDataCache.h"


namespace tch {

class Database;

// ================================================================================================
// 内部使用的实体缓存数据结构
// 包含基础数据、修饰后数据、临时状态（预选、选中、临时暗显）等信息
// 注意：activeStates 使用 DataCacheVertex 中定义的标记位
// ================================================================================================
struct CachedEntityData {
    EntityGraphicsCacheData baseData;        // 引擎生成的基础几何数据（含图层锁定暗显）
    EntityGraphicsCacheData modulatedData;   // 修饰后数据（临时修饰状态不为0则使用这个数据显示，需要时懒生成）
    uint32_t activeStates = 0;               // 当前激活的临时状态掩码（使用 DataCacheVertex::kFlag*）
    bool modulatedDirty = true;              // 修饰数据是否需要重建
};

// ================================================================================================
// 图形数据缓存实现类
// 职责：管理数据库中每个实体的CPU端缓存顶点数据，维护脏实体集合，统一管理临时状态
// ================================================================================================
class GraphicsDataCache : public IGraphicsDataCache {
public:
    GraphicsDataCache() = default;
    ~GraphicsDataCache() override = default;

    // 禁止拷贝
    GraphicsDataCache(const GraphicsDataCache&) = delete;
    GraphicsDataCache& operator=(const GraphicsDataCache&) = delete;

    // ========================================================================
    // 数据库关联
    // ========================================================================
    void setDatabase(Database* pDb) { m_pDatabase = pDb; }
    Database* getDatabase() const override { return m_pDatabase; }

    // ========================================================================
    // 查询与写入接口
    // ========================================================================
    std::vector<ObjectId> getDirtyEntities() const override;
    void setEntityCacheData(ObjectId id, EntityGraphicsCacheData&& cacheData) override;
    void removeEntityCacheData(ObjectId id) override;
    void clearDirty(ObjectId id) override;
    void markAllDirty() override { m_regenAll = true; }
    bool needsRegenAll() const override { return m_regenAll; }
    void clearAllDirty() override { m_regenAll = false; }
    void clearAllCacheData() override;
    bool isCacheDirty(ObjectId id) const override;

    // ========================================================================
    // 视口管理（无限实体会在 updateViewport 中被标记为脏）
    // ========================================================================
    void updateViewport(const Geometry::AABB& newViewport) override;
    const Geometry::AABB& getCurrentViewport() const override { return m_currentViewportAABB; }

    // ========================================================================
    // 通知接口（由数据库调用，响应实体变化）
    // ========================================================================
    void onEntityAdded(ObjectId id) override;
    void onEntityModified(ObjectId id) override;
    void onEntityRemoved(ObjectId id) override;

    // ========================================================================
    // 缓存数据查询接口（供渲染器使用，返回修饰后的最终数据）
    // ========================================================================
    std::vector<ObjectId> getAllEntityIds() const override;
    const EntityGraphicsCacheData& getEntityCacheData(ObjectId id) override;
    void iterateAllCacheData(const std::function<void(ObjectId id, const EntityGraphicsCacheData& cacheData)>& func) override;

    // ========================================================================
    // 临时状态通知接口（统一操作 activeStates）
    // ========================================================================
    void onEntityPreSelected(ObjectId id) override;
    void onEntityUnPreSelected(ObjectId id) override;
    void onEntitySelected(ObjectId id) override;
    void onEntityUnSelected(ObjectId id) override;
    void onEntityTempDimmed(ObjectId id) override;
    void onEntityUnTempDimmed(ObjectId id) override;

    // ========================================================================
    // 全量重生成与状态管理
    // ========================================================================
    void prepareForRegenAll() override;   // 清空基础几何，保留 activeStates，标记全部脏
    void resetAllStates() override;       // 彻底清空所有临时状态（如新建文档）

private:
    // 根据 activeStates 重建 modulatedData
    void applyModifiers(ObjectId id);

private:
    Database* m_pDatabase = nullptr;                                    // 关联的数据库指针
    std::unordered_map<ObjectId, CachedEntityData> m_cacheData;         // 实体ID到缓存数据的映射
    std::unordered_set<ObjectId> m_dirtyEntities;                       // 脏实体ID集合
    std::unordered_set<ObjectId> m_infiniteEntityIds;                   // 无限实体ID集合（射线、构造线）
    Geometry::AABB m_lastViewportAABB;                                  // 上一帧视口
    Geometry::AABB m_currentViewportAABB;                               // 当前帧视口
    bool m_regenAll = false;                                            // 全量重新生成标记
};

} // namespace tch
