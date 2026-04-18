// C++ 标准库
#include <array>
#include <memory>
#include <functional>

// 第三方库
#include <rapidjson/document.h>

// 项目头文件
#include "DbObject.h"


namespace tch {

class DbObjectFactory {
public:
    using Creator = std::function<std::unique_ptr<DbObject>()>;
    
    static DbObjectFactory& getInstance();
    
    template<typename T>
    void registerType(DbObject::Type type) {
        size_t idx = static_cast<size_t>(type);
        if (idx < static_cast<size_t>(DbObject::kCount)) {
            m_creators[idx] = []() { return std::make_unique<T>(); };
        }
    }
    
    std::unique_ptr<DbObject> create(DbObject::Type type) const;
    
    // 从 JSON 创建对象
    std::unique_ptr<DbObject> createFromJson(const rapidjson::Value& value) const;
    
private:
    DbObjectFactory();
    ~DbObjectFactory() = default;
    
    DbObjectFactory(const DbObjectFactory&) = delete;
    DbObjectFactory& operator=(const DbObjectFactory&) = delete;
    
    std::array<Creator, static_cast<size_t>(DbObject::kCount)> m_creators;
};

} // namespace tch