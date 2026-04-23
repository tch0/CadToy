#pragma once

// C++ 标准库
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <vector>
#include <string>

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

// 项目头文件
#include "DbObject.h"
#include "DbCommon.h"


namespace tch {

// 前向声明
class DbObject;
class DbEntity;
class DbLayer;
class IGraphicsDataCache;

// =======================================================================================================
// Database - 数据库核心类
// =======================================================================================================

class Database {
public:
    // ID 分区常量
    static constexpr ObjectId kSystemStart = 1;
    static constexpr ObjectId kSystemEnd = 999;
    static constexpr ObjectId kSymbolStart = 1000;
    static constexpr ObjectId kSymbolEnd = 9999;
    static constexpr ObjectId kEntityStart = 10000;

    Database();
    ~Database();

    // 禁用拷贝
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // =======================================================================================
    // 对象、实体管理（命令层使用）
    // =======================================================================================

    // 添加对象，自动分配 ID（根据类型分发到 addEntity/addLayer）
    ObjectId addObject(std::unique_ptr<DbObject> pObj);

    // 添加实体，自动分配 ID 并维护图层索引
    ObjectId addEntity(std::unique_ptr<DbObject> pObj);

    // 获取对象（不包括备份区的）
    DbObject* getObject(ObjectId id) const;

    // 获取实体
    DbEntity* getEntity(ObjectId id) const;

    // 检查对象是否存在（不包括备份区的）
    bool hasObject(ObjectId id) const;

    // 移除对象（移动到备份区而不实际删除，自动分发到 removeEntity/removeLayer）
    bool removeObject(ObjectId id);

    // 移除实体（移动到备份区，维护图层索引）
    bool removeEntity(ObjectId id);

    // 永久删除对象
    void eraseObject(ObjectId id);

    // =======================================================================================
    // Undo/Redo 支持（UndoManager 使用）
    // =======================================================================================

    // 分配普通备份 ID（用于修改、添加预留）
    ObjectId allocateBackupId();

    // 获取删除备份 ID（静态方法，供 UndoManager 使用）
    static ObjectId getRemoveBackupId(ObjectId objId);

    // 创建修改备份（克隆对象到备份区，使用指定的 backupId）
    void backupForModify(ObjectId objId, ObjectId backupId);

    // 从备份区恢复对象（使用指定的 backupId）
    void restoreFromBackup(ObjectId objId, ObjectId backupId);

    // 交换对象和备份（用于修改的 undo/redo）
    void swapWithBackup(ObjectId objId, ObjectId backupId);

    // 转移对象到备份区（用于添加的 undo）
    void moveToBackup(ObjectId objId, ObjectId backupId);

    // 检查备份是否存在
    bool hasBackup(ObjectId backupId) const;

    // 获取备份对象
    DbObject* getBackup(ObjectId backupId) const;

    // 从备份区永久删除指定备份实体
    void removeBackup(ObjectId backupId);

    // 清理冗余备份实体
    void purge();
    
    // =======================================================================================
    // 图层管理
    // =======================================================================================

    // 添加图层对象，自动分配 ID 并维护图层列表
    ObjectId addLayer(std::unique_ptr<DbObject> pObj);

    // 添加图层，返回图层 ID
    ObjectId addLayer(const std::string& name);

    // 获取图层
    DbLayer* getLayer(ObjectId id) const;

    // 根据名称获取图层
    DbLayer* getLayerByName(const std::string& name) const;

    // 检查图层名称是否存在
    bool layerExists(const std::string& name) const;

    // 删除图层，返回是否成功
    bool removeLayer(ObjectId id);

    // 获取所有图层 ID
    const std::vector<ObjectId>& layerIds() const { return m_layerIds; }

    // 获取当前图层
    DbLayer* currentLayer() const;
    
    // 移动实体到指定图层，返回实际设置的图层ID（如果目标图层不存在则移动到当前图层并返回当前图层ID）
    // 提供给实体setLayerId调用，以正确维护索引表，不再其他任何地方调用
    ObjectId moveEntityToLayer(ObjectId entityId, ObjectId targetLayerId);

    // 获取指定图层上的所有实体ID（如果不存在返回空集合）
    const std::unordered_set<ObjectId>& getEntitiesOnLayer(ObjectId layerId) const;

    // =======================================================================================
    // 文档属性
    // =======================================================================================

    // 默认线宽 (LWDEFAULT)
    DbLineWeight defaultLineWeight() const { return m_defaultLineWeight; }
    void setDefaultLineWeight(DbLineWeight lw);

    // 线型比例 (LTSCALE)，全局线型比例缩放因子，影响所有实体显示
    double linetypeScale() const { return m_linetypeScale; }
    void setLinetypeScale(double scale);

    // 当前实体线型比例 (CELTSCALE)，新建实体的默认线型比例
    double currentEntityLinetypeScale() const { return m_currentEntityLinetypeScale; }
    void setCurrentEntityLinetypeScale(double scale);

    // 当前图层 (CLAYER)
    ObjectId currentLayerId() const { return m_currentLayerId; }
    void setCurrentLayerId(ObjectId id);

    // 线宽显示 (LWDISPLAY)，是否显示线宽，0=不显示，1=显示
    bool lineWeightDisplay() const { return m_lineWeightDisplay; }
    void setLineWeightDisplay(bool display);

    // 当前实体颜色 (CECOLOR)，新建实体的默认颜色，默认ByLayer
    DbColor currentEntityColor() const { return m_currentEntityColor; }
    void setCurrentEntityColor(const DbColor& color);

