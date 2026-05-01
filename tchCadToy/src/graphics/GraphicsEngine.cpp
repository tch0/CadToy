// 对应头文件
#include "GraphicsEngine.h"

// C++ 标准库
#include <cmath>
#include <algorithm>

// 第三方库

// 项目头文件
#include "GraphicsDataCache.h"
#include "IGraphicsDataCache.h"
#include "Database.h"
#include "DbEntity.h"
#include "DbLayer.h"
#include "DbLine.h"
#include "DbCircle.h"
#include "DbArc.h"
#include "DbEllipse.h"
#include "DbXLine.h"
#include "DbRay.h"
#include "Geometry.h"


namespace tch {

// ============================================================================
// 单例实现
// ============================================================================

GraphicsEngine& GraphicsEngine::getInstance() {
    static GraphicsEngine instance;
    return instance;
}

// ============================================================================
// 主生成方法
// ============================================================================

void GraphicsEngine::generate(IGraphicsDataCache* pDataCache) const {
    if (!pDataCache) {
        return;
    }
    
    // 获取数据库
    Database* pDb = pDataCache->getDatabase();
    if (!pDb) {
        return;
    }
    
    // 获取当前视口
    const Geometry::AABB& viewport = pDataCache->getCurrentViewport();
    
    // 检查是否需要全量重新生成
    if (pDataCache->needsRegenAll()) {
        // 清除所有旧缓存数据
        pDataCache->clearAllCacheData();
        
        // 遍历数据库所有实体生成缓存
        pDb->forEachEntity([&](DbEntity* pEntity) {
            if (!pEntity) {
                return;
            }
            
            ObjectId id = pEntity->id();
            EntityGraphicsCacheData cacheData = generateEntityCache(pEntity, pDb, viewport);
            pDataCache->setEntityCacheData(id, std::move(cacheData));
        });
        
        // 清除全量重新生成标记
        pDataCache->clearAllDirty();
        return;
    }
    
    // 生成无限实体，内部调用图形引擎接口生成几何数据后，由数据缓存内部处理状态保持
    pDataCache->generateInfiniteEntities();

    // 生成其他普通脏实体
    for (ObjectId id : pDataCache->getDirtyEntities()) {
        // 获取实体
        DbEntity* pEntity = pDb->getEntity(id);
        if (!pEntity) {
            continue;
        }
        
        // 生成缓存数据
        EntityGraphicsCacheData cacheData = generateEntityCache(pEntity, pDb, viewport);
        
        // 存储缓存数据
        pDataCache->setEntityCacheData(id, std::move(cacheData));
        
        // 清除脏标记
        pDataCache->clearDirty(id);
    }
}

// ============================================================================
// 单个实体生成
// ============================================================================

// 单独为指定实体生成图形缓存数据
void GraphicsEngine::generateForEntity(IGraphicsDataCache* pDataCache, ObjectId id) const {
    if (!pDataCache || id == 0) {
        return;
    }

    // 获取数据库
    Database* pDb = pDataCache->getDatabase();
    if (!pDb) {
        return;
    }

    // 获取实体
    DbEntity* pEntity = pDb->getEntity(id);
    if (!pEntity) {
        return;
    }

    // 获取当前视口
    const Geometry::AABB& viewport = pDataCache->getCurrentViewport();

    // 生成缓存数据
    EntityGraphicsCacheData cacheData = generateEntityCache(pEntity, pDb, viewport);

    // 存储缓存数据
    pDataCache->setEntityCacheData(id, std::move(cacheData));

    // 清除脏标记
    pDataCache->clearDirty(id);
}

// ============================================================================
// 实体生成分发
// ============================================================================

EntityGraphicsCacheData GraphicsEngine::generateEntityCache(const DbEntity* pEntity, Database* pDb, const Geometry::AABB& viewport) const {
    static const EntityGraphicsCacheData emptyData {false, false, EntityGraphicsCacheData::kInvisibleEntity, {}};
    if (!pEntity || !pDb) {
        return emptyData;
    }

    // 检查实体自身可见性
    if (!pEntity->isVisible()) {
        return emptyData;
    }

    // 检查图层状态（冻结或锁定）
    uint32_t flags = 0;
    ObjectId layerId = pEntity->layerId();
    if (layerId != 0) {
        DbLayer* pLayer = pDb->getLayer(layerId);
        if (pLayer) {
            // 图层冻结，实体不可见
            if (pLayer->isFrozen()) {
                return emptyData;
            }
            // 图层锁定，实体需要暗显
            if (pLayer->isLocked()) {
                flags = DataCacheVertex::kFlagDimmed;
            }
        }
    }

    // 根据实体类型分发到对应的生成方法，传入已计算的flags
    switch (pEntity->type()) {
        case DbObject::kLine:
            return generateLineCache(pEntity->as<DbLine>(), pDb, flags);
        case DbObject::kCircle:
            return generateCircleCache(pEntity->as<DbCircle>(), pDb, flags);
        case DbObject::kArc:
            return generateArcCache(pEntity->as<DbArc>(), pDb, flags);
        case DbObject::kEllipse:
            return generateEllipseCache(pEntity->as<DbEllipse>(), pDb, flags);
        case DbObject::kXLine:
            return generateXLineCache(pEntity->as<DbXLine>(), pDb, flags, viewport);
        case DbObject::kRay:
            return generateRayCache(pEntity->as<DbRay>(), pDb, flags, viewport);
        default:
            return emptyData;
    }
}

// ============================================================================
// 各种实体类型的生成方法
// ============================================================================

EntityGraphicsCacheData GraphicsEngine::generateLineCache(const DbLine* pLine, Database* pDb, uint32_t flags) const {
    EntityGraphicsCacheData cacheData;
    
    if (!pLine || !pDb) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }
    
