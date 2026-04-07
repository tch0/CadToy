#pragma once

// C++ 标准库
#include <cstdint>

// 第三方库

// 项目头文件

namespace tch {

using ObjectId = uint64_t;

// ============================================================================
// 颜色
// ============================================================================

class DbColor {
public:
    enum Type {
        kByLayer,
        kByBlock,
        kColor
    };
    
    static DbColor byLayer() { return DbColor(kByLayer); }
    static DbColor byBlock() { return DbColor(kByBlock); }
    static DbColor rgb(uint8_t r, uint8_t g, uint8_t b) {
        DbColor c(kColor);
        c.m_rgb = (static_cast<uint32_t>(r) << 16) |
                  (static_cast<uint32_t>(g) << 8) |
                  static_cast<uint32_t>(b);
        return c;
    }
    static DbColor rgb(uint32_t rgb) {
        DbColor c(kColor);
        c.m_rgb = rgb;
        return c;
    }
    
    Type type() const { return m_type; }
    uint32_t rgb() const { return m_rgb; }
    
    bool isByLayer() const { return m_type == kByLayer; }
    bool isByBlock() const { return m_type == kByBlock; }
    
private:
    DbColor(Type t) : m_type(t) {}
    
    Type m_type;
    uint32_t m_rgb = 0;
};

// ============================================================================
// 线宽
// ============================================================================

enum class DbLineWeight : int16_t {
    kByLayer = -1,      // 跟随图层
    kByBlock = -2,      // 跟随块
    kByLwDefault = -3,  // 使用软件默认线宽（LWDEFAULT 系统变量）
    k000 = 0,
    k005 = 5,
    k009 = 9,
    k013 = 13,
    k015 = 15,
    k018 = 18,
    k020 = 20,
    k025 = 25,
    k030 = 30,
    k035 = 35,
    k040 = 40,
    k050 = 50,
    k053 = 53,
    k060 = 60,
    k070 = 70,
    k080 = 80,
    k090 = 90,
    k100 = 100,
    k106 = 106,
    k120 = 120,
    k140 = 140,
    k158 = 158,
    k200 = 200,
    k211 = 211
};

// ============================================================================
// 线型引用
// ============================================================================

class DbLinetypeRef {
public:
    enum Type {
        kByLayer,       // 跟随图层
        kByBlock,       // 跟随块
        kContinuous,    // 实线（特殊处理，不查表）
        kLinetype       // 普通线型，通过 ID 引用
    };
    
    static DbLinetypeRef byLayer() { return DbLinetypeRef(kByLayer); }
    static DbLinetypeRef byBlock() { return DbLinetypeRef(kByBlock); }
    static DbLinetypeRef continuous() { return DbLinetypeRef(kContinuous); }
    static DbLinetypeRef byId(ObjectId id) {
        DbLinetypeRef ref(kLinetype);
        ref.m_linetypeId = id;
        return ref;
    }
    
    Type type() const { return m_type; }
    ObjectId linetypeId() const { return m_linetypeId; }
    
    bool isByLayer() const { return m_type == kByLayer; }
    bool isByBlock() const { return m_type == kByBlock; }
    bool isContinuous() const { return m_type == kContinuous; }
    
private:
    DbLinetypeRef(Type t) : m_type(t) {}
    
    Type m_type;
    ObjectId m_linetypeId = 0;
};

} // namespace tch