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
    static Type staticType() { return Type::kUnknown; }
    Type type() const override { return staticType(); }
    const char* typeName() const override { return "DbEntity"; }
    
    // 图层
    ObjectId layerId() const { return m_layerId; }
    void setLayerId(ObjectId id) { m_layerId = id; }
    
    // 颜色
    const DbColor& color() const { return m_color; }
    void setColor(const DbColor& color) { m_color = color; }
    
    // 线型
    const DbLinetypeRef& linetype() const { return m_linetype; }
    void setLinetype(const DbLinetypeRef& lt) { m_linetype = lt; }
    
    // 线型比例
    double linetypeScale() const { return m_linetypeScale; }
    void setLinetypeScale(double scale) { m_linetypeScale = scale; }
    
    // 线宽
    DbLineWeight lineWeight() const { return m_lineWeight; }
    void setLineWeight(DbLineWeight lw) { m_lineWeight = lw; }
    
    // 可见性
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
    // 几何接口
    virtual Geometry::AABB boundingBox() const = 0;
    
    std::unique_ptr<DbObject> clone() const override = 0;
    
    // 通知数据库实体被修改
    void notifyModified() override;
    
protected:
    void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
    bool readFields(const rapidjson::Value& value) override;
    
    ObjectId m_layerId = 0;
    DbColor m_color = DbColor::byLayer();
    DbLinetypeRef m_linetype = DbLinetypeRef::continuous();
    double m_linetypeScale = 1.0;
    DbLineWeight m_lineWeight = DbLineWeight::kByLayer;
    bool m_visible = true;
};

} // namespace tch