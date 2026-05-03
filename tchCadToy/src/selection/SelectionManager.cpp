// 对应头文件
#include "SelectionManager.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "Database.h"
#include "IGraphicsDataCache.h"
#include "Renderer.h"
#include "DocManager.h"
#include "Document.h"
#include "DbEntity.h"
#include "DbLayer.h"

namespace tch {

// ================================================================================================
// 单例实现
// ================================================================================================

SelectionManager& SelectionManager::getInstance() {
    static SelectionManager instance;
    return instance;
}

SelectionManager::SelectionManager() {
}

// ================================================================================================
// 内部方法：获取当前文档的数据库和图形缓存
// ================================================================================================

Database* SelectionManager::getDatabase() const {
    return DocManager::getCurrentDocument().getDatabase();
}

IGraphicsDataCache* SelectionManager::getGraphicsDataCache() const {
    return DocManager::getCurrentDocument().getGraphicsDataCache();
}

// ================================================================================================
// 预选交互
// ================================================================================================

void SelectionManager::preSelectPick(const glm::dvec3& worldPos) {
    Database* pDb = getDatabase();
    if (!pDb) { return; }

    // 创建拾取框包围盒
    Geometry::AABB pickBox = createPickBoxAABB(worldPos);

    // 查询与拾取框相交的实体（点选使用交叉模式）
    std::vector<ObjectId> ids = pDb->queryWindow(pickBox, true);

    // 构造原始候选集（含锁定实体）
    SelectionSet newRawSet(ids.begin(), ids.end());

    // 更新点选原始候选集历史
    m_prevRawPickSet = m_currRawPickSet;
    m_currRawPickSet = newRawSet;

    // 决策当前点选的预选实体
    m_currPickPreSelectId = pickOneFromRawSet(newRawSet);

    // 构造预选实体的集合（可能为空，或包含一个锁定/非锁定实体）
    SelectionSet candidateSet;
    if (m_currPickPreSelectId != 0) {
        candidateSet.add(m_currPickPreSelectId);
    }

    // 交由统一预选处理（内部过滤锁定并更新高亮）
    updatePreSelectState(candidateSet);
}

void SelectionManager::preSelectWindow(const Geometry::AABB& rect) {
    Database* pDb = getDatabase();
    IGraphicsDataCache* pCache = getGraphicsDataCache();
    if (!pDb || !pCache) { return; }

    // 查询完全在矩形内部的实体
    std::vector<ObjectId> ids = pDb->queryWindow(rect, false);

    // 使用迭代器直接构造新的预选集
    SelectionSet newPreSelectIds(ids.begin(), ids.end());

    // 更新预选状态
    updatePreSelectState(newPreSelectIds);
}

void SelectionManager::preSelectCrossing(const Geometry::AABB& rect) {
    Database* pDb = getDatabase();
    IGraphicsDataCache* pCache = getGraphicsDataCache();
    if (!pDb || !pCache) { return; }

    // 查询与矩形相交的实体
    std::vector<ObjectId> ids = pDb->queryWindow(rect, true);

    // 使用迭代器直接构造新的预选集
    SelectionSet newPreSelectIds(ids.begin(), ids.end());

    // 更新预选状态
    updatePreSelectState(newPreSelectIds);
}

void SelectionManager::clearPreSelect() {
    IGraphicsDataCache* pCache = getGraphicsDataCache();
    if (pCache) {
        // 清除所有高亮预选标记
        for (ObjectId id : m_currHighlightSet) {
            pCache->onEntityUnPreSelected(id);
        }
    }

    // 清理所有相关状态
    m_prevPreSelectSet.clear();
    m_currPreSelectSet.clear();
    m_prevRawPickSet.clear();
    m_currRawPickSet.clear();
    m_currPickPreSelectId = 0;
    m_prevPickPreSelectId = 0;
    m_prevHighlightSet.clear();
    m_currHighlightSet.clear();
    m_lockCache.clear();
    m_isPreSelectEntOnLockedLayer = false;
}

// ================================================================================================
// 确认选择
// ================================================================================================

SelectionSet SelectionManager::commitPick(const glm::dvec3&) const {
    // 返回当前点选决策的候选实体
    SelectionSet result;
    if (m_currPickPreSelectId != 0) {
        result.add(m_currPickPreSelectId);
    }
    return result;
}

SelectionSet SelectionManager::commitWindow(const Geometry::AABB& rect) const {
    Database* pDb = getDatabase();
    if (!pDb) { return SelectionSet(); }

    // 查询完全在矩形内部的实体
    std::vector<ObjectId> ids = pDb->queryWindow(rect, false);

    // 使用迭代器直接构造选择集
    return SelectionSet(ids.begin(), ids.end());
}

SelectionSet SelectionManager::commitCrossing(const Geometry::AABB& rect) const {
    Database* pDb = getDatabase();
    if (!pDb) { return SelectionSet(); }

    // 查询与矩形相交的实体
    std::vector<ObjectId> ids = pDb->queryWindow(rect, true);

    // 使用迭代器直接构造选择集
    return SelectionSet(ids.begin(), ids.end());
}

// ================================================================================================
// 内部方法
// ================================================================================================

void SelectionManager::updatePreSelectState(const SelectionSet& newIds) {
    IGraphicsDataCache* pCache = getGraphicsDataCache();
    if (!pCache) { return; }

    // 保存当前预选集为上一次（用于窗选/交叉窗选）
    m_prevPreSelectSet = m_currPreSelectSet;
    m_currPreSelectSet = newIds;

    // 过滤锁定实体，得到本帧应高亮的集合
    SelectionSet filteredNew;
    for (ObjectId id : newIds) {
        if (!isEntityLocked(id)) {
            filteredNew.add(id);
        }
    }

    // 保存实际高亮集合的历史
    m_prevHighlightSet = m_currHighlightSet;
    m_currHighlightSet = filteredNew;

    // 计算需要添加预选标记的实体（在新集合中但不在旧集合中）
    SelectionSet toAdd = m_currHighlightSet - m_prevHighlightSet;

    // 计算需要移除预选标记的实体（在旧集合中但不在新集合中）
    SelectionSet toRemove = m_prevHighlightSet - m_currHighlightSet;

    // 通知添加预选标记
    for (ObjectId id : toAdd) {
        pCache->onEntityPreSelected(id);
    }

    // 通知移除预选标记
    for (ObjectId id : toRemove) {
        pCache->onEntityUnPreSelected(id);
    }

    // 锁定光标标记：预选实体集合中只有一个实体且该实体在锁定图层上（过滤后为空，原始集合有一个）
    m_isPreSelectEntOnLockedLayer = (filteredNew.empty() && newIds.size() == 1);
}

Geometry::AABB SelectionManager::createPickBoxAABB(const glm::dvec3& worldPos) const {
    // 获取拾取框大小（屏幕像素），这个大小就是拾取框边长1/2
    float pickBoxSize = Renderer::getPickBoxSize();

    // 将屏幕拾取框大小转换为世界坐标
    // 使用Renderer的变换管理器进行转换
    glm::vec2 screenPos = Renderer::getTransformManager().worldToScreen(worldPos);

    // 计算拾取框的屏幕坐标范围（pickBoxSize是半边长，直接使用）
    glm::vec2 minScreen(screenPos.x - pickBoxSize, screenPos.y - pickBoxSize);
    glm::vec2 maxScreen(screenPos.x + pickBoxSize, screenPos.y + pickBoxSize);
    
    // 转换到世界坐标
    glm::dvec3 minWorld = Renderer::getTransformManager().screenToWorld(minScreen);
    glm::dvec3 maxWorld = Renderer::getTransformManager().screenToWorld(maxScreen);
    
    return Geometry::AABB(minWorld, maxWorld);
}

// ================================================================================================
// 锁定查询与点选决策
// ================================================================================================

// 查询实体是否在锁定图层上，带缓存，存储本次选择交互所有预选到的实体，只在第一次查询，后续直接读取缓存
bool SelectionManager::isEntityLocked(ObjectId id) const {
    auto it = m_lockCache.find(id);
    if (it != m_lockCache.end()) {
        return it->second;
    }

    bool locked = false;
    if (Database* pDb = getDatabase()) {
        if (DbEntity* pEntity = pDb->getEntity(id)) {
            if (ObjectId layerId = pEntity->layerId()) {
                if (DbLayer* pLayer = pDb->getLayer(layerId)) {
                    locked = pLayer->isLocked();
                }
            }
        }
    }
    m_lockCache[id] = locked;
    return locked;
}

ObjectId SelectionManager::pickOneFromRawSet(const SelectionSet& rawSet) {
    if (rawSet.empty()) {
        m_prevPickPreSelectId = 0;
        return 0;
    }

    // 1. 有新增实体的话，找新增实体中 ID 最大的（使用差集，取最后一个）
    SelectionSet newIds = rawSet - m_prevRawPickSet;
    if (!newIds.empty()) {
        m_prevPickPreSelectId = *newIds.rbegin();
        return m_prevPickPreSelectId;
    }

    // 2. 无新增，检查上一帧选中的是否仍在集合中，在的话保持
    if (m_prevPickPreSelectId != 0 && rawSet.contains(m_prevPickPreSelectId)) {
        return m_prevPickPreSelectId;
    }

    // 3. 上一帧预选实体已经不在集合中，返回集合中 ID 最大的（最后一个）
    m_prevPickPreSelectId = *rawSet.rbegin();
    return m_prevPickPreSelectId;
}

} // namespace tch
