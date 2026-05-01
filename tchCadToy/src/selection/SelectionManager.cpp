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
    IGraphicsDataCache* pCache = getGraphicsDataCache();
    if (!pDb || !pCache) { return; }

    // 创建拾取框包围盒
    Geometry::AABB pickBox = createPickBoxAABB(worldPos);

    // 查询与拾取框相交的实体（点选使用交叉模式）
    std::vector<ObjectId> ids = pDb->queryWindow(pickBox, true);

    // 使用迭代器直接构造新的预选集
    SelectionSet newPreSelectIds(ids.begin(), ids.end());

    // 更新预选状态
    updatePreSelectState(newPreSelectIds);
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
    if (!pCache) { return; }
    
    // 清除所有预选标记
    for (ObjectId id : m_currentPreSelectIds) {
        pCache->onEntityUnPreSelected(id);
    }
    
    m_previousPreSelectIds.clear();
    m_currentPreSelectIds.clear();
}

// ================================================================================================
// 确认选择
// ================================================================================================

SelectionSet SelectionManager::commitPick(const glm::dvec3& worldPos) const {
    Database* pDb = getDatabase();
    if (!pDb) { return SelectionSet(); }

    // 创建拾取框包围盒
    Geometry::AABB pickBox = createPickBoxAABB(worldPos);

    // 查询与拾取框相交的实体
    std::vector<ObjectId> ids = pDb->queryWindow(pickBox, true);

    // 使用迭代器直接构造选择集
    return SelectionSet(ids.begin(), ids.end());
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
    
    // 保存当前预选集为上一次
    m_previousPreSelectIds = m_currentPreSelectIds;
    m_currentPreSelectIds = newIds;
    
    // 计算需要添加预选标记的实体（在新集合中但不在旧集合中）
    SelectionSet toAdd = m_currentPreSelectIds - m_previousPreSelectIds;

    // 计算需要移除预选标记的实体（在旧集合中但不在新集合中）
    SelectionSet toRemove = m_previousPreSelectIds - m_currentPreSelectIds;
    
    // 通知添加预选标记
    for (ObjectId id : toAdd) {
        pCache->onEntityPreSelected(id);
    }

    // 通知移除预选标记
    for (ObjectId id : toRemove) {
        pCache->onEntityUnPreSelected(id);
    }
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

} // namespace tch
