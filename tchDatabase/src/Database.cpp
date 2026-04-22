// 项目头文件
#include "Database.h"

// 项目头文件
#include "DbLayer.h"
#include "DbObjectFactory.h"
#include "IGraphicsDataCache.h"


namespace tch {

Database::Database() {
    initDefaultSysVars();

    // 创建默认"0"图层，并设为当前图层
    ObjectId layer0Id = ensureLayerZero();
    setCurrentLayerId(layer0Id);
}

Database::~Database() = default;

// ============================================================================
// 对象管理
// ============================================================================

ObjectId Database::addObject(std::unique_ptr<DbObject> obj) {
    if (!obj) {
        return 0;
    }

    ObjectId id = allocateId(obj->type());
    obj->setId(id);
    obj->setDatabase(this);
    m_objects[id] = std::move(obj);
    return id;
}

DbObject* Database::getObject(ObjectId id) const {
    if (id == 0 || isBackupId(id)) {
        return nullptr;
    }
    auto it = m_objects.find(id);
    if (it != m_objects.end() && it->second) {
        return it->second.get();
    }
    return nullptr;
}

bool Database::hasObject(ObjectId id) const {
    if (id == 0 || isBackupId(id)) {
        return false;
    }
    auto it = m_objects.find(id);
    if (it != m_objects.end()) {
        return it->second != nullptr;
    }
    return false;
}

void Database::removeObject(ObjectId id) {
    if (id == 0 || isBackupId(id)) {
        return;
    }

    auto it = m_objects.find(id);
    if (it == m_objects.end()) {
        return;
    }

    // 修改 ID 为备份区 ID
    ObjectId backupId = getBackupId(id);
    it->second->setId(backupId);

    // 移动到备份区
    m_backupObjects[backupId] = std::move(it->second);
    m_objects.erase(it);
}

void Database::eraseObject(ObjectId id) {
    if (id == 0) {
        return;
    }

    // 可以从正常区或备份区删除
    if (isBackupId(id)) {
        // 从备份区删除
        m_backupObjects.erase(id);
    } else {
        // 从正常区删除
        m_objects.erase(id);
    }
}

// ============================================================================
// Undo/Redo 支持
// ============================================================================

void Database::backupForModify(ObjectId id) {
    if (id == 0 || isBackupId(id)) {
        return;
    }

    auto it = m_objects.find(id);
    if (it == m_objects.end() || !it->second) {
        return;
    }

    // 克隆对象到备份区，使用偏移ID
    ObjectId backupId = getBackupId(id);
    std::unique_ptr<DbObject> backup = it->second->clone();
    if (backup) {
        backup->setId(backupId);
        m_backupObjects[backupId] = std::move(backup);
    }
}

void Database::restoreFromBackup(ObjectId id) {
    if (id == 0 || isBackupId(id)) {
        return;
    }

    ObjectId backupId = getBackupId(id);
    auto it = m_backupObjects.find(backupId);
    if (it == m_backupObjects.end() || !it->second) {
        return;
    }

    // 还原 ID
    it->second->setId(id);

    // 移回正常区
    m_objects[id] = std::move(it->second);
    m_backupObjects.erase(it);
}

void Database::swapWithBackup(ObjectId id) {
    if (id == 0 || isBackupId(id)) {
        return;
    }

    // 获取对象和备份
    auto objIt = m_objects.find(id);
    if (objIt == m_objects.end() || !objIt->second) {
        return;
    }

    ObjectId backupId = getBackupId(id);
    auto backupIt = m_backupObjects.find(backupId);
    if (backupIt == m_backupObjects.end() || !backupIt->second) {
        return;
    }

    // 交换 ID
    objIt->second->setId(backupId);
    backupIt->second->setId(id);

    // 交换指针
    std::swap(objIt->second, backupIt->second);
}

DbObject* Database::getBackup(ObjectId backupId) const {
    auto it = m_backupObjects.find(backupId);
    if (it != m_backupObjects.end() && it->second) {
        return it->second.get();
    }
    return nullptr;
}

// ============================================================================
// 图层管理
// ============================================================================

ObjectId Database::addLayer(const std::string& name) {
    if (name.empty()) {
        return 0;
    }

    // 检查是否已存在
    if (m_layerNameMap.count(name)) {
        return m_layerNameMap[name];
    }

    // 创建新图层
    auto layer = std::make_unique<DbLayer>();
    layer->setName(name);

    ObjectId id = addObject(std::move(layer));
    if (id != 0) {
        m_layerIds.push_back(id);
        m_layerNameMap[name] = id;
    }

    return id;
}

DbLayer* Database::getLayer(ObjectId id) const {
    DbObject* obj = getObject(id);
    if (obj && obj->type() == DbObject::kLayer) {
        return static_cast<DbLayer*>(obj);
    }
    return nullptr;
}

DbLayer* Database::getLayerByName(const std::string& name) const {
    auto it = m_layerNameMap.find(name);
    if (it != m_layerNameMap.end()) {
        return getLayer(it->second);
    }
    return nullptr;
}

bool Database::removeLayer(ObjectId id) {
    DbLayer* layer = getLayer(id);
    if (!layer) {
        return false;
    }

    // 禁止删除当前图层
    if (currentLayerId() == id) {
        return false;
    }

    // TODO: 检查图层上是否有实体，有则返回失败

    // 从名称映射中移除
    m_layerNameMap.erase(layer->name());

    // 从图层列表中移除
    auto it = std::find(m_layerIds.begin(), m_layerIds.end(), id);
    if (it != m_layerIds.end()) {
        m_layerIds.erase(it);
    }

    // 移动到备份区
    removeObject(id);
    return true;
}

ObjectId Database::currentLayerId() const {
    auto it = m_sysVars.find(SysVar::kCLayer);
    if (it != m_sysVars.end()) {
        return static_cast<ObjectId>(it->second.asInt());
    }
    return 0;
}

void Database::setCurrentLayerId(ObjectId id) {
    // 检查 ID 是否有效且是图层
    if (id != 0 && !getLayer(id)) {
        return;
    }
    m_sysVars[SysVar::kCLayer] = SysVarValue::fromInt(static_cast<int>(id));
}

DbLayer* Database::currentLayer() const {
    return getLayer(currentLayerId());
}

// ============================================================================
// 系统变量
// ============================================================================

void Database::setSysVar(SysVar var, const SysVarValue& value) {
    m_sysVars[var] = value;
}

SysVarValue Database::getSysVar(SysVar var) const {
    auto it = m_sysVars.find(var);
    if (it != m_sysVars.end()) {
        return it->second;
    }

    // 如果没有找到返回默认值
    SysVarValue defaultValue;
    if (var == SysVar::kLwDefault) {
        defaultValue = SysVarValue::fromInt(static_cast<int>(DbLineWeight::k000));
    } else if (var == SysVar::kLtScale) {
        defaultValue = SysVarValue::fromDouble(1.0);
    } else {
        defaultValue = SysVarValue::fromInt(0);
    }

    const_cast<Database*>(this)->m_sysVars[var] = defaultValue;
    return defaultValue;
}

DbLineWeight Database::defaultLineWeight() const {
    return static_cast<DbLineWeight>(getSysVar(SysVar::kLwDefault).asInt());
}

double Database::linetypeScale() const {
    return getSysVar(SysVar::kLtScale).asDouble();
}

// ============================================================================
// 遍历和查询
// ============================================================================

void Database::forEachObject(std::function<void(DbObject*)> callback) const {
    for (const auto& [id, obj] : m_objects) {
        if (obj) {
            callback(obj.get());
        }
    }
}

void Database::forEachInBackup(std::function<void(DbObject*)> callback) const {
    for (const auto& [id, obj] : m_backupObjects) {
        if (obj && isBackupId(id)) {
            callback(obj.get());
        }
    }
}

// ============================================================================
// 序列化
// ============================================================================

void Database::saveToJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    writer.StartObject();

