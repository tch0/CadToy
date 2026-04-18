// 对应头文件
#include "DbObjectFactory.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "DbLine.h"
#include "DbCircle.h"
#include "DbArc.h"
#include "DbEllipse.h"
#include "DbRay.h"
#include "DbXLine.h"
#include "DbLayer.h"


namespace tch {

DbObjectFactory& DbObjectFactory::getInstance() {
    static DbObjectFactory registry;
    return registry;
}

DbObjectFactory::DbObjectFactory() {
    for (auto& creator : m_creators) {
        creator = nullptr;
    }
    
    registerType<DbLine>(DbObject::kLine);
    registerType<DbCircle>(DbObject::kCircle);
    registerType<DbArc>(DbObject::kArc);
    registerType<DbEllipse>(DbObject::kEllipse);
    registerType<DbRay>(DbObject::kRay);
    registerType<DbXLine>(DbObject::kXLine);
    registerType<DbLayer>(DbObject::kLayer);
}

std::unique_ptr<DbObject> DbObjectFactory::create(DbObject::Type type) const {
    size_t idx = static_cast<size_t>(type);
    if (idx < static_cast<size_t>(DbObject::kCount) && m_creators[idx]) {
        return m_creators[idx]();
    }
    return nullptr;
}

std::unique_ptr<DbObject> DbObjectFactory::createFromJson(const rapidjson::Value& value) const {
    if (!value.IsObject()) {
        return nullptr;
    }

    // 读取 type 字段
    if (!value.HasMember("type") || !value["type"].IsInt()) {
        return nullptr;
    }

    int typeValue = value["type"].GetInt();
    if (typeValue < 0 || typeValue >= static_cast<int>(DbObject::kCount)) {
        return nullptr;
    }

    DbObject::Type type = static_cast<DbObject::Type>(typeValue);
    std::unique_ptr<DbObject> obj = create(type);
    if (!obj) {
        return nullptr;
    }

    // 从 JSON 读取数据
    if (!obj->loadFromJson(value)) {
        return nullptr;
    }

    return obj;
}

} // namespace tch
