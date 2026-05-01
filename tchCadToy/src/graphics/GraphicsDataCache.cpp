// 对应头文件
#include "GraphicsDataCache.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "IGraphicsDataCache.h"
#include "GraphicsEngine.h"
#include "Database.h"
#include "DbEntity.h"

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
    
    // 同步清除预选缓存数据
    m_preSelectedCacheData.erase(id);
    
    // 同步清除选中缓存数据
    m_selectedCacheData.erase(id);
}

void GraphicsDataCache::clearDirty(ObjectId id) {
    if (id == 0) {
        return;
    }
    
    m_dirtyEntities.erase(id);
}

void GraphicsDataCache::clearAllCacheData() {
    m_cacheData.clear();
    m_dirtyEntities.clear();
    
    // 同步清除所有预选缓存数据
    m_preSelectedCacheData.clear();
    
    // 同步清除所有选中缓存数据
    m_selectedCacheData.clear();
}

// 判断某实体顶点缓存是否脏
bool GraphicsDataCache::isCacheDirty(ObjectId id) const {
    if (id == 0) { return false; }
    // 如果实体在脏集合中，或者缓存中不存在该实体，则认为缓存是脏的
    if (m_dirtyEntities.find(id) != m_dirtyEntities.end()) { return true; }
    return m_cacheData.find(id) == m_cacheData.end();
}

// ============================================================================
// 无限实体相关接口：视口更新与获取、无限实体的生成
// 无限实体随视口变化而重生成，比较特殊，所以需要单独处理
// ============================================================================

void GraphicsDataCache::updateViewport(const Geometry::AABB& newViewport) {
    m_lastViewportAABB = m_currentViewportAABB;
    m_currentViewportAABB = newViewport;
}