    // 版本号
    writer.Key("version");
    writer.String("1.0");

    // 系统变量
    writer.Key("variables");
    writer.StartObject();
    writer.Key("LWDEFAULT");
    writer.Int(getSysVar(SysVar::kLwDefault).asInt());
    writer.Key("LTSCALE");
    writer.Double(getSysVar(SysVar::kLtScale).asDouble());
    writer.Key("CLAYER");
    writer.Uint64(currentLayerId());
    writer.EndObject();

    // 图层列表
    writer.Key("layers");
    writer.StartArray();
    for (ObjectId layerId : m_layerIds) {
        DbObject* obj = getObject(layerId);
        if (obj) {
            obj->saveToJson(writer);
        }
    }
    writer.EndArray();

    // 实体列表
    writer.Key("entities");
    writer.StartArray();
    for (const auto& [id, obj] : m_objects) {
        if (obj && !isBackupId(id) && obj->type() != DbObject::kLayer) {
            obj->saveToJson(writer);
        }
    }
    writer.EndArray();

    writer.EndObject();
}

bool Database::loadFromJson(const rapidjson::Value& value) {
    if (!value.IsObject()) {
        return false;
    }

    // 清空现有数据
    m_objects.clear();
    m_backupObjects.clear();
    m_layerIds.clear();
    m_layerNameMap.clear();
    m_sysVars.clear();
    initDefaultSysVars();

    // 读取系统变量
    if (value.HasMember("variables") && value["variables"].IsObject()) {
        const auto& vars = value["variables"];

        // LWDEFAULT - 必须是整数
        if (vars.HasMember("LWDEFAULT") && vars["LWDEFAULT"].IsInt()) {
            setSysVar(SysVar::kLwDefault, SysVarValue::fromInt(vars["LWDEFAULT"].GetInt()));
        }

        // LTSCALE - 可以是数值
        if (vars.HasMember("LTSCALE") && vars["LTSCALE"].IsNumber()) {
            setSysVar(SysVar::kLtScale, SysVarValue::fromDouble(vars["LTSCALE"].GetDouble()));
        }

        // CLAYER - 必须是 uint64
        if (vars.HasMember("CLAYER") && vars["CLAYER"].IsUint64()) {
            setCurrentLayerId(vars["CLAYER"].GetUint64());
        }
    }

    // 读取图层
    if (value.HasMember("layers") && value["layers"].IsArray()) {
        for (const auto& layerValue : value["layers"].GetArray()) {
            std::unique_ptr<DbObject> obj = DbObjectFactory::getInstance().createFromJson(layerValue);
            if (obj && obj->type() == DbObject::kLayer) {
                ObjectId id = obj->id();
                if (id == 0) {
                    id = allocateId(DbObject::kLayer);
                    obj->setId(id);
                }
                obj->setDatabase(this);
                m_objects[id] = std::move(obj);
                m_layerIds.push_back(id);

                // 更新名称映射
                DbLayer* layer = getLayer(id);
                if (layer) {
                    m_layerNameMap[layer->name()] = id;
                }
            }
        }
    }

    // 读取实体
    if (value.HasMember("entities") && value["entities"].IsArray()) {
        for (const auto& entityValue : value["entities"].GetArray()) {
            std::unique_ptr<DbObject> obj = DbObjectFactory::getInstance().createFromJson(entityValue);
            if (obj) {
                ObjectId id = obj->id();
                if (id == 0) {
                    id = allocateId(obj->type());
                    obj->setId(id);
                }
                obj->setDatabase(this);
                m_objects[id] = std::move(obj);
            }
        }
    }

    // 加载完成后，检查 CLAYER 有效性
    ObjectId currentId = currentLayerId();
    if (currentId == 0 || !getLayer(currentId)) {
        // CLAYER 无效，确保"0"图层存在并设为当前
        ObjectId layer0Id = ensureLayerZero();
        setCurrentLayerId(layer0Id);
    }

    return true;
}

