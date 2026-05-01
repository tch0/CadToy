// 对应头文件
#include "DbEntity.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "Database.h"
#include "IGraphicsDataCache.h"


namespace tch {

void DbEntity::setLayerId(ObjectId id) {
    if (m_layerId == id) {
        return;
    }

    // 只有在数据库中且已分配ID时才触发moveEntityToLayer
    if (m_pDb && m_id != 0) {
        ObjectId actualLayerId = m_pDb->moveEntityToLayer(m_id, id);
        m_layerId = actualLayerId;
        notifyModified();
    } else {
        // 不在数据库中，直接设置
        m_layerId = id;
    }
}

// 包围盒默认使用缓存顶点计算，但无限实体XLine/Ray不适用，需要派生重写
Geometry::AABB DbEntity::boundingBox() const {
    if (!m_bboxDirty) { return m_cachedBBox; }

    // 尝试从图形缓存计算（离散化顶点，适合选择）
    // getEntityCacheData 会自动处理脏缓存的重生成
    if (m_pDb) {
        IGraphicsDataCache* pDataCache = m_pDb->getGraphicsDataCache();
        if (pDataCache) {
            const auto& entCache = pDataCache->getEntityCacheData(m_id);
            if (entCache.type != EntityGraphicsCacheData::kInvalidEmptyData && !entCache.vertices.empty()) {
                m_cachedBBox = Geometry::AABB(entCache.vertices[0].position, entCache.vertices[0].position);
                for (const auto& v : entCache.vertices) {
                    m_cachedBBox.expand(v.position);
                }
                m_bboxDirty = false;
                return m_cachedBBox;
            }
        }
    }

    // 如果没有图形数据则回退到解析公式计算
    m_cachedBBox = computeBoundingBox();
    m_bboxDirty = false;
    return m_cachedBBox;
}

void DbEntity::notifyModified() {
    m_bboxDirty = true;
    if (m_pDb && m_id != 0) {
        m_pDb->onEntityModified(m_id);
    }
}

// 实体是否完全位于给定的轴对齐包围盒内
// 默认通用实现使用图形缓存顶点判定，派生类中可以覆写为更简单的解析判定
// 基类判定中完全通用，且默认处理线型，后续如果实现了线型又影响选择的话那么删掉相关子类的覆写接口即可
bool DbEntity::intersects(const Geometry::AABB& rect) const {
    if (!m_pDb) { return false; }
    auto* pCache = m_pDb->getGraphicsDataCache();
    if (!pCache) { return false; }

    const auto& cacheData = pCache->getEntityCacheData(m_id);
    const auto& verts = cacheData.vertices;
    if (verts.empty()) { return false; }

    for (size_t i = 0; i + 1 < verts.size(); ++i) {
        if (rect.intersectsSegment(verts[i].position, verts[i + 1].position)) { return true; }
    }
    return false;
}

// 实体是否与给定轴对齐包围盒相交
// 默认通用实现使用图形缓存顶点判定，派生类可以覆写为更简单的解析判定
// 基类判定中完全通用，且默认处理线型，后续如果实现了线型又影响选择的话那么删掉相关子类的覆写接口即可
bool DbEntity::isInside(const Geometry::AABB& rect) const {
    if (!m_pDb) { return false; }
    auto* pCache = m_pDb->getGraphicsDataCache();
    if (!pCache) { return false; }

    const auto& cacheData = pCache->getEntityCacheData(m_id);
    const auto& verts = cacheData.vertices;
    if (verts.empty()) { return false; }

    for (const auto& v : verts) {
        if (!rect.contains(v.position)) { return false; }
    }
    return true;
}

void DbEntity::setColor(const DbColor& color) {
    m_color = color;
    notifyModified();
}

void DbEntity::setLinetype(const DbLinetypeRef& lt) {
    m_linetype = lt;
    notifyModified();
}

void DbEntity::setLinetypeScale(double scale) {
    m_linetypeScale = scale;
    notifyModified();
}

void DbEntity::setLineWeight(DbLineWeight lw) {
    m_lineWeight = lw;
    notifyModified();
}

void DbEntity::setVisible(bool visible) {
    m_visible = visible;
    notifyModified();
}

void DbEntity::setPropertiesFromDb() {
    if (!m_pDb) {
        return;
    }

    // 设置图层为当前图层
    m_layerId = m_pDb->currentLayerId();

    // 设置颜色为数据库当前实体颜色
    m_color = m_pDb->currentEntityColor();

    // 设置线型比例
    m_linetypeScale = m_pDb->currentEntityLinetypeScale();

    // 设置线宽
    m_lineWeight = m_pDb->currentEntityLineWeight();

    // 设置线型
    m_linetype = m_pDb->currentEntityLinetype();
}

void DbEntity::writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    DbObject::writeFields(writer);
    
    writer.Key("layerId");
    DbJsonUtils::writeUint64(writer, m_layerId);
    
    writer.Key("color");
    DbJsonUtils::writeColor(writer, m_color);
    
    writer.Key("linetype");
    DbJsonUtils::writeLinetype(writer, m_linetype);
    
    writer.Key("linetypeScale");
    DbJsonUtils::writeDouble(writer, m_linetypeScale);
    
    writer.Key("lineWeight");
    DbJsonUtils::writeLineWeight(writer, m_lineWeight);
    
    writer.Key("visible");
    DbJsonUtils::writeBool(writer, m_visible);
}

bool DbEntity::readFields(const rapidjson::Value& value) {
    if (!DbObject::readFields(value)) {
        return false;
    }
    
    if (value.HasMember("layerId")) {
        DbJsonUtils::readUint64(value["layerId"], m_layerId);
    }
    
    if (value.HasMember("color")) {
        DbJsonUtils::readColor(value["color"], m_color);
    }
    
    if (value.HasMember("linetype")) {
        DbJsonUtils::readLinetype(value["linetype"], m_linetype);
    }
    
    if (value.HasMember("linetypeScale")) {
        DbJsonUtils::readDouble(value["linetypeScale"], m_linetypeScale);
    }
    
    if (value.HasMember("lineWeight")) {
        DbJsonUtils::readLineWeight(value["lineWeight"], m_lineWeight);
    }
    
    if (value.HasMember("visible")) {
        DbJsonUtils::readBool(value["visible"], m_visible);
    }
    
    return true;
}

} // namespace tch