    // 获取颜色和线宽
    glm::vec3 color = resolveEntityColor(pLine, pDb);
    float lineWidth = resolveEntityLineWidth(pLine, pDb);
    
    // 确定缓存类型
    cacheData.type = determineCacheType(lineWidth, true);
    
    // 如果不可见，直接返回
    if (cacheData.type == EntityGraphicsCacheData::kInvisibleEntity) {
        return cacheData;
    }
    
    // 获取线段端点
    const Geometry::Point& start = pLine->start();
    const Geometry::Point& end = pLine->end();
    
    // 创建两个顶点
    cacheData.vertices.push_back({start, color, flags, lineWidth});
    cacheData.vertices.push_back({end, color, flags, lineWidth});
    
    return cacheData;
}

EntityGraphicsCacheData GraphicsEngine::generateCircleCache(const DbCircle* pCircle, Database* pDb, uint32_t flags) const {
    EntityGraphicsCacheData cacheData;
    
    if (!pCircle || !pDb) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }
    
    // 获取颜色和线宽
    glm::vec3 color = resolveEntityColor(pCircle, pDb);
    float lineWidth = resolveEntityLineWidth(pCircle, pDb);
    
    // 确定缓存类型
    cacheData.type = determineCacheType(lineWidth, true);
    
    // 如果不可见，直接返回
    if (cacheData.type == EntityGraphicsCacheData::kInvisibleEntity) {
        return cacheData;
    }
    
    // 获取圆心和半径
    const Geometry::Point& center = pCircle->center();
    double radius = pCircle->radius();
    
    // 半径为0，不可见
    if (radius <= 0.0) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }
    
    // 细分圆为线段
    // 计算细分数量：基于半径动态调整，确保视觉平滑
    int segments = kCircleSegments;
    
    // 生成顶点（GL_LINES格式：每段线段2个顶点）
    // 第一个点(i=0)和最后一个点(i=segments)各添加1次，中间点添加2次
    // 圆：i=segments时angle=2π，与i=0重合，自动闭合
    cacheData.vertices.reserve(segments * 2);
    
    for (int i = 0; i <= segments; ++i) {
        double angle = (2.0 * Geometry::PI * i) / segments;
        Geometry::Point pos;
        pos.x = center.x + radius * std::cos(angle);
        pos.y = center.y + radius * std::sin(angle);
        pos.z = center.z;
        
        if (i == 0 || i == segments) {
            // 第一个点和最后一个点：各添加1次
            cacheData.vertices.push_back({pos, color, flags, lineWidth});
        } else {
            // 中间点：添加2次
            cacheData.vertices.push_back({pos, color, flags, lineWidth});
            cacheData.vertices.push_back({pos, color, flags, lineWidth});
        }
    }
    // 圆自动闭合：i=segments时angle=2π与i=0时angle=0重合
    
    return cacheData;
}

