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


// 前置声明
class Database;

// 实体ID类型（可根据实际定义调整）
using ObjectId = uint64_t;

// ================================================================================================
// 图形缓存接口
// 职责：关联到唯一的数据库实例，管理数据库中每个实体的CPU端顶点数据，维护脏实体集合，接收来自数据库的通知。
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
    // 设置指定实体的顶点数据（移动语义）
    virtual void setEntityVertices(ObjectId id, std::vector<DataCacheVertex>&& vertices) = 0;
    // 移除实体顶点数据
    virtual void removeEntityVertices(ObjectId id) = 0;
    // 清除指定实体的脏标记（引擎处理完该实体后调用）
    virtual void clearDirty(ObjectId id) = 0;
    // 标记所有实体为脏，以便全量重生成，提供给命令层以及初始化时使用
    virtual void generateAll() = 0;
    
    // ============================================================================
    // 通知接口：提供给数据库通知实体变化情况，其中会标记实体为脏
    // 实体已添加，需要为实体添加并生成顶点数据
    virtual void onEntityAdded(ObjectId id) = 0;
    // 实体已修改，几何或属性变化，需重新生成顶点数据
    virtual void onEntityModified(ObjectId id) = 0;
    // 实体已删除，清理该实体的顶点数据
    virtual void onEntityRemoved(ObjectId id) = 0;
    
    // ============================================================================
    // 顶点数据查询接口：提供给渲染器渲染
    // 获取所有实体ID
    virtual std::vector<ObjectId> getAllEntityIds() const = 0;
    // 通过ID查询读取顶点数据
    virtual const std::vector<DataCacheVertex>& getEntityVertices(ObjectId id) const = 0;
    // 提供更通用的遍历接口，方便渲染器渲染，相比查询ID再根据ID去依次查询性能会更好，回调参数为实体id和数组引用
    virtual void iterateAllCacheData(const std::function<void(ObjectId id, const std::vector<DataCacheVertex>& vertices)>& func) = 0;
};

} // namespace tch