// ============================================================================
// Purge 支持
// ============================================================================

void Database::purge() {
    // 清理所有备份实体
    m_backupObjects.clear();
}

// ============================================================================
// 通知接口实现
// ============================================================================

void Database::onEntityModified(ObjectId id) {
    if (m_pGraphicsCache) {
        m_pGraphicsCache->onEntityModified(id);
    }
}

void Database::onLayerModified(ObjectId id) {
    if (m_pGraphicsCache) {
        m_pGraphicsCache->onEntityModified(id);
    }
}

// ============================================================================
// 私有方法
// ============================================================================

ObjectId Database::allocateId(DbObject::Type type) {
    // 根据类型分配 ID
    switch (type) {
        case DbObject::kLayer:
            // 图层属于符号表
            if (m_nextSymbolId <= kSymbolEnd) {
                return m_nextSymbolId++;
            }
            break;

        case DbObject::kLine:
        case DbObject::kCircle:
        case DbObject::kArc:
        case DbObject::kEllipse:
        case DbObject::kXLine:
        case DbObject::kRay:
            // 实体
            return m_nextEntityId++;

        default:
            break;
    }

    // 默认使用实体 ID
    return m_nextEntityId++;
}

void Database::initDefaultSysVars() {
    m_sysVars[SysVar::kLwDefault] = SysVarValue::fromInt(static_cast<int>(DbLineWeight::k000));
    m_sysVars[SysVar::kLtScale] = SysVarValue::fromDouble(1.0);
    m_sysVars[SysVar::kCLayer] = SysVarValue::fromInt(0);  // 0 表示无效ID
}

ObjectId Database::ensureLayerZero() {
    // 检查是否已存在"0"图层
    DbLayer* layer0 = getLayerByName("0");
    if (layer0) {
        return layer0->id();
    }

    // 创建"0"图层
    auto newLayer = std::make_unique<DbLayer>();
    newLayer->setName("0");
    newLayer->setColor(DbColor::White);
    newLayer->setLinetype(DbLinetypeRef::continuous());
    newLayer->setLineWeight(DbLineWeight::kByLwDefault);

    ObjectId id = allocateId(DbObject::kLayer);
    newLayer->setId(id);
    m_objects[id] = std::move(newLayer);
    m_layerIds.push_back(id);
    m_layerNameMap["0"] = id;

    return id;
}

} // namespace tch
