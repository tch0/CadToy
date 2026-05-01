#pragma once

// C++ 标准库

// 第三方库

// 项目头文件
#include "IGraphicsDataCache.h"


namespace tch {

class DbEntity;

// ============================================================================
// 图形引擎接口
// 职责：无状态算法库，根据数据缓存需要生成数据
// ============================================================================
class IGraphicsEngine {
public:
    virtual ~IGraphicsEngine() = default;

    // 生成数据缓存，为缓存中所有脏实体生成缓存，调用数据缓存的相关接口
    virtual void generate(IGraphicsDataCache* pDataCache) const = 0;

    // 单独为指定实体生成图形缓存数据
    virtual void generateForEntity(IGraphicsDataCache* pDataCache, ObjectId id) const = 0;
};

} // namespace tch