// 无限实体生成接口：由数据缓存负责组装无限实体数据，做帧间状态保持（现在相关状态），几何数据则还是由图形引擎来做
void GraphicsDataCache::generateInfiniteEntities() {
    if (!m_pDatabase) {
        return;
    }

    bool viewportChanged = (m_lastViewportAABB != m_currentViewportAABB);

    // 生成所有需要重生成的无限实体
    for (ObjectId id : m_infiniteEntityIds) {
        // 只有视口变化或实体为脏时才生成
        if (!viewportChanged && !isCacheDirty(id)) {
            continue;
        }

        // 获取上一帧的预选/选中状态
        bool bPreSelected = false;
        bool bSelected = false;
        auto it = m_cacheData.find(id);
        if (it != m_cacheData.end()) {
            bPreSelected = it->second.bPreSelected;
            bSelected = it->second.bSelected;
        }

        // 调用图形引擎生成缓存
        GraphicsEngine::getInstance().generateForEntity(this, id);

        // 获取生成后的缓存数据，设置选择状态
        auto cacheIt = m_cacheData.find(id);
        if (cacheIt != m_cacheData.end()) {
            cacheIt->second.bPreSelected = bPreSelected;
            cacheIt->second.bSelected = bSelected;
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
    
    m_dirtyEntities.insert(id);

    // 检查是否为无限实体（射线、构造线）
    if (m_pDatabase) {
        if (auto* pEnt = m_pDatabase->getEntity(id)) {
            if (pEnt->isInfinite()) {
                m_infiniteEntityIds.insert(id);
            }
        }
    }
}

void GraphicsDataCache::onEntityModified(ObjectId id) {
    if (id == 0) {
        return;
    }

    m_dirtyEntities.insert(id);

    // 实体修改时清除预选缓存数据，下次获取时重新生成
    m_preSelectedCacheData.erase(id);

    // 实体修改时清除选中缓存数据，下次获取时重新生成
    m_selectedCacheData.erase(id);
}

void GraphicsDataCache::onEntityRemoved(ObjectId id) {
    if (id == 0) {
        return;
    }

    m_cacheData.erase(id);
    m_dirtyEntities.erase(id);
    m_infiniteEntityIds.erase(id);  // 从无限实体集合移除

    // 同步清除预选缓存数据
    m_preSelectedCacheData.erase(id);

    // 同步清除选中缓存数据
    m_selectedCacheData.erase(id);
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

const EntityGraphicsCacheData& GraphicsDataCache::getEntityCacheData(ObjectId id) {
    // 局部静态空缓存数据，用于返回无效ID的引用
    static const EntityGraphicsCacheData kEmptyCacheData;

    if (id == 0) { return kEmptyCacheData; }

    // 检查是否需要重生成（脏标记或缓存不存在）
    // 如果实体在脏集合中，或者缓存中不存在该实体，则需要重生成
    bool needRegen = (m_dirtyEntities.find(id) != m_dirtyEntities.end()) || (m_cacheData.find(id) == m_cacheData.end());
    if (needRegen) {
        // 直接调用图形引擎生成单个实体的缓存
        GraphicsEngine::getInstance().generateForEntity(this, id);
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

// ============================================================================
// 预选实体缓存数据相关接口
// ============================================================================
// 根据ID查询预选实体的预选缓存数据
const EntityGraphicsCacheData& GraphicsDataCache::getPreSelectedEntityCacheData(ObjectId id) const {
    // 局部静态空缓存数据，用于返回无效ID的引用
    static const EntityGraphicsCacheData kEmptyCacheData;

    if (id == 0) {
        return kEmptyCacheData;
    }

    // 检查正常数据中的预选标记
    auto normalIt = m_cacheData.find(id);
    if (normalIt == m_cacheData.end() || !normalIt->second.bPreSelected) {
        return kEmptyCacheData;
    }

    // 检查是否为无限实体（XLine/Ray）
    bool isInfinite = m_infiniteEntityIds.find(id) != m_infiniteEntityIds.end();

    // 普通实体：检查预选数据是否已存在，存在则直接返回
    if (!isInfinite) {
        auto it = m_preSelectedCacheData.find(id);
        if (it != m_preSelectedCacheData.end()) {
            return it->second;
        }
    }

    // 懒生成：从正常数据拷贝并设置预选标记
    EntityGraphicsCacheData preSelectedData = normalIt->second;  // 拷贝
    preSelectedData.bPreSelected = true;  // 设置预选标记
    preSelectedData.type = EntityGraphicsCacheData::kAlwaysShowLineWidth;  // 预选高亮总是显示线宽

    // 设置顶点预选标记
    for (auto& vertex : preSelectedData.vertices) {
        vertex.flags |= DataCacheVertex::kFlagPreSelected;
    }

    // 无限实体每帧都重新插入/更新，普通实体首次插入
    auto result = m_preSelectedCacheData.insert_or_assign(id, std::move(preSelectedData));
    return result.first->second;
}

// 实体被预选中，通知后需要设置数据预选标记，标记为脏，获取时懒生成即可（没有就生成，有就读取）
void GraphicsDataCache::onEntityPreSelected(ObjectId id) {
    if (id == 0) {
        return;
    }

    // 设置正常数据的预选标记
    auto it = m_cacheData.find(id);
    if (it != m_cacheData.end()) {
        it->second.bPreSelected = true;
    }
}

// 实体从预选状态移除，清除预选标记，预选数据不需要同时清除，几何重生成时才需要重新生成或者直接移除
void GraphicsDataCache::onEntityUnPreSelected(ObjectId id) {
    if (id == 0) {
        return;
    }

    // 清除正常数据的预选标记
    auto it = m_cacheData.find(id);
    if (it != m_cacheData.end()) {
        it->second.bPreSelected = false;
    }

    // 注意：不清除预选缓存数据，因为预选状态可能随着鼠标移动多次进入/退出
    // 预选数据会在几何重生成（onEntityModified）或实体移除时清除
}

// ============================================================================
// 选中实体缓存数据相关接口
// ============================================================================

const EntityGraphicsCacheData& GraphicsDataCache::getSelectedEntityCacheData(ObjectId id) const {
    // 局部静态空缓存数据，用于返回无效ID的引用
    static const EntityGraphicsCacheData kEmptyCacheData;

    if (id == 0) {
        return kEmptyCacheData;
    }

    // 检查正常数据中的选中标记
    auto normalIt = m_cacheData.find(id);
    if (normalIt == m_cacheData.end() || !normalIt->second.bSelected) {
        return kEmptyCacheData;
    }

    // 检查是否为无限实体（XLine/Ray）
    bool isInfinite = m_infiniteEntityIds.find(id) != m_infiniteEntityIds.end();

    // 普通实体：检查选中数据是否已存在，存在则直接返回
    if (!isInfinite) {
        auto it = m_selectedCacheData.find(id);
        if (it != m_selectedCacheData.end()) {
            return it->second;
        }
    }

    // 懒生成：从正常数据拷贝并设置选中标记
    EntityGraphicsCacheData selectedData = normalIt->second;  // 拷贝
    selectedData.bSelected = true;  // 设置选中标记

    // 设置顶点选中标记
    for (auto& vertex : selectedData.vertices) {
        vertex.flags |= DataCacheVertex::kFlagSelected;
    }

    // 无限实体每帧都重新插入/更新，普通实体首次插入
    auto result = m_selectedCacheData.insert_or_assign(id, std::move(selectedData));
    return result.first->second;
}

void GraphicsDataCache::onEntitySelected(ObjectId id) {
    if (id == 0) {
        return;
    }

    // 设置正常数据的选中标记
    auto it = m_cacheData.find(id);
    if (it != m_cacheData.end()) {
        it->second.bSelected = true;
    }
}

void GraphicsDataCache::onEntityUnSelected(ObjectId id) {
    if (id == 0) {
        return;
    }

    // 清除正常数据的选中标记
    auto it = m_cacheData.find(id);
    if (it != m_cacheData.end()) {
        it->second.bSelected = false;
    }

    // 注意：不清除选中缓存数据，因为选中状态可能多次进入/退出
    // 选中数据会在几何重生成（onEntityModified）或实体移除时清除
}

} // namespace tch