EntityGraphicsCacheData GraphicsEngine::generateArcCache(const DbArc* pArc, Database* pDb, uint32_t flags) const {
    EntityGraphicsCacheData cacheData;
    
    if (!pArc || !pDb) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }
    
    // 获取颜色和线宽
    glm::vec3 color = resolveEntityColor(pArc, pDb);
    float lineWidth = resolveEntityLineWidth(pArc, pDb);
    
    // 确定缓存类型
    cacheData.type = determineCacheType(lineWidth, true);
    
    // 如果不可见，直接返回
    if (cacheData.type == EntityGraphicsCacheData::kInvisibleEntity) {
        return cacheData;
    }
    
    // 获取圆弧参数
    const Geometry::Point& center = pArc->center();
    double radius = pArc->radius();
    double startAngle = pArc->startAngle();
    double endAngle = pArc->endAngle();
    
    // 半径为0，不可见
    if (radius <= 0.0) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }
    
    // 计算角度跨度
    double angleSpan = endAngle - startAngle;
    if (angleSpan < 0.0) {
        angleSpan += 2.0 * Geometry::PI;
    }
    
    // 计算细分数量
    int segments = std::max(kMinArcSegments,
                           static_cast<int>((angleSpan / (2.0 * Geometry::PI)) * kCircleSegments));
    
    // 生成顶点（GL_LINES格式：每段线段2个顶点）
    // 顶点数 = 2 * segments（偶数）
    cacheData.vertices.reserve(segments * 2);
    
    for (int i = 0; i <= segments; ++i) {
        double t = i * 1.0 / segments;
        double angle = startAngle + angleSpan * t;
        
        Geometry::Point pos;
        pos.x = center.x + radius * std::cos(angle);
        pos.y = center.y + radius * std::sin(angle);
        pos.z = center.z;
        
        if (i == 0 || i == segments) {
            // 第一个点和最后一个点：各添加1次
            cacheData.vertices.push_back({pos, color, flags, lineWidth});
        } else {
            // 中间点：添加2次
            cacheData.vertices.push_back({pos, color, flags, lineWidth});
            cacheData.vertices.push_back({pos, color, flags, lineWidth});
        }
    }
    
    return cacheData;
}

EntityGraphicsCacheData GraphicsEngine::generateEllipseCache(const DbEllipse* pEllipse, Database* pDb, uint32_t flags) const {
    EntityGraphicsCacheData cacheData;
    
    if (!pEllipse || !pDb) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }
    
    // 获取颜色和线宽
    glm::vec3 color = resolveEntityColor(pEllipse, pDb);
    float lineWidth = resolveEntityLineWidth(pEllipse, pDb);
    
    // 确定缓存类型
    cacheData.type = determineCacheType(lineWidth, true);
    
    // 如果不可见，直接返回
    if (cacheData.type == EntityGraphicsCacheData::kInvisibleEntity) {
        return cacheData;
    }
    
    // 获取椭圆参数
    const Geometry::Point& center = pEllipse->center();
    double radiusX = pEllipse->radiusX();
    double radiusY = pEllipse->radiusY();
    double rotation = pEllipse->rotation();
    double startParam = pEllipse->startParam();
    double endParam = pEllipse->endParam();
    
    // 半径为0，不可见
    if (radiusX <= 0.0 || radiusY <= 0.0) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }
    
    // 计算角度跨度
    double paramSpan = endParam - startParam;
    if (paramSpan < 0.0) {
        paramSpan += 2.0 * Geometry::PI;
    }
    
    // 计算周长近似（Ramanujan近似公式）
    double h = std::pow((radiusX - radiusY) / (radiusX + radiusY), 2);
    double circumference = Geometry::PI * (radiusX + radiusY) * (1.0 + 3.0 * h / (10.0 + std::sqrt(4.0 - 3.0 * h)));
    double arcLength = circumference * (paramSpan / (2.0 * Geometry::PI));
    
    // 根据弧长计算细分数量
    int segments = std::max(kMinArcSegments,
                           static_cast<int>(arcLength / (2.0 * Geometry::PI * 50.0) * kCircleSegments));
    
    // 预计算旋转角度
    double cosRot = std::cos(rotation);
    double sinRot = std::sin(rotation);
    
    // 生成顶点（GL_LINES格式：每段线段2个顶点）
    // 顶点数 = 2 * segments（偶数）
    cacheData.vertices.reserve(segments * 2);
    
    for (int i = 0; i <= segments; ++i) {
        double t = i * 1.0 / segments;
        double angle = startParam + paramSpan * t;
        
        // 椭圆参数方程（局部坐标）
        double localX = radiusX * std::cos(angle);
        double localY = radiusY * std::sin(angle);
        
        // 旋转到世界坐标
        Geometry::Point pos;
        pos.x = center.x + localX * cosRot - localY * sinRot;
        pos.y = center.y + localX * sinRot + localY * cosRot;
        pos.z = center.z;

        if (i == 0 || i == segments) {
            // 第一个点和最后一个点：各添加1次
            cacheData.vertices.push_back({pos, color, flags, lineWidth});
        } else {
            // 中间点：添加2次
            cacheData.vertices.push_back({pos, color, flags, lineWidth});
            cacheData.vertices.push_back({pos, color, flags, lineWidth});
        }
    }
    
    return cacheData;
}

