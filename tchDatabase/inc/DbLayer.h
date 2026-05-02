#pragma once

// C++ 标准库
#include <string>
#include <algorithm>

// 第三方库

// 项目头文件
#include "DbObject.h"
#include "DbCommon.h"


namespace tch {

class DbLayer : public DbObject {
public:
    // RTTI
    static constexpr Type staticType() { return Type::kLayer; }
    Type type() const override { return staticType(); }
    const char* typeName() const override { return "DbLayer"; }
    
    bool isType(Type t) const override {
        if (t == kLayer) { return true; }
        return DbObject::isType(t);
    }
    
    DbLayer() = default;
    explicit DbLayer(const std::string& name) : m_name(name) {}
    
    // 名称
    const std::string& name() const { return m_name; }
    void setName(const std::string& name);
    
    // 状态
    bool isFrozen() const { return m_frozen; }
    void setFrozen(bool frozen);

    bool isLocked() const { return m_locked; }
    void setLocked(bool locked);
    
    // 默认属性
    const DbColor& color() const { return m_color; }
    void setColor(const DbColor& color);

    const DbLinetypeRef& linetype() const { return m_linetype; }
    void setLinetype(const DbLinetypeRef& linetype);

    DbLineWeight lineWeight() const { return m_lineWeight; }
    void setLineWeight(DbLineWeight lw);

    // 透明度 0.0~1.0，默认0.0（不透明）
    float transparency() const { return m_transparency; }
    void setTransparency(float t);

    // 说明
    const std::string& description() const { return m_description; }
    void setDescription(const std::string& desc);
    
    // 克隆
    std::unique_ptr<DbObject> clone() const override;
    
    // 通知数据库图层被修改
    void notifyModified() override;
    
protected:
    void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
    bool readFields(const rapidjson::Value& value) override;
    
private:
    std::string m_name;
    bool m_frozen = false;
    bool m_locked = false;
    DbColor m_color = DbColor::White;  // 白色
    DbLinetypeRef m_linetype = DbLinetypeRef::continuous();
    DbLineWeight m_lineWeight = DbLineWeight::kByLwDefault;
    float m_transparency = 0.0f;  // 0.0~1.0
    std::string m_description;
};

} // namespace tch
