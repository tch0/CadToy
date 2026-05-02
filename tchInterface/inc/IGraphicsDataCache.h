#pragma once

// C++ 标准库
#include <cstdint>
#include <vector>
#include <functional>

// 第三方库
#include <glm/glm.hpp>

// 项目头文件
#include "Geometry.h"


namespace tch {

// ================================================================================================
// 顶点数据结构
// 用于实体渲染的顶点格式，包含位置、颜色、状态标志和线宽，用于线框渲染和三角面渲染
// ================================================================================================
struct DataCacheVertex {
    glm::vec3 position;     // 位置（世界坐标）
    glm::vec3 color;        // 基础颜色 (RGB)
    uint32_t flags;         // 状态标志位
    float lineWidth;        // 线宽值（屏幕像素为单位），线宽LineWeight应该需要经过换算才能得到这个像素线宽

    // 标志位定义
    static constexpr uint32_t kFlagPreSelected   = 1 << 0;      // bit0: 预选高亮，临时状态，缓存负责处理
    static constexpr uint32_t kFlagSelected      = 1 << 1;      // bit1: 选中高亮，临时状态，缓存负责处理
    static constexpr uint32_t kFlagTempDimmed    = 1 << 2;      // bit2: 命令层临时暗显，临时状态，缓存负责处理
    static constexpr uint32_t kFlagLockedLayerDimmed = 1 << 3;  // bit3: 图层锁定暗显，实体基础显示属性，图形引擎负责处理

    // 所有临时标志的掩码，用于清除操作（不包含持久的图层锁定标志）
    static constexpr uint32_t kAllTempFlags = kFlagPreSelected | kFlagSelected | kFlagTempDimmed;
};


// ================================================================================================
// 实体的图形缓存数据
// 包括数据类型（指示如何显示线宽，据此来选择渲染器），以及顶点数组，后续可以在这里添加其他需要的数据
// 注意：不再包含是否预选中、选中等临时状态字段，临时状态由缓存统一管理
// ================================================================================================
struct EntityGraphicsCacheData {
    enum Type {
        kInvalidEmptyData,              // 无效空数据，获取无效ID则会返回这个
        kAlwaysNoLineWidth,             // 没有线宽，无论LwDisplay是否打开，线宽总是显示为1像素的情况，总是调用无线宽版本
        kLineWidthDependsOnLwDisplay,   // 实体原始线宽大于1像素的情况，但是否显示出来取决于Lwdisplay系统变量，开启时使用线宽版本渲染器，关闭时使用无线宽版本
        kAlwaysShowLineWidth,           // 总是显示为有线宽的图元，比如预选高亮就一定有宽度，总是调用有线宽版本渲染器
        kInvisibleEntity,               // 不可见实体，渲染时将直接跳过
    };
    Type type = kInvalidEmptyData;      // 实体渲染类型
    std::vector<DataCacheVertex> vertices;  // 顶点
};



// 前置声明
class Database;
// 实体ID类型，多处都有定义，必须保持一致
using ObjectId = uint64_t;

// ================================================================================================
// 图形缓存接口
// 职责：关联到唯一的数据库实例，管理数据库中每个实体的CPU端缓存顶点数据，维护脏实体集合，接收来自数据库的通知。
// ================================================================================================
class IGraphicsDataCache {
public:
    virtual ~IGraphicsDataCache() = default;
    
    // ============================================================================
    // 查询与写入接口：提供给图形引擎查询相关信息
    // ============================================================================
    // 数据库访问
    virtual Database* getDatabase() const = 0;
    // 获取当前所有脏实体ID列表
    virtual std::vector<ObjectId> getDirtyEntities() const = 0;
    // 设置指定实体的缓存数据（移动语义）
    virtual void setEntityCacheData(ObjectId id, EntityGraphicsCacheData&& cacheData) = 0;
    // 移除实体缓存数据
    virtual void removeEntityCacheData(ObjectId id) = 0;
    // 清除指定实体的脏标记（引擎处理完该实体后调用）
    virtual void clearDirty(ObjectId id) = 0;
    // 全量重生成标记，提供给命令层regen、初始化时以及某些可能影响全局显示的系统变量修改等场景使用
    virtual void markAllDirty() = 0;
    // 检查是否需要全量重新生成
    virtual bool needsRegenAll() const = 0;
    // 清除全量重新生成标记
    virtual void clearAllDirty() = 0;
    // 清除所有缓存数据（全量重新生成前调用）
    virtual void clearAllCacheData() = 0;
    // 判断某实体顶点缓存是否脏
    virtual bool isCacheDirty(ObjectId id) const = 0;

    // ============================================================================
    // 视口管理（无限实体会在 updateViewport 中被标记为脏）
    // ============================================================================
    virtual void updateViewport(const Geometry::AABB& newViewport) = 0;
    virtual const Geometry::AABB& getCurrentViewport() const = 0;

    // ============================================================================
    // 通知接口：提供给数据库通知实体变化情况，其中会标记实体为脏，以实现部分重生成机制
    // ============================================================================
    virtual void onEntityAdded(ObjectId id) = 0;
    virtual void onEntityModified(ObjectId id) = 0;
    virtual void onEntityRemoved(ObjectId id) = 0;

    // ============================================================================
    // 缓存数据查询接口：提供给渲染器渲染（返回修饰后的最终数据）
    // ============================================================================
    virtual std::vector<ObjectId> getAllEntityIds() const = 0;
    virtual const EntityGraphicsCacheData& getEntityCacheData(ObjectId id) = 0;
    virtual void iterateAllCacheData(const std::function<void(ObjectId id, const EntityGraphicsCacheData& cacheData)>& func) = 0;

    // ============================================================================
    // 临时状态通知接口（统一操作 activeStates）
    // ============================================================================
    virtual void onEntityPreSelected(ObjectId id) = 0;
    virtual void onEntityUnPreSelected(ObjectId id) = 0;
    virtual void onEntitySelected(ObjectId id) = 0;
    virtual void onEntityUnSelected(ObjectId id) = 0;
    virtual void onEntityTempDimmed(ObjectId id) = 0;      // 实体临时暗显
    virtual void onEntityUnTempDimmed(ObjectId id) = 0;    // 取消实体临时暗显

    // ============================================================================
    // 全量重生成与状态管理
    // ============================================================================
    virtual void prepareForRegenAll() = 0;   // 清空基础几何，保留 activeStates，标记全部脏
    virtual void resetAllStates() = 0;       // 彻底清空所有临时状态（如新建文档）
};

} // namespace tch
