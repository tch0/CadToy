// 对应头文件
#include "GraphicsDataCache.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "Database.h"
#include "DbEntity.h"
#include "GraphicsEngine.h"

namespace tch {

// ============================================================================
// 查询与写入接口
// ============================================================================

std::vector<ObjectId> GraphicsDataCache::getDirtyEntities() const {
    return std::vector<ObjectId>(m_dirtyEntities.begin(), m_dirtyEntities.end());
}

void GraphicsDataCache::setEntityCacheData(ObjectId id, EntityGraphicsCacheData&& cacheData) {
    // 获取或创建 CachedEntityData 条目，正常情况下这里都是获取，实体添加时缓存实体数据已经创建
    auto& cached = m_cacheData[id];
    
    // 将传入的缓存数据移动赋值给 baseData
    cached.baseData = std::move(cacheData);
    
    // 保留原有的 activeStates（若为新实体则为0）
    // 标记修饰数据需要重建
    cached.modulatedDirty = true;
}

void GraphicsDataCache::removeEntityCacheData(ObjectId id) {
    m_cacheData.erase(id);
    m_dirtyEntities.erase(id);
    m_infiniteEntityIds.erase(id);
}

void GraphicsDataCache::clearDirty(ObjectId id) {
    m_dirtyEntities.erase(id);
}

void GraphicsDataCache::clearAllCacheData() {
    m_cacheData.clear();
    m_dirtyEntities.clear();
    m_infiniteEntityIds.clear();
}

bool GraphicsDataCache::isCacheDirty(ObjectId id) const {
    return m_dirtyEntities.find(id) != m_dirtyEntities.end();
}

// ============================================================================
// 视口管理
// ============================================================================

void GraphicsDataCache::updateViewport(const Geometry::AABB& newViewport) {
    // 比较视口是否变化（使用容差）
    if (m_currentViewportAABB != newViewport) {
        m_lastViewportAABB = m_currentViewportAABB;
        m_currentViewportAABB = newViewport;
        
        // 视口变化，将所有无限实体标记为脏
        for (ObjectId id : m_infiniteEntityIds) {
            m_dirtyEntities.insert(id);
        }
    }
}

// ============================================================================
// 通知接口
// ============================================================================

void GraphicsDataCache::onEntityAdded(ObjectId id) {
    if (id == 0) {
        return;
    }

    // 插入默认构造的实体缓存数据
    m_cacheData.emplace(id, CachedEntityData{});

    // 新实体标记为脏，需要生成基础几何数据
    m_dirtyEntities.insert(id);
    
    // 检查是否为无限实体
    if (m_pDatabase) {
        DbEntity* pEntity = m_pDatabase->getEntity(id);
        if (pEntity && pEntity->isInfinite()) {
            m_infiniteEntityIds.insert(id);
        }
    }
}

void GraphicsDataCache::onEntityModified(ObjectId id) {
    if (id == 0) {
        return;
    }
    // 修改的实体标记为脏
    m_dirtyEntities.insert(id);
}

void GraphicsDataCache::onEntityRemoved(ObjectId id) {
    if (id == 0) {
        return;
    }
    // 移除实体时清理缓存
    removeEntityCacheData(id);
}

// ============================================================================
// 缓存数据查询接口（返回修饰后的最终数据）
// ============================================================================

std::vector<ObjectId> GraphicsDataCache::getAllEntityIds() const {
    std::vector<ObjectId> ids;
    ids.reserve(m_cacheData.size());
    for (const auto& pair : m_cacheData) {
        ids.push_back(pair.first);
    }
    return ids;
}

const EntityGraphicsCacheData& GraphicsDataCache::getEntityCacheData(ObjectId id) {
    static const EntityGraphicsCacheData kEmptyCacheData;
    
    if (id == 0) {
        return kEmptyCacheData;
    }
    
    // 如果实体在脏集合中，调用引擎生成该实体（按需生成）
    if (isCacheDirty(id)) {
        GraphicsEngine::getInstance().generateForEntity(this, id);
    }
    
    // 获取缓存数据
    auto it = m_cacheData.find(id);
    if (it == m_cacheData.end()) {
        return kEmptyCacheData;
    }
    
    auto& cached = it->second;
    
    // 如果修饰数据需要重建，调用 applyModifiers
    if (cached.modulatedDirty) {
        applyModifiers(id);
    }
    
    // 返回修饰后的数据（如果有临时状态）或基础数据
    return (cached.activeStates != 0) ? cached.modulatedData : cached.baseData;
}

void GraphicsDataCache::iterateAllCacheData(const std::function<void(ObjectId id, const EntityGraphicsCacheData& cacheData)>& func) {
    if (!func) {
        return;
    }
    
    for (auto& pair : m_cacheData) {
        // 通过 getEntityCacheData 获取（会自动处理修饰）
        const auto& cacheData = getEntityCacheData(pair.first);
        if (cacheData.type != EntityGraphicsCacheData::kInvalidEmptyData) {
            func(pair.first, cacheData);
        }
    }
}

// ============================================================================
// 临时状态通知接口
// ============================================================================

void GraphicsDataCache::onEntityPreSelected(ObjectId id) {
    auto it = m_cacheData.find(id);
    if (it == m_cacheData.end()) {
        return;
    }
    
    // 如果实体已被选中，不进行预选高亮
    if (it->second.activeStates & DataCacheVertex::kFlagSelected) {
        return;
    }
    
    it->second.activeStates |= DataCacheVertex::kFlagPreSelected;
    it->second.modulatedDirty = true;
}

void GraphicsDataCache::onEntityUnPreSelected(ObjectId id) {
    auto it = m_cacheData.find(id);
    if (it == m_cacheData.end()) {
        return;
    }
    it->second.activeStates &= ~DataCacheVertex::kFlagPreSelected;
    it->second.modulatedDirty = true;
}

void GraphicsDataCache::onEntitySelected(ObjectId id) {
    auto it = m_cacheData.find(id);
    if (it == m_cacheData.end()) {
        return;
    }
    it->second.activeStates |= DataCacheVertex::kFlagSelected;
    it->second.modulatedDirty = true;
}

void GraphicsDataCache::onEntityUnSelected(ObjectId id) {
    auto it = m_cacheData.find(id);
    if (it == m_cacheData.end()) {
        return;
    }
    it->second.activeStates &= ~DataCacheVertex::kFlagSelected;
    it->second.modulatedDirty = true;
}

void GraphicsDataCache::onEntityTempDimmed(ObjectId id) {
    auto it = m_cacheData.find(id);
    if (it == m_cacheData.end()) {
        return;
    }
    it->second.activeStates |= DataCacheVertex::kFlagTempDimmed;
    it->second.modulatedDirty = true;
}

void GraphicsDataCache::onEntityUnTempDimmed(ObjectId id) {
    auto it = m_cacheData.find(id);
    if (it == m_cacheData.end()) {
        return;
    }
    it->second.activeStates &= ~DataCacheVertex::kFlagTempDimmed;
    it->second.modulatedDirty = true;
}

// ============================================================================
// 全量重生成与状态管理
// ============================================================================

void GraphicsDataCache::prepareForRegenAll() {
    // 遍历所有缓存数据，清空基础几何但保留 activeStates
    for (auto& [id, cached] : m_cacheData) {
        cached.baseData.type = EntityGraphicsCacheData::kInvalidEmptyData;
        cached.baseData.vertices.clear();
        cached.modulatedDirty = true;

        // 标记为脏
        m_dirtyEntities.insert(id);
    }

    // 如果没有缓存数据，标记全量重生成
    if (m_cacheData.empty()) {
        m_regenAll = true;
    }
}

void GraphicsDataCache::resetAllStates() {
    // 遍历所有缓存数据，清空所有临时状态
    for (auto& [_, cached] : m_cacheData) {
        cached.activeStates = 0;
        cached.modulatedDirty = true;
    }
}

// ============================================================================
// 私有方法：根据 activeStates 重建 modulatedData
// ============================================================================

void GraphicsDataCache::applyModifiers(ObjectId id) {
    auto it = m_cacheData.find(id);
    if (it == m_cacheData.end()) {
        return;
    }
    
    auto& cached = it->second;
    
    // 如果没有临时状态，不需要生成修饰数据，直接返回
    if (cached.activeStates == 0) {
        cached.modulatedDirty = false;
        return;
    }
    
    // 拷贝基础数据到修饰数据
    cached.modulatedData = cached.baseData;
    
    // 根据状态调整类型（例如预选强制线宽）
    if (cached.activeStates & DataCacheVertex::kFlagPreSelected) {
        cached.modulatedData.type = EntityGraphicsCacheData::kAlwaysShowLineWidth;
    }

    // 遍历顶点，清除所有临时标志后重新设置
    for (auto& v : cached.modulatedData.vertices) {
        // 清除临时状态标志（保留图层锁定暗显）
        v.flags &= ~DataCacheVertex::kAllTempFlags;
        
        // 根据 activeStates 设置标志（标志位与 DataCacheVertex 一致，可直接设置）
        v.flags |= cached.activeStates;
        // 注意：kFlagLockedLayerDimmed 已在 baseData 中，经拷贝保留，不会被清除，不由缓存负责管理
    }
    
    cached.modulatedDirty = false;
}

} // namespace tch
