#pragma once

// C++ 标准库

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

// 项目头文件
#include "DbCommon.h"
#include "DbObject.h"
#include "Geometry.h"


namespace tch {

class DbEntity : public DbObject {
public:
    // RTTI
    static constexpr Type staticType() { return Type::kEntity; }
    Type type() const override = 0;
    const char* typeName() const override = 0;
    
    bool isType(Type t) const override {
        if (t == kEntity) { return true; }
        return DbObject::isType(t);
    }
    
    DbEntity() = default;
    ~DbEntity() override = default;
    
    // 图层
    ObjectId layerId() const { return m_layerId; }
    void setLayerId(ObjectId id);
    
    // 颜色
    const DbColor& color() const { return m_color; }
    void setColor(const DbColor& color);
    
    // 线型
    const DbLinetypeRef& linetype() const { return m_linetype; }
    void setLinetype(const DbLinetypeRef& lt);
    
    // 线型比例
    double linetypeScale() const { return m_linetypeScale; }
    void setLinetypeScale(double scale);
    
    // 线宽
    DbLineWeight lineWeight() const { return m_lineWeight; }
    void setLineWeight(DbLineWeight lw);
    
    // 可见性
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);
    
    // 几何接口 - 获取包围盒（带缓存）
    virtual Geometry::AABB boundingBox() const final;
    
    // 计算包围盒 - 由各派生类实现具体计算逻辑
    virtual Geometry::AABB computeBoundingBox() const = 0;
    
    // 实体是否完全位于给定的轴对齐包围盒内
    virtual bool isInside(const Geometry::AABB& rect) const = 0;
    
    // 实体是否与给定轴对齐包围盒相交（包括完全包含在内）
    virtual bool intersects(const Geometry::AABB& rect) const = 0;
    
    std::unique_ptr<DbObject> clone() const override = 0;
    
    // 通知数据库实体被修改
    void notifyModified() override;
    
    // 从数据库系统变量设置实体默认属性（图层、颜色、线型、线型比例、线宽）
    // 命令层创建实体后调用，快速应用数据库当前设置
    void setPropertiesFromDb();
    
protected:
    void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
    bool readFields(const rapidjson::Value& value) override;
    
    ObjectId m_layerId = 0;
    DbColor m_color = DbColor::byLayer();
    DbLinetypeRef m_linetype = DbLinetypeRef::continuous();
    double m_linetypeScale = 1.0;
    DbLineWeight m_lineWeight = DbLineWeight::kByLayer;
    bool m_visible = true;
    
    // 包围盒缓存
    mutable Geometry::AABB m_cachedBBox;   // 缓存包围盒
    mutable bool m_bboxDirty = true;       // 脏标记
};

} // namespace tch