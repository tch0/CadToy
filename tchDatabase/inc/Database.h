#pragma once

// C++ 标准库
#include <memory>
#include <unordered_map>
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
class DbLayer;
class IGraphicsDataCache;

// ============================================================================
// Database - 数据库核心类
// ============================================================================

class Database {
public:
    // ID 分区常量
    static constexpr ObjectId kSystemStart = 1;
    static constexpr ObjectId kSystemEnd = 999;
    static constexpr ObjectId kSymbolStart = 1000;
    static constexpr ObjectId kSymbolEnd = 9999;
    static constexpr ObjectId kEntityStart = 10000;
    static constexpr ObjectId kBackupOffset = 0x8000000000000000ULL;

    Database();
    ~Database();

    // 禁用拷贝
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // ========================================================================
    // 对象管理（命令层使用）
    // ========================================================================

    // 添加对象，自动分配 ID
    ObjectId addObject(std::unique_ptr<DbObject> obj);

    // 获取对象（不包括备份区的）
    DbObject* getObject(ObjectId id) const;

    // 检查对象是否存在（不包括备份区的）
    bool hasObject(ObjectId id) const;

    // 移除对象（移动到备份区而不实际删除）
    void removeObject(ObjectId id);

    // 永久删除对象
    void eraseObject(ObjectId id);

    // ========================================================================
    // ID 管理
    // ========================================================================

    // 检查 ID 是否在备份区
    static bool isBackupId(ObjectId id) { return id >= kBackupOffset; }

    // 获取原始 ID（从备份区 ID 还原）
    static ObjectId getOriginalId(ObjectId backupId) { return backupId - kBackupOffset; }

    // 获取备份区 ID
    static ObjectId getBackupId(ObjectId originalId) { return originalId + kBackupOffset; }

    // ========================================================================
    // Undo/Redo 支持（UndoManager 使用）
    // ========================================================================

    // 创建修改备份（克隆对象到备份区，使用偏移ID）
    void backupForModify(ObjectId id);

    // 从备份区恢复对象（还原 ID）
    void restoreFromBackup(ObjectId id);

    // 交换对象和备份（用于修改的 undo/redo）
    void swapWithBackup(ObjectId id);

    // 获取备份对象
    DbObject* getBackup(ObjectId backupId) const;

    // ========================================================================
    // 图层管理
    // ========================================================================

    // 添加图层，返回图层 ID
    ObjectId addLayer(const std::string& name);

    // 获取图层
    DbLayer* getLayer(ObjectId id) const;

    // 根据名称获取图层
    DbLayer* getLayerByName(const std::string& name) const;

    // 删除图层，返回是否成功
    // TODO: 需要检查图层上是否有实体，有则返回失败并提供相关信息
    bool removeLayer(ObjectId id);

    // 获取所有图层 ID
    const std::vector<ObjectId>& layerIds() const { return m_layerIds; }

    // 获取/设置当前图层 ID
    ObjectId currentLayerId() const;
    void setCurrentLayerId(ObjectId id);

    // 获取当前图层
    DbLayer* currentLayer() const;

    // ========================================================================
    // 系统变量
    // ========================================================================

    // 设置系统变量
    void setSysVar(SysVar var, const SysVarValue& value);

    // 获取系统变量
    SysVarValue getSysVar(SysVar var) const;

    // 获取默认线宽
    DbLineWeight defaultLineWeight() const;

    // 获取线型比例
    double linetypeScale() const;

    // ========================================================================
    // 遍历和查询
    // ========================================================================

    // 遍历所有对象（不包括备份区的）
    void forEachObject(std::function<void(DbObject*)> callback) const;

    // 遍历所有备份区的对象
    void forEachInBackup(std::function<void(DbObject*)> callback) const;

    // 获取对象数量
    size_t objectCount() const { return m_objects.size(); }

    // 获取备份数量
    size_t backupCount() const { return m_backupObjects.size(); }

    // ========================================================================
    // 序列化
    // ========================================================================

    void saveToJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
    bool loadFromJson(const rapidjson::Value& value);

    // ========================================================================
    // Purge 支持
    // ========================================================================

    // 清理冗余备份实体
    void purge();

    // ========================================================================
    // 图形数据缓存关联
    // ========================================================================

    // 设置图形数据缓存指针（文档构造时调用）
    void setGraphicsDataCache(IGraphicsDataCache* pCache) { m_pGraphicsCache = pCache; }

    // 获取图形数据缓存指针
    IGraphicsDataCache* getGraphicsDataCache() const { return m_pGraphicsCache; }

    // ========================================================================
    // 通知接口（供 DbObject 调用）
    // ========================================================================

    // 实体被修改
    void onEntityModified(ObjectId id);

    // 图层被修改
    void onLayerModified(ObjectId id);

private:
    // 对象存储
    std::unordered_map<ObjectId, std::unique_ptr<DbObject>> m_objects;

    // 备份对象存储（用于 undo/redo）
    std::unordered_map<ObjectId, std::unique_ptr<DbObject>> m_backupObjects;

    // 图层列表（按添加顺序）
    std::vector<ObjectId> m_layerIds;

    // 图层名称到 ID 的映射
    std::unordered_map<std::string, ObjectId> m_layerNameMap;

    // 系统变量表
    std::unordered_map<SysVar, SysVarValue> m_sysVars;

    // 下一个可用 ID
    ObjectId m_nextSystemId = kSystemStart;
    ObjectId m_nextSymbolId = kSymbolStart;
    ObjectId m_nextEntityId = kEntityStart;

    // 图形数据缓存指针（不参与序列化）
    IGraphicsDataCache* m_pGraphicsCache = nullptr;

    // 分配 ID
    ObjectId allocateId(DbObject::Type type);

    // 初始化默认系统变量
    void initDefaultSysVars();

    // 确保有"0"图层，返回其ID
    ObjectId ensureLayerZero();
};

} // namespace tch