    // 当前实体线型 (CELTYPE)，新建实体的默认线型，默认ByLayer
    DbLinetypeRef currentEntityLinetype() const { return m_currentEntityLinetype; }
    void setCurrentEntityLinetype(const DbLinetypeRef& lt);

    // 当前实体线宽 (CELWEIGHT)，新建实体的默认线宽，默认ByLayer
    DbLineWeight currentEntityLineWeight() const { return m_currentEntityLineWeight; }
    void setCurrentEntityLineWeight(DbLineWeight lw);

    // =======================================================================================
    // 遍历和查询
    // =======================================================================================

    // 遍历所有对象（不包括备份区的），回调中不应该修改容器，比如添加移除对象
    void forEachObject(const std::function<void(DbObject*)>& callback) const;

    // 遍历所有实体（不包括备份区的），回调中不应该修改容器，比如添加移除对象
    void forEachEntity(const std::function<void(DbEntity*)>& callback) const;

    // 遍历所有图层，回调中不应该修改容器，比如添加移除对象
    void forEachLayer(const std::function<void(DbLayer*)>& callback) const;

    // 遍历所有备份区的对象
    void forEachInBackup(const std::function<void(DbObject*)>& callback) const;

    // 获取对象数量
    size_t objectCount() const { return m_objects.size(); }

    // 获取备份数量
    size_t backupCount() const { return m_backupObjects.size(); }

    // =======================================================================================
    // 序列化
    // =======================================================================================

    void saveToJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer);
    bool loadFromJson(const rapidjson::Value& value);

    // =======================================================================================
    // 图形数据缓存关联
    // =======================================================================================

    // 设置图形数据缓存指针（文档构造时调用）
    void setGraphicsDataCache(IGraphicsDataCache* pCache) { m_pGraphicsCache = pCache; }

    // 获取图形数据缓存指针
    IGraphicsDataCache* getGraphicsDataCache() const { return m_pGraphicsCache; }

    // =======================================================================================
    // 修改标记
    // =======================================================================================

    // 数据库是否被修改
    bool isModified() const { return m_modified; }

    // 清除修改标记（saveToJson后自动调用）
    void clearModified() { m_modified = false; }

    // =======================================================================================
    // 通知接口（供 DbObject 调用）
    // =======================================================================================

    // 实体被修改
    void onEntityModified(ObjectId id);

    // 图层被修改
    void onLayerModified(ObjectId id);

private:
    // 对象存储
    std::unordered_map<ObjectId, std::unique_ptr<DbObject>> m_objects;

    // 图层列表（按添加顺序）
    std::vector<ObjectId> m_layerIds;
    
    // 图层ID到图层中实体ID集合的索引表
    std::unordered_map<ObjectId, std::unordered_set<ObjectId>> m_layerEntityIndex;
    
    // 图层名称到 ID 的映射
    std::unordered_map<std::string, ObjectId> m_layerNameMap;
    
    // 备份对象存储（用于 undo/redo）
    std::unordered_map<ObjectId, std::unique_ptr<DbObject>> m_backupObjects;

    // 文档属性 (即文档级别的系统变量)
    DbLineWeight m_defaultLineWeight = DbLineWeight::k000;  // 默认线宽 (LWDEFAULT)
    double m_linetypeScale = 1.0;                           // 线型比例 (LTSCALE)，全局缩放因子
    double m_currentEntityLinetypeScale = 1.0;              // 当前实体线型比例 (CELTSCALE)，新建实体默认值
    ObjectId m_currentLayerId = 0;                          // 当前图层 (CLAYER)
    bool m_lineWeightDisplay = false;                       // 线宽显示 (LWDISPLAY)，是否显示线宽
    DbColor m_currentEntityColor = DbColor::byLayer();                  // 当前实体颜色 (CECOLOR)，新建实体默认颜色，默认ByLayer
    DbLinetypeRef m_currentEntityLinetype = DbLinetypeRef::byLayer();   // 当前实体线型 (CELTYPE)，新建实体默认线型，默认ByLayer
    DbLineWeight m_currentEntityLineWeight = DbLineWeight::kByLayer;    // 当前实体线宽 (CELWEIGHT)，新建实体默认线宽，默认ByLayer

    // 脏标记
    bool m_modified = false;                                // 数据库是否被修改

    // 下一个可用 ID
    ObjectId m_nextSystemId = kSystemStart;
    ObjectId m_nextSymbolId = kSymbolStart;
    ObjectId m_nextEntityId = kEntityStart;
    ObjectId m_nextBackupId = 1;  // 普通备份 ID 从 1 开始

    // 图形数据缓存指针（不参与序列化）
    IGraphicsDataCache* m_pGraphicsCache = nullptr;

    // 分配 ID
    ObjectId allocateId(DbObject::Type type);

    // 确保有"0"图层，返回其ID
    ObjectId ensureLayerZero();

    // =======================================================================================
    // 表格维护辅助函数（集中管理索引表一致性）
    // =======================================================================================

    // 实体索引维护
    void addEntityToIndex(ObjectId entityId, ObjectId layerId);
    void removeEntityFromIndex(ObjectId entityId, ObjectId layerId);
    void moveEntityInIndex(ObjectId entityId, ObjectId oldLayerId, ObjectId newLayerId);

    // 图层表格维护
    void addLayerToTables(ObjectId layerId, const std::string& name);
    void removeLayerFromTables(ObjectId layerId, const std::string& name);
};

} // namespace tch
