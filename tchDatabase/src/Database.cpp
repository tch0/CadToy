// 对应头文件
#include "Database.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "DbEntity.h"
#include "DbLayer.h"
#include "DbObjectFactory.h"
#include "IGraphicsDataCache.h"


namespace tch {

Database::Database() {
    // 创建默认"0"图层，并设为当前图层
    ObjectId layer0Id = ensureLayerZero();
    m_currentLayerId = layer0Id;
}

Database::~Database() = default;

// ======================================================================================================
// 对象、实体管理
// ======================================================================================================

ObjectId Database::addObject(std::unique_ptr<DbObject> pObj) {
    if (!pObj) {
        return 0;
    }

    // 根据类型分发
    if (pObj->isType(DbObject::kEntity)) {
        return addEntity(std::move(pObj));
    }
    else if (pObj->isType(DbObject::kLayer)) {
        return addLayer(std::move(pObj));
    }

    // 暂时不支持的类型，返回无效ID
    return 0;
}

ObjectId Database::addEntity(std::unique_ptr<DbObject> pObj) {
    // 转换为实体指针，失败则返回0
    DbEntity* pEntity = pObj->as<DbEntity>();
    if (!pEntity) {
        return 0;
    }

    ObjectId id = allocateId(pObj->type());
    pObj->setId(id);

    // 维护图层索引
    ObjectId layerId = pEntity->layerId();
    // 如果图层无效或不存在，设置为当前图层
    if (layerId == 0 || !getLayer(layerId)) {
        layerId = currentLayerId();
        pEntity->setLayerId(layerId);  // 此时m_pDb为nullptr，不会触发moveEntityToLayer
    }
    addEntityToIndex(id, layerId);

    pObj->setDatabase(this);
    m_objects[id] = std::move(pObj);
    
    // 标记为脏并通知添加了实体
    m_dirty = true;
    if (m_pGraphicsCache) {
        m_pGraphicsCache->onEntityAdded(id);
    }
    
    return id;
}

// 对象查询
DbObject* Database::getObject(ObjectId id) const {
    if (id == 0) {
        return nullptr;
    }
    auto it = m_objects.find(id);
    if (it != m_objects.end() && it->second) {
        return it->second.get();
    }
    return nullptr;
}

// 实体查询
DbEntity* Database::getEntity(ObjectId id) const {
    DbObject* pObj = getObject(id);
    return pObj ? pObj->as<DbEntity>() : nullptr;
}

// 遍历所有实体
void Database::iterateEntities(const std::function<void(DbEntity*)>& func) const {
    if (!func) {
        return;
    }
    
    for (const auto& [_, pObj] : m_objects) {
        if (pObj && pObj->isType(DbObject::kEntity)) {
            func(pObj->as<DbEntity>());
        }
    }
}

bool Database::hasObject(ObjectId id) const {
    if (id == 0) {
        return false;
    }
    auto it = m_objects.find(id);
    if (it != m_objects.end()) {
        return it->second != nullptr;
    }
    return false;
}

bool Database::removeObject(ObjectId id) {
    if (id == 0) {
        return false;
    }

    DbObject* pObj = getObject(id);
    if (!pObj) {
        return false;
    }

    // 根据类型分发
    if (pObj->isType(DbObject::kEntity)) {
        return removeEntity(id);
    }
    else if (pObj->isType(DbObject::kLayer)) {
        return removeLayer(id);
    }

    return false;
}

bool Database::removeEntity(ObjectId id) {
    if (id == 0) {
        return false;
    }

    auto it = m_objects.find(id);
    if (it == m_objects.end()) {
        return false;
    }

    // 必须是实体类型
    if (!it->second->isType(DbObject::kEntity)) {
        return false;
    }

    // 从图层索引中移除
    if (DbEntity* pEntity = it->second->as<DbEntity>()) {
        removeEntityFromIndex(id, pEntity->layerId());
    }

    // 清除数据库指针
    it->second->setDatabase(nullptr);

    // 移动到备份区（保持原始ID）
    m_backupObjects[id] = std::move(it->second);
    m_objects.erase(it);
    
    // 标记脏并通知删除了实体
    m_dirty = true;
    if (m_pGraphicsCache) {
        m_pGraphicsCache->onEntityRemoved(id);
    }

    return true;
}

void Database::eraseObject(ObjectId id) {
    if (id == 0) {
        return;
    }

    // 先尝试从正常区删除
    auto it = m_objects.find(id);
    if (it != m_objects.end()) {
        // 根据类型维护表格并通知缓存
        // 实体
        if (DbEntity* pEntity = it->second->as<DbEntity>()) {
            removeEntityFromIndex(id, pEntity->layerId());
            m_objects.erase(it);
            // 通知删除了实体
            if (m_pGraphicsCache) {
                m_pGraphicsCache->onEntityRemoved(id);
            }
        }
        // 图层
        else if (DbLayer* pLayer = it->second->as<DbLayer>()) {
            removeLayerFromTables(id, pLayer->name());
            m_objects.erase(it);
        }
        
        // 标记为脏
        m_dirty = true;
        return;
    }

    // 再尝试从备份区删除（备份区不需要维护表格）
    auto backupIt = m_backupObjects.find(id);
    if (backupIt != m_backupObjects.end()) {
        m_backupObjects.erase(backupIt);
    }
}

// ======================================================================================================
// Undo/Redo 支持
// ======================================================================================================

void Database::backupForModify(ObjectId id) {
    if (id == 0) {
        return;
    }

    auto it = m_objects.find(id);
    if (it == m_objects.end() || !it->second) {
        return;
    }

    // 克隆对象到备份区（保持原始ID）
    std::unique_ptr<DbObject> backup = it->second->clone();
    if (backup) {
        backup->setId(id);
        m_backupObjects[id] = std::move(backup);
    }
}

void Database::restoreFromBackup(ObjectId id) {
    if (id == 0) {
        return;
    }

    auto it = m_backupObjects.find(id);
    if (it == m_backupObjects.end() || !it->second) {
        return;
    }

    // 设置数据库指针
    it->second->setDatabase(this);

    // 根据类型维护表格并通知缓存
    // 实体
    if (DbEntity* pEntity = it->second->as<DbEntity>()) {
        addEntityToIndex(id, pEntity->layerId());
        // 通知添加了实体（只有实体需要通知）
        if (m_pGraphicsCache) {
            m_pGraphicsCache->onEntityAdded(id);
        }
    }
    // 图层
    else if (DbLayer* pLayer = it->second->as<DbLayer>()) {
        addLayerToTables(id, pLayer->name());
    }

    // 移回正常区（保持原始ID）
    m_objects[id] = std::move(it->second);
    m_backupObjects.erase(it);
    
    // 标记为脏
    m_dirty = true;
}

void Database::swapWithBackup(ObjectId id) {
    if (id == 0) {
        return;
    }

    // 获取对象和备份
    auto objIt = m_objects.find(id);
    if (objIt == m_objects.end() || !objIt->second) {
        return;
    }

    auto backupIt = m_backupObjects.find(id);
    if (backupIt == m_backupObjects.end() || !backupIt->second) {
        return;
    }

    // 交换前：从表格中移除当前对象的信息
    // 实体
    if (DbEntity* pEntity = objIt->second->as<DbEntity>()) {
        removeEntityFromIndex(id, pEntity->layerId());
    }
    // 图层
    else if (DbLayer* pLayer = objIt->second->as<DbLayer>()) {
        removeLayerFromTables(id, pLayer->name());
    }

    // 交换指针（ID保持不变）
    std::swap(objIt->second, backupIt->second);

    // 更新数据库指针
    objIt->second->setDatabase(this);       // 正常区对象设置指针
    backupIt->second->setDatabase(nullptr); // 备份区对象清除指针

    // 交换后：添加新对象的信息到表格并通知缓存
    // 实体
    if (DbEntity* pEntity = objIt->second->as<DbEntity>()) {
        addEntityToIndex(id, pEntity->layerId());
        // 通知修改了实体（只有实体需要通知）
        if (m_pGraphicsCache) {
            m_pGraphicsCache->onEntityModified(id);
        }
    }
    // 图层
    else if (DbLayer* pLayer = objIt->second->as<DbLayer>()) {
        addLayerToTables(id, pLayer->name());
    }
    
    // 标记为脏
    m_dirty = true;
}

DbObject* Database::getBackup(ObjectId id) const {
    auto it = m_backupObjects.find(id);
    if (it != m_backupObjects.end() && it->second) {
        return it->second.get();
    }
    return nullptr;
}

// ======================================================================================================
// 图层管理
// ======================================================================================================

ObjectId Database::addLayer(std::unique_ptr<DbObject> pObj) {
    // 转换为图层指针，失败则返回0
    DbLayer* pLayer = pObj->as<DbLayer>();
    if (!pLayer) {
        return 0;
    }

    // 检查图层名称是否已存在
    if (layerExists(pLayer->name())) {
        return 0;  // 名称已存在，添加失败
    }

    ObjectId id = allocateId(pObj->type());
    pObj->setId(id);

    // 维护图层列表和名称映射
    addLayerToTables(id, pLayer->name());

    pObj->setDatabase(this);
    m_objects[id] = std::move(pObj);
    
    // 标记脏（空图层不影响显示，不需要通知缓存）
    m_dirty = true;
    
    return id;
}

ObjectId Database::addLayer(const std::string& name) {
    // 检查是否已存在，存在返回0，无效id
    auto it = m_layerNameMap.find(name);
    if (it != m_layerNameMap.end()) {
        return 0;
    }

    // 创建新图层
    std::unique_ptr<DbLayer> newLayer = std::make_unique<DbLayer>();
    newLayer->setName(name);

    ObjectId id = allocateId(DbObject::kLayer);
    newLayer->setId(id);
    newLayer->setDatabase(this);
    m_objects[id] = std::move(newLayer);
    addLayerToTables(id, name);
    
    // 标记脏（空图层不影响显示，不需要通知缓存）
    m_dirty = true;

    return id;
}

DbLayer* Database::getLayer(ObjectId id) const {
    DbObject* pObj = getObject(id);
    return pObj ? pObj->as<DbLayer>() : nullptr;
}

DbLayer* Database::getLayerByName(const std::string& name) const {
    auto it = m_layerNameMap.find(name);
    if (it != m_layerNameMap.end()) {
        return getLayer(it->second);
    }
    return nullptr;
}

bool Database::layerExists(const std::string& name) const {
    return m_layerNameMap.find(name) != m_layerNameMap.end();
}

bool Database::removeLayer(ObjectId id) {
    auto it = m_objects.find(id);
    if (it == m_objects.end()) {
        return false;
    }

    // 必须是图层类型
    if (!it->second->isType(DbObject::kLayer)) {
        return false;
    }

    // 禁止删除当前图层
    if (currentLayerId() == id) {
        return false;
    }

    // 检查图层上是否有实体，有则返回失败
    auto indexIt = m_layerEntityIndex.find(id);
    if (indexIt != m_layerEntityIndex.end() && !indexIt->second.empty()) {
        return false;
    }

    // 从图层表格中移除
    if (DbLayer* pLayer = it->second->as<DbLayer>()) {
        removeLayerFromTables(id, pLayer->name());
    }

    // 清除数据库指针并移动到备份区
    it->second->setDatabase(nullptr);
    m_backupObjects[id] = std::move(it->second);
    m_objects.erase(it);
    
    // 标记为脏（图层上无实体，删除不影响显示，不需要通知缓存）
    m_dirty = true;

    return true;
}

void Database::setCurrentLayerId(ObjectId id) {
    // 检查 ID 是否有效且是图层
    if (id != 0 && !getLayer(id)) {
        return;
    }
    m_currentLayerId = id;
    m_dirty = true;
    if (m_pGraphicsCache) {
        // 修改当前图层也不需要重生成
        // m_pGraphicsCache->markAllDirty();
    }
}

DbLayer* Database::currentLayer() const {
    return getLayer(m_currentLayerId);
}

void Database::setDefaultLineWeight(DbLineWeight lw) {
    m_defaultLineWeight = lw;
    m_dirty = true;
    if (m_pGraphicsCache) {
        // 默认线宽修改可能影响所有线宽值为默认的实体，需要通知重生成
        m_pGraphicsCache->markAllDirty();
    }
}

void Database::setLinetypeScale(double scale) {
    m_linetypeScale = scale;
    m_dirty = true;
    if (m_pGraphicsCache) {
        // 全局线性比例因子，影响显示，需要通知重生成
        m_pGraphicsCache->markAllDirty();
    }
}

void Database::setCurrentEntityLinetypeScale(double scale) {
    m_currentEntityLinetypeScale = scale;
    m_dirty = true;
    // 只影响接下来创建实体的线型比例，对已存在实体无影响，无需重生成
}

void Database::setLineWeightDisplay(bool display) {
    m_lineWeightDisplay = display;
    m_dirty = true;
    // 线宽值始终生成在顶点中数据中，渲染器根据LWDISPLAY选择渲染器，修改不需要重生成
}

void Database::setCurrentEntityColor(const DbColor& color) {
    m_currentEntityColor = color;
    m_dirty = true;
    // 只影响后续创建的实体，不通知缓存重生成
}

void Database::setCurrentEntityLinetype(const DbLinetypeRef& lt) {
    m_currentEntityLinetype = lt;
    m_dirty = true;
    // 只影响后续创建的实体，不通知缓存重生成
}

void Database::setCurrentEntityLineWeight(DbLineWeight lw) {
    m_currentEntityLineWeight = lw;
    m_dirty = true;
    // 只影响后续创建的实体，不通知缓存重生成
}

// 移动实体到指定图层，返回实际设置的图层ID（如果目标图层不存在则移动到当前图层并返回当前图层ID）
// 提供给实体setLayerId调用，以正确维护索引表，不再其他任何地方调用
ObjectId Database::moveEntityToLayer(ObjectId entityId, ObjectId targetLayerId) {
    DbObject* pObj = getObject(entityId);
    if (!pObj) {
        return 0;
    }

    DbEntity* pEntity = pObj->as<DbEntity>();
    if (!pEntity) {
        return 0;  // 不是实体
    }

    ObjectId currentLayerId = pEntity->layerId();

    // 检查目标图层是否存在
    if (targetLayerId == 0 || !getLayer(targetLayerId)) {
        // 目标图层不存在，返回当前图层ID作为fallback
        return currentLayerId;
    }

    // 维护图层索引
    moveEntityInIndex(entityId, currentLayerId, targetLayerId);
    
    // 标记脏并通知实体修改
    m_dirty = true;
    if (m_pGraphicsCache) {
        m_pGraphicsCache->onEntityModified(entityId);
    }

    return targetLayerId;
}

const std::unordered_set<ObjectId>& Database::getEntitiesOnLayer(ObjectId layerId) const {
    static const std::unordered_set<ObjectId> kEmptyEntitySet;
    auto it = m_layerEntityIndex.find(layerId);
    if (it != m_layerEntityIndex.end()) {
        return it->second;
    }
    return kEmptyEntitySet;
}

// ======================================================================================================
// 遍历和查询
// ======================================================================================================

void Database::forEachObject(const std::function<void(DbObject*)>& callback) const {
    for (const auto& [id, pObj] : m_objects) {
        if (pObj) {
            callback(pObj.get());
        }
    }
}

void Database::forEachInBackup(const std::function<void(DbObject*)>& callback) const {
    for (const auto& [id, pObj] : m_backupObjects) {
        if (pObj) {
            callback(pObj.get());
        }
    }
}

// ======================================================================================================
// 序列化
// ======================================================================================================

void Database::saveToJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) {
    writer.StartObject();

    // 版本号
    writer.Key("version");
    writer.Uint(1);

    // 文档属性
    writer.Key("sysvars");
    writer.StartObject();
    
    // 保存 LWDEFAULT
    writer.Key("LWDEFAULT");
    writer.Int(static_cast<int>(m_defaultLineWeight));
    
    // 保存 LTSCALE
    writer.Key("LTSCALE");
    writer.Double(m_linetypeScale);
    
    // 保存 CELTSCALE
    writer.Key("CELTSCALE");
    writer.Double(m_currentEntityLinetypeScale);
    
    // 保存 CLAYER
    writer.Key("CLAYER");
    writer.Uint64(m_currentLayerId);
    
    // 保存 LWDISPLAY
    writer.Key("LWDISPLAY");
    writer.Bool(m_lineWeightDisplay);
    
    // 保存 CECOLOR
    writer.Key("CECOLOR");
    DbJsonUtils::writeColor(writer, m_currentEntityColor);

    // 保存 CELTYPE
    writer.Key("CELTYPE");
    DbJsonUtils::writeLinetype(writer, m_currentEntityLinetype);

    // 保存 CELWEIGHT
    writer.Key("CELWEIGHT");
    writer.Int(static_cast<int>(m_currentEntityLineWeight));
    
    writer.EndObject();

    // 图层列表
    writer.Key("layers");
    writer.StartArray();
    for (ObjectId layerId : m_layerIds) {
        DbObject* pObj = getObject(layerId);
        if (pObj) {
            pObj->saveToJson(writer);
        }
    }
    writer.EndArray();

    // 实体列表
    writer.Key("entities");
    writer.StartArray();
    for (const auto& [id, pObj] : m_objects) {
        if (pObj && pObj->isType(DbObject::kEntity)) {
            pObj->saveToJson(writer);
        }
    }
    writer.EndArray();

    writer.EndObject();

    // 保存成功后清除脏标记
    clearDirty();
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
    m_layerEntityIndex.clear();
    // 重置文档属性为默认值
    m_defaultLineWeight = DbLineWeight::k000;
    m_linetypeScale = 1.0;
    m_currentEntityLinetypeScale = 1.0;
    m_currentLayerId = 0;
    m_lineWeightDisplay = false;
    m_currentEntityColor = DbColor::byLayer();
    m_currentEntityLinetype = DbLinetypeRef::byLayer();
    m_currentEntityLineWeight = DbLineWeight::kByLayer;

    // 读取文档属性
    if (value.HasMember("sysvars") && value["sysvars"].IsObject()) {
        const auto& vars = value["sysvars"];

        // LWDEFAULT - 必须是整数
        if (vars.HasMember("LWDEFAULT") && vars["LWDEFAULT"].IsInt()) {
            m_defaultLineWeight = static_cast<DbLineWeight>(vars["LWDEFAULT"].GetInt());
        }

        // LTSCALE - 可以是数值
        if (vars.HasMember("LTSCALE") && vars["LTSCALE"].IsNumber()) {
            m_linetypeScale = vars["LTSCALE"].GetDouble();
        }

        // CELTSCALE - 可以是数值
        if (vars.HasMember("CELTSCALE") && vars["CELTSCALE"].IsNumber()) {
            m_currentEntityLinetypeScale = vars["CELTSCALE"].GetDouble();
        }

        // CLAYER - 必须是 uint64
        if (vars.HasMember("CLAYER") && vars["CLAYER"].IsUint64()) {
            m_currentLayerId = vars["CLAYER"].GetUint64();
        }

        // LWDISPLAY - 可以是布尔值或整数
        if (vars.HasMember("LWDISPLAY")) {
            if (vars["LWDISPLAY"].IsBool()) {
                m_lineWeightDisplay = vars["LWDISPLAY"].GetBool();
            } else if (vars["LWDISPLAY"].IsInt()) {
                m_lineWeightDisplay = vars["LWDISPLAY"].GetInt() != 0;
            }
        }
        
        // CECOLOR - 颜色
        if (vars.HasMember("CECOLOR")) {
            DbJsonUtils::readColor(vars["CECOLOR"], m_currentEntityColor);
        }
        
        // CELTYPE - 线型引用
        if (vars.HasMember("CELTYPE")) {
            DbJsonUtils::readLinetype(vars["CELTYPE"], m_currentEntityLinetype);
        }

        // CELWEIGHT - 整数
        if (vars.HasMember("CELWEIGHT") && vars["CELWEIGHT"].IsInt()) {
            m_currentEntityLineWeight = static_cast<DbLineWeight>(vars["CELWEIGHT"].GetInt());
        }
    }

    // 读取图层
    if (value.HasMember("layers") && value["layers"].IsArray()) {
        for (const auto& layerValue : value["layers"].GetArray()) {
            std::unique_ptr<DbObject> pObj = DbObjectFactory::getInstance().createFromJson(layerValue);
            if (pObj && pObj->isType(DbObject::kLayer)) {
                ObjectId id = pObj->id();
                if (id == 0) {
                    id = allocateId(DbObject::kLayer);
                    pObj->setId(id);
                }
                pObj->setDatabase(this);
                m_objects[id] = std::move(pObj);
                m_layerIds.push_back(id);

                // 更新名称映射
                DbLayer* pLayer = getLayer(id);
                if (pLayer) {
                    m_layerNameMap[pLayer->name()] = id;
                }
            }
        }
    }

    // 读取实体
    if (value.HasMember("entities") && value["entities"].IsArray()) {
        for (const auto& entityValue : value["entities"].GetArray()) {
            std::unique_ptr<DbObject> pObj = DbObjectFactory::getInstance().createFromJson(entityValue);
            if (pObj) {
                ObjectId id = pObj->id();
                if (id == 0) {
                    id = allocateId(pObj->type());
                    pObj->setId(id);
                }
                
                // 维护图层索引（在setDatabase之前，此时m_pDb为nullptr，不会触发moveEntityToLayer）
                if (DbEntity* pEnt = pObj->as<DbEntity>()) {
                    ObjectId layerId = pEnt->layerId();
                    // 验证图层有效性
                    if (layerId == 0 || !getLayer(layerId)) {
                        layerId = currentLayerId();
                        pEnt->setLayerId(layerId);
                    }
                    m_layerEntityIndex[layerId].insert(id);
                }
                
                pObj->setDatabase(this);
                m_objects[id] = std::move(pObj);
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

    // 加载完成后设置为未修改状态
    clearDirty();
    // 加载完成后需要标记全量重生成
    if (m_pGraphicsCache) {
        m_pGraphicsCache->markAllDirty();
    }

    return true;
}

// ======================================================================================================
// Purge 支持
// ======================================================================================================

void Database::purge() {
    // 清理所有备份实体
    m_backupObjects.clear();
}

// ======================================================================================================
// 通知接口实现
// ======================================================================================================

void Database::onEntityModified(ObjectId id) {
    m_dirty = true;
    if (m_pGraphicsCache) {
        m_pGraphicsCache->onEntityModified(id);
    }
}

void Database::onLayerModified(ObjectId id) {
    m_dirty = true;
    // 获取该图层上的所有实体，逐个通知
    const auto& entities = getEntitiesOnLayer(id);
    if (m_pGraphicsCache) {
        for (ObjectId entityId : entities) {
            m_pGraphicsCache->onEntityModified(entityId);
        }
    }
}

// ======================================================================================================
// 私有方法
// ======================================================================================================

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

ObjectId Database::ensureLayerZero() {
    // 检查是否已存在"0"图层
    DbLayer* pLayer0 = getLayerByName("0");
    if (pLayer0) {
        return pLayer0->id();
    }

    // 创建"0"图层
    auto newLayer = std::make_unique<DbLayer>();
    newLayer->setName("0");
    newLayer->setColor(DbColor::White);
    newLayer->setLinetype(DbLinetypeRef::continuous());
    newLayer->setLineWeight(DbLineWeight::kByLwDefault);

    ObjectId id = allocateId(DbObject::kLayer);
    newLayer->setId(id);
    newLayer->setDatabase(this);
    m_objects[id] = std::move(newLayer);
    addLayerToTables(id, "0");

    return id;
}

// ======================================================================================================
// 表格维护辅助函数
// ======================================================================================================

void Database::addEntityToIndex(ObjectId entityId, ObjectId layerId) {
    if (entityId == 0 || layerId == 0) {
        return;
    }
    m_layerEntityIndex[layerId].insert(entityId);
}

void Database::removeEntityFromIndex(ObjectId entityId, ObjectId layerId) {
    if (entityId == 0 || layerId == 0) {
        return;
    }
    auto it = m_layerEntityIndex.find(layerId);
    if (it != m_layerEntityIndex.end()) {
        it->second.erase(entityId);
        if (it->second.empty()) {
            m_layerEntityIndex.erase(it);
        }
    }
}

void Database::moveEntityInIndex(ObjectId entityId, ObjectId oldLayerId, ObjectId newLayerId) {
    if (entityId == 0 || oldLayerId == newLayerId) {
        return;
    }
    removeEntityFromIndex(entityId, oldLayerId);
    addEntityToIndex(entityId, newLayerId);
}

void Database::addLayerToTables(ObjectId layerId, const std::string& name) {
    if (layerId == 0) {
        return;
    }
    m_layerIds.push_back(layerId);
    if (!name.empty()) {
        m_layerNameMap[name] = layerId;
    }
}

void Database::removeLayerFromTables(ObjectId layerId, const std::string& name) {
    if (layerId == 0) {
        return;
    }
    // 从名称映射中移除
    if (!name.empty()) {
        m_layerNameMap.erase(name);
    }
    // 从图层列表中移除
    auto it = std::find(m_layerIds.begin(), m_layerIds.end(), layerId);
    if (it != m_layerIds.end()) {
        m_layerIds.erase(it);
    }
    // 从实体索引中移除（该图层上的实体）
    m_layerEntityIndex.erase(layerId);
}

} // namespace tch
