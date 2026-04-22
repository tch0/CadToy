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

// 前向声明
class Database;

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
    
    // 数据库访问
    Database* database() const { return m_pDb; }
    
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
    virtual void saveToJson(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
    virtual bool loadFromJson(const rapidjson::Value& value);
    
    // 通知数据库对象被修改（子类实现，在属性修改时调用）
    virtual void notifyModified() = 0;
    
protected:
    virtual void writeFields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
    virtual bool readFields(const rapidjson::Value& value);
    
protected:
    friend class Database;
    void setId(ObjectId id) { m_id = id; }
    void setDatabase(Database* pDb) { m_pDb = pDb; }
    
    ObjectId m_id = 0;
    Database* m_pDb = nullptr;
};

} // namespace tch