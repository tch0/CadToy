#pragma once

// C++ 标准库
#include <cstdint>
#include <vector>
#include <functional>

// 第三方库
#include <glm/glm.hpp>

// 项目头文件


namespace tch {

// ================================================================================================
// 顶点数据结构
// 用于实体渲染的顶点格式，包含位置、颜色、状态标志和线宽，用于线框渲染和三角面渲染
// ================================================================================================
struct DataCacheVertex {
    glm::vec3 position;     // 位置（世界坐标）
    glm::vec3 color;        // 基础颜色 (RGB)
    uint32_t flags;         // 状态标志位：bit0=预选高亮, bit1=选中高亮, bit2=暗显
    float lineWidth;        // 线宽值（屏幕像素为单位），线宽LineWeight应该需要经过换算才能得到这个像素线宽
};


// ================================================================================================
// 实体的图形缓存数据
// 包括数据类型（是否有线宽，据此来选择渲染器），以及顶点数组，后续可以在这里添加其他需要的数据
// ================================================================================================
struct EntityGraphicsCacheData {
    enum Type {
        kInvalidEmtpyData,              // 无效空数据，获取无效ID则会返回这个
        kAlwaysNoLineWidth,             // 没有线宽，无论LwDisplay是否打开，线宽总是显示为1像素的情况，总是调用无线宽版本
        kLineWidthDependsOnLwDisplay,   // 实体原始线宽大于1像素的情况，但是否显示出来取决于Lwdisplay系统变量，开启时使用线宽版本渲染器，关闭时使用无线宽版本
        kAlwaysShowLineWidth,           // 总是显示为有线宽的图元，比如预选高亮就一定有宽度，总是调用有线宽版本渲染器
        kInvisibleEntity,               // 不可见实体，渲染时将直接跳过
        kPreviewCacheData,              // 暂未使用，预览数据类型，
    };
    Type type = kInvalidEmtpyData;
    std::vector<DataCacheVertex> vertices;
};



// 前置声明
class Database;
// 实体ID类型（可根据实际定义调整）
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
    // 标记所有实体为脏，以便全量重生成，提供给命令层regen、初始化时以及某些会影响显示的系统变量修改等需要全量重生成的场景使用
    virtual void markAllDirty() = 0;
    
    // ============================================================================
    // 通知接口：提供给数据库通知实体变化情况，其中会标记实体为脏，以实现部分重生成机制
    // 实体已添加，需要为实体添加并生成缓存数据
    virtual void onEntityAdded(ObjectId id) = 0;
    // 实体已修改，几何或属性变化，需重新生成缓存数据
    virtual void onEntityModified(ObjectId id) = 0;
    // 实体已删除，清理该实体的缓存数据
    virtual void onEntityRemoved(ObjectId id) = 0;
    
    // ============================================================================
    // 缓存数据查询接口：提供给渲染器渲染
    // 获取所有实体ID
    virtual std::vector<ObjectId> getAllEntityIds() const = 0;
    // 通过ID查询读取缓存数据
    virtual const EntityGraphicsCacheData& getEntityCacheData(ObjectId id) const = 0;
    // 提供更通用的遍历接口，方便渲染器渲染，相比查询ID再根据ID去依次查询性能会更好，回调参数为实体id和实体缓存数据
    virtual void iterateAllCacheData(const std::function<void(ObjectId id, const EntityGraphicsCacheData& cacheData)>& func) = 0;
};

} // namespace tch
