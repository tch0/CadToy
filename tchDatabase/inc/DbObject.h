#pragma once

// C++ 标准库
#include <cstdint>
#include <memory>

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

// 项目头文件


namespace tch {

using ObjectId = uint64_t;

class DbObject {
public:
    // DbObject的对象类型
    enum Type : uint16_t {
        kUnknown = 0,
        // 实体
        kLine,
        kCircle,
        kArc,
        kEllipse,
        kXLine,
        kRay,
        // 其他对象
        kLayer,
        // 计数
        kCount
    };
    
    DbObject() = default;
    virtual ~DbObject() = default;
    
    // ID 管理
    ObjectId id() const { return m_id; }
    
    // 类型信息与RTTI
    virtual Type type() const = 0;
    virtual const char* typeName() const = 0;
    
    bool isType(Type t) const { return type() == t; }
    
    template<typename T>
    T* as() {
        if (type() == T::staticType()) { return static_cast<T*>(this); }
        return nullptr;
    }
    
    template<typename T>
    const T* as() const {
        if (type() == T::staticType()) { return static_cast<const T*>(this); }
        return nullptr;
    }
    
    // 克隆
    virtual std::unique_ptr<DbObject> clone() const = 0;
    
    // 序列化
    virtual void toJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
    virtual bool fromJson(const rapidjson::Value& value);
    
protected:
    virtual void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
    virtual bool readFields(const rapidjson::Value& value);
    
private:
    friend class Database;
    void setId(ObjectId id) { m_id = id; } // 仅提供给Database使用
    ObjectId m_id = 0;
};

} // namespace tch