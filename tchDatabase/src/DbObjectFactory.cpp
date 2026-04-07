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


namespace tch {

DbObjectFactory& DbObjectFactory::instance() {
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
}

std::unique_ptr<DbObject> DbObjectFactory::create(DbObject::Type type) const {
    size_t idx = static_cast<size_t>(type);
    if (idx < static_cast<size_t>(DbObject::kCount) && m_creators[idx]) {
        return m_creators[idx]();
    }
    return nullptr;
}

} // namespace tch
