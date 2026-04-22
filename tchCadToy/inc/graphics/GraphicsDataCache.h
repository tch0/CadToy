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

// =========================================================================================================================
// 图形数据缓存实现类
// 职责：管理数据库中每个实体的CPU端缓存顶点数据，维护脏实体集合
// =========================================================================================================================
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
    
    // 获取当前所有脏实体ID列表，供图形引擎查询需要重生成的实体
    std::vector<ObjectId> getDirtyEntities() const override;
    
    // 设置指定实体的缓存数据（移动语义），由图形引擎调用存储生成的顶点数据
    void setEntityCacheData(ObjectId id, EntityGraphicsCacheData&& cacheData) override;
    
    // 移除实体缓存数据，实体删除时清理对应的缓存
    void removeEntityCacheData(ObjectId id) override;
    
    // 清除指定实体的脏标记，图形引擎完成该实体缓存生成后调用
    void clearDirty(ObjectId id) override;
    
    // 全量重生成标记，提供给命令层regen、初始化时以及某些可能影响全局显示的系统变量修改等场景使用
    void markAllDirty() override { m_regenAll = true; }
    
    // 检查是否需要全量重新生成
    bool needsRegenAll() const override { return m_regenAll; }
    
    // 清除全量重新生成标记
    void clearAllDirty() override { m_regenAll = false; }
    
    // 清除所有缓存数据（全量重新生成前调用）
    void clearAllCacheData() override;

    // ========================================================================
    // 通知接口（由数据库调用，响应实体变化）
    // ========================================================================
    
    // 实体已添加，数据库添加实体后调用，将该实体标记为脏以便生成缓存
    void onEntityAdded(ObjectId id) override;
    
    // 实体已修改，数据库中实体几何或属性变化后调用，重新标记为脏
    void onEntityModified(ObjectId id) override;
    
    // 实体已删除，数据库删除实体后调用，清理该实体的缓存数据
    void onEntityRemoved(ObjectId id) override;

    // ========================================================================
    // 缓存数据查询接口（供渲染器使用）
    // ========================================================================
    
    // 获取所有实体ID，渲染器遍历使用
    std::vector<ObjectId> getAllEntityIds() const override;
    
    // 通过ID查询读取缓存数据，无效ID返回空缓存数据（kInvalidEmtpyData类型）
    const EntityGraphicsCacheData& getEntityCacheData(ObjectId id) const override;
    
    // 遍历所有缓存数据，供渲染器高效遍历所有实体缓存
    void iterateAllCacheData(const std::function<void(ObjectId id, const EntityGraphicsCacheData& cacheData)>& func) override;

private:
    Database* m_pDatabase = nullptr;                                    // 关联的数据库指针
    std::unordered_map<ObjectId, EntityGraphicsCacheData> m_cacheData;  // 实体ID到缓存数据的映射
    std::unordered_set<ObjectId> m_dirtyEntities;                       // 脏实体ID集合，需要重生成的实体
    bool m_regenAll = false;                                            // 全量重新生成标记
};

} // namespace tch
