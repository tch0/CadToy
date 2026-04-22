#pragma once

// C++ 标准库
#include <cstdint>

// 第三方库

// 项目头文件
#include "IGraphicsEngine.h"


namespace tch {

// 前置声明
class Database;
class DbEntity;
class DbLine;
class DbCircle;
class DbArc;
class DbEllipse;
class DbXLine;
class DbRay;
class IGraphicsDataCache;
struct EntityGraphicsCacheData;
struct DataCacheVertex;

// =========================================================================================================================
// 图形引擎实现类
// 职责：无状态算法库，根据数据库实体生成图形缓存数据
// =========================================================================================================================
class GraphicsEngine : public IGraphicsEngine {
public:
    GraphicsEngine() = default;
    ~GraphicsEngine() override = default;

    // 禁止拷贝
    GraphicsEngine(const GraphicsEngine&) = delete;
    GraphicsEngine& operator=(const GraphicsEngine&) = delete;

    // ========================================================================
    // IGraphicsEngine 接口实现
    // ========================================================================
    
    // 生成数据缓存，为缓存中所有脏实体生成缓存
    void generate(IGraphicsDataCache* pDataCache) const override;

private:
    // ========================================================================
    // 实体生成辅助方法
    // ========================================================================
    
    // 为单个实体生成缓存数据
    EntityGraphicsCacheData generateEntityCache(const DbEntity* pEntity, Database* pDb) const;
    
    // 各种实体类型的生成方法（flags由generateEntityCache统一计算传入）
    EntityGraphicsCacheData generateLineCache(const DbLine* pLine, Database* pDb, uint32_t flags) const;
    EntityGraphicsCacheData generateCircleCache(const DbCircle* pCircle, Database* pDb, uint32_t flags) const;
    EntityGraphicsCacheData generateArcCache(const DbArc* pArc, Database* pDb, uint32_t flags) const;
    EntityGraphicsCacheData generateEllipseCache(const DbEllipse* pEllipse, Database* pDb, uint32_t flags) const;
    EntityGraphicsCacheData generateXLineCache(const DbXLine* pXLine, Database* pDb, uint32_t flags) const;
    EntityGraphicsCacheData generateRayCache(const DbRay* pRay, Database* pDb, uint32_t flags) const;
    
    // ========================================================================
    // 工具方法
    // ========================================================================
    
    // 获取实体实际颜色（解析ByLayer等）
    glm::vec3 resolveEntityColor(const DbEntity* pEntity, Database* pDb) const;
    
    // 获取实体实际线宽（解析ByLayer等，返回像素值）
    float resolveEntityLineWidth(const DbEntity* pEntity, Database* pDb) const;
    
    // 确定缓存数据类型（基于线宽和可见性）
    EntityGraphicsCacheData::Type determineCacheType(float lineWidth, bool visible) const;

    // 圆/圆弧细分参数
    static constexpr int kCircleSegments = 64;      // 完整圆细分数
    static constexpr int kMinArcSegments = 8;       // 圆弧最少细分数
    static constexpr float kMaxLineWidthPixels = 100.0f;  // 最大线宽像素值
    static constexpr float kDefaultLineWidthPixels = 1.0f; // 默认线宽像素值
};

} // namespace tch