EntityGraphicsCacheData GraphicsEngine::generateXLineCache(const DbXLine* pXLine, Database* pDb, uint32_t flags, const Geometry::AABB& viewport) const {
    EntityGraphicsCacheData cacheData;

    if (!pXLine || !pDb) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }

    // 获取颜色和线宽
    glm::vec3 color = resolveEntityColor(pXLine, pDb);
    float lineWidth = resolveEntityLineWidth(pXLine, pDb);

    // 确定缓存类型
    cacheData.type = determineCacheType(lineWidth, true);

    // 如果不可见，直接返回
    if (cacheData.type == EntityGraphicsCacheData::kInvisibleEntity) {
        return cacheData;
    }

    // 获取构造线参数
    const Geometry::Point& origin = pXLine->origin();
    const Geometry::Point& direction = pXLine->direction();
    
    // 使用 Slab 方法计算构造线与视口的交点
    const double tol = Geometry::Tolerance::Default.absolute;
    double tMin = -Geometry::INF, tMax = Geometry::INF;

    auto clip = [&](double p, double q) -> bool {
        if (std::abs(p) < tol) { return q >= 0; }
        double r = q / p;
        if (p < 0) {
            if (r > tMax) { return false; }
            if (r > tMin) { tMin = r; }
        } else {
            if (r < tMin) { return false; }
            if (r < tMax) { tMax = r; }
        }
        return true;
    };

    // X/Y/Z 维度裁剪
    if (!clip(-direction.x, origin.x - viewport.min.x)) { return cacheData; }
    if (!clip(direction.x, viewport.max.x - origin.x)) { return cacheData; }
    if (!clip(-direction.y, origin.y - viewport.min.y)) { return cacheData; }
    if (!clip(direction.y, viewport.max.y - origin.y)) { return cacheData; }
    if (!clip(-direction.z, origin.z - viewport.min.z)) { return cacheData; }
    if (!clip(direction.z, viewport.max.z - origin.z)) { return cacheData; }

    // 检查是否有有效相交区间
    if (tMin > tMax) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }
    // 计算裁剪后的起点和终点（tMin/tMax 已由 Slab 算法限制在视口范围内）
    Geometry::Point start = origin + direction * tMin;
    Geometry::Point end = origin + direction * tMax;
    
    cacheData.vertices.push_back({start, color, flags, lineWidth});
    cacheData.vertices.push_back({end, color, flags, lineWidth});
    
    return cacheData;
}

