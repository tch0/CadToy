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
    uint32_t flags;         // 状态标志位
    float lineWidth;        // 线宽值（屏幕像素为单位），线宽LineWeight应该需要经过换算才能得到这个像素线宽

    // 标志位定义
    static constexpr uint32_t kFlagPreSelected = 1 << 0;    // bit0: 预选高亮
    static constexpr uint32_t kFlagSelected = 1 << 1;       // bit1: 选中高亮
    static constexpr uint32_t kFlagDimmed = 1 << 2;         // bit2: 暗显，如锁定图层
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
    };
    bool bPreSelected = false;          // 是否预选中，预选触发会很频繁，所以不修改正常顶点数据而是单独创建独立预选顶点缓存数据，上传顶点数据时根据此标记用预选顶点数据替换正常顶点以实现高效高性能的预选高亮
    bool bSelected = false;             // 是否选中，有标记时单独建立选中顶点缓存，上传时根据标记来替换
    Type type = kInvalidEmtpyData;      // 实体渲染类型
    std::vector<DataCacheVertex> vertices;  // 顶点
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
    // 全量重生成标记，提供给命令层regen、初始化时以及某些可能影响全局显示的系统变量修改等场景使用
    virtual void markAllDirty() = 0;
    // 检查是否需要全量重新生成
    virtual bool needsRegenAll() const = 0;
    // 清除全量重新生成标记
    virtual void clearAllDirty() = 0;
    // 清除所有缓存数据（全量重新生成前调用）
    virtual void clearAllCacheData() = 0;
    
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
    // 通过ID查询读取实体缓存数据
    virtual const EntityGraphicsCacheData& getEntityCacheData(ObjectId id) const = 0;
    // 提供更通用的遍历接口，方便渲染器渲染，相比查询ID再根据ID去依次查询性能会更好，回调参数为实体id和实体缓存数据
    virtual void iterateAllCacheData(const std::function<void(ObjectId id, const EntityGraphicsCacheData& cacheData)>& func) = 0;
    
    // ============================================================================
    // 预选实体缓存数据相关接口，图形引擎对此不需要知情，完全缓存数据内部处理
    // 根据ID查询预选实体的预选缓存数据
    virtual const EntityGraphicsCacheData& getPreSelectedEntityCacheData(ObjectId id) const = 0;
    // 通知实体被预选中，通知后需要设置数据预选标记，获取时懒生成即可（没有就生成，有就读取），由选择任务负责通知
    virtual void notifyEntityPreSelected(ObjectId id) = 0;
    // 通知实体从预选状态移除，清除预选标记，预选数据不需要同时清除，几何重生成时才需要重新生成或者直接移除
    virtual void notifyEntityUnPreSelected(ObjectId id) = 0;

    // ============================================================================
    // 选中实体缓存数据相关接口，图形引擎对此不需要知情，完全缓存数据内部处理
    // 根据ID查询选中实体的选中缓存数据
    virtual const EntityGraphicsCacheData& getSelectedEntityCacheData(ObjectId id) const = 0;
    // 通知实体被选中，通知后需要设置数据选中标记，获取时懒生成即可（没有就生成，有就读取），由选择集负责通知
    virtual void notifyEntitySelected(ObjectId id) = 0;
    // 通知实体从选中状态移除，清除选中标记，选中数据不需要同时清除，几何重生成时才需要重新生成或者直接移除
    virtual void notifyEntityUnSelected(ObjectId id) = 0;
};

} // namespace tch
