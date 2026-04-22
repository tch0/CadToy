// 对应头文件
#include "GraphicsDataCache.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "IGraphicsDataCache.h"


namespace tch {

// ============================================================================
// 查询与写入接口
// ============================================================================

std::vector<ObjectId> GraphicsDataCache::getDirtyEntities() const {
    return std::vector<ObjectId>(m_dirtyEntities.begin(), m_dirtyEntities.end());
}

void GraphicsDataCache::setEntityCacheData(ObjectId id, EntityGraphicsCacheData&& cacheData) {
    if (id == 0) {
        return;
    }
    
    m_cacheData[id] = std::move(cacheData);
}

void GraphicsDataCache::removeEntityCacheData(ObjectId id) {
    if (id == 0) {
        return;
    }
    
    m_cacheData.erase(id);
    m_dirtyEntities.erase(id);
}

void GraphicsDataCache::clearDirty(ObjectId id) {
    if (id == 0) {
        return;
    }
    
    m_dirtyEntities.erase(id);
}

void GraphicsDataCache::markAllDirty() {
    // 将所有有缓存数据的实体标记为脏
    m_dirtyEntities.clear();
    for (const auto& [id, _] : m_cacheData) {
        m_dirtyEntities.insert(id);
    }
}

// ============================================================================
// 通知接口
// ============================================================================

void GraphicsDataCache::onEntityAdded(ObjectId id) {
    if (id == 0) {
        return;
    }
    
    m_dirtyEntities.insert(id);
}

void GraphicsDataCache::onEntityModified(ObjectId id) {
    if (id == 0) {
        return;
    }
    
    m_dirtyEntities.insert(id);
}

void GraphicsDataCache::onEntityRemoved(ObjectId id) {
    if (id == 0) {
        return;
    }
    
    m_cacheData.erase(id);
    m_dirtyEntities.erase(id);
}

// ============================================================================
// 缓存数据查询接口
// ============================================================================

std::vector<ObjectId> GraphicsDataCache::getAllEntityIds() const {
    std::vector<ObjectId> ids;
    ids.reserve(m_cacheData.size());
    
    for (const auto& [id, _] : m_cacheData) {
        ids.push_back(id);
    }
    
    return ids;
}

const EntityGraphicsCacheData& GraphicsDataCache::getEntityCacheData(ObjectId id) const {
    // 局部静态空缓存数据，用于返回无效ID的引用
    static const EntityGraphicsCacheData kEmptyCacheData;
    
    if (id == 0) {
        return kEmptyCacheData;
    }
    
    auto it = m_cacheData.find(id);
    if (it != m_cacheData.end()) {
        return it->second;
    }
    
    return kEmptyCacheData;
}

void GraphicsDataCache::iterateAllCacheData(
    const std::function<void(ObjectId id, const EntityGraphicsCacheData& cacheData)>& func) {
    if (!func) {
        return;
    }
    
    for (const auto& [id, cacheData] : m_cacheData) {
        func(id, cacheData);
    }
}

} // namespace tch