EntityGraphicsCacheData GraphicsEngine::generateRayCache(const DbRay* pRay, Database* pDb, uint32_t flags, const Geometry::AABB& viewport) const {
    EntityGraphicsCacheData cacheData;

    if (!pRay || !pDb) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }

    // 获取颜色和线宽
    glm::vec3 color = resolveEntityColor(pRay, pDb);
    float lineWidth = resolveEntityLineWidth(pRay, pDb);

    // 确定缓存类型
    cacheData.type = determineCacheType(lineWidth, true);

    // 如果不可见，直接返回
    if (cacheData.type == EntityGraphicsCacheData::kInvisibleEntity) {
        return cacheData;
    }

    // 获取射线参数
    const Geometry::Point& origin = pRay->origin();
    const Geometry::Point& direction = pRay->direction();

    // 使用 Slab 方法计算射线与视口的交点 (t 范围 [0, ∞))
    const double tol = Geometry::Tolerance::Default.absolute;
    double tMin = 0.0, tMax = Geometry::INF;

    auto clip = [&](double p, double q) -> bool {
        if (std::abs(p) < tol) { return q >= 0; }
        double r = q / p;
        if (p < 0) {
            if (r > tMax) { return false; }
            if (r > tMin) { tMin = r; }
        } else {
            if (r < tMin) { return false; }
            if (r < tMax) { tMax = r; }
        }
        return true;
    };

    // X/Y/Z 维度裁剪
    if (!clip(-direction.x, origin.x - viewport.min.x)) { return cacheData; }
    if (!clip(direction.x, viewport.max.x - origin.x)) { return cacheData; }
    if (!clip(-direction.y, origin.y - viewport.min.y)) { return cacheData; }
    if (!clip(direction.y, viewport.max.y - origin.y)) { return cacheData; }
    if (!clip(-direction.z, origin.z - viewport.min.z)) { return cacheData; }
    if (!clip(direction.z, viewport.max.z - origin.z)) { return cacheData; }

    // 检查是否有有效相交区间
    if (tMin > tMax) {
        cacheData.type = EntityGraphicsCacheData::kInvisibleEntity;
        return cacheData;
    }
    
    // 计算裁剪后的起点和终点（tMin/tMax 已由 Slab 算法限制在视口范围内）
    Geometry::Point start = origin + direction * tMin;
    Geometry::Point end = origin + direction * tMax;

    cacheData.vertices.push_back({start, color, flags, lineWidth});
    cacheData.vertices.push_back({end, color, flags, lineWidth});
    
    return cacheData;
}

// ============================================================================
// 工具方法
// ============================================================================

glm::vec3 GraphicsEngine::resolveEntityColor(const DbEntity* pEntity, Database* pDb) const {
    if (!pEntity) {
        return glm::vec3(1.0f, 1.0f, 1.0f);  // 默认白色
    }
    
    const DbColor& color = pEntity->color();
    
    if (color.isByLayer()) {
        // 获取图层颜色
        ObjectId layerId = pEntity->layerId();
        if (layerId != 0) {
            DbLayer* pLayer = pDb->getLayer(layerId);
            if (pLayer && pLayer->color().type() == DbColor::kRGB) {
                return pLayer->color().toVec3Color();
            }
        }
        // 默认白色
        return glm::vec3(1.0f, 1.0f, 1.0f);
    } else if (color.type() == DbColor::kRGB) {
        return color.toVec3Color();
    }
    
    // 默认白色
    return glm::vec3(1.0f, 1.0f, 1.0f);
}

float GraphicsEngine::resolveEntityLineWidth(const DbEntity* pEntity, Database* pDb) const {
    if (!pEntity || !pDb) {
        return kDefaultLineWidthPixels;
    }
    
    DbLineWeight lw = pEntity->lineWeight();
    
    if (lw == DbLineWeight::kByLayer) {
        // 获取图层线宽
        ObjectId layerId = pEntity->layerId();
        if (layerId != 0) {
            DbLayer* pLayer = pDb->getLayer(layerId);
            if (pLayer) {
                lw = pLayer->lineWeight();
            }
        }
    }
    
    if (lw == DbLineWeight::kByLwDefault) {
        // 使用默认线宽
        lw = pDb->defaultLineWeight();
    }
    
    // 将线宽枚举值转换为像素值
    // 线宽值是以1/100毫米为单位的
    int lwValue = static_cast<int>(lw);
    if (lwValue < 0) {
        return kDefaultLineWidthPixels;
    }
    
    // 转换为像素（简单映射：1/100毫米 = 0.01毫米 ≈ 0.038像素 at 96 DPI）
    // 这里使用简化的映射，实际应该考虑视图缩放
    float pixels = lwValue * 0.04f;
    
    // 限制最大线宽
    return std::min(pixels, kMaxLineWidthPixels);
}

EntityGraphicsCacheData::Type GraphicsEngine::determineCacheType(float lineWidth, bool visible) const {
    if (!visible) {
        return EntityGraphicsCacheData::kInvisibleEntity;
    }
    
    // 线宽小于等于1.5像素，使用无线宽渲染（避免1.5像素显示为1像素时的伪影）
    if (lineWidth <= 1.5f) {
        return EntityGraphicsCacheData::kAlwaysNoLineWidth;
    }
    
    // 线宽大于1像素，根据LwDisplay设置决定
    // 这里返回条件类型，实际渲染时根据系统变量决定
    return EntityGraphicsCacheData::kLineWidthDependsOnLwDisplay;
}

} // namespace tch
