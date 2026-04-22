#pragma once

// C++ 标准库
#include <cstdint>
#include <string>

// 第三方库
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <glm/glm.hpp>

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
        kRGB
    };
    
    // 预定义颜色（静态成员）
    static const DbColor White;    // 白
    static const DbColor Red;      // 红
    static const DbColor Yellow;   // 黄
    static const DbColor Green;    // 绿
    static const DbColor Cyan;     // 青
    static const DbColor Blue;     // 蓝
    static const DbColor Magenta;  // 洋红
    
    static DbColor byLayer() { return DbColor(kByLayer); }
    static DbColor byBlock() { return DbColor(kByBlock); }
    static DbColor fromRgb(uint8_t r, uint8_t g, uint8_t b) {
        DbColor c(kRGB);
        c.m_rgb = (static_cast<uint32_t>(r) << 16) |
                  (static_cast<uint32_t>(g) << 8) |
                  static_cast<uint32_t>(b);
        return c;
    }
    static DbColor fromRgb(uint32_t rgb) {
        DbColor c(kRGB);
        c.m_rgb = rgb;
        return c;
    }
    
    Type type() const { return m_type; }
    uint32_t rgb() const { return m_rgb; }
    
    bool isByLayer() const { return m_type == kByLayer; }
    bool isByBlock() const { return m_type == kByBlock; }
    
    // 将RGB颜色转换为glm::vec3（范围0.0-1.0）
    glm::vec3 toVec3Color() const {
        return glm::vec3(
            ((m_rgb >> 16) & 0xFF) / 255.0f,
            ((m_rgb >> 8) & 0xFF) / 255.0f,
            (m_rgb & 0xFF) / 255.0f
        );
    }
    
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
        kLinetypeId     // 普通线型，通过 ID 引用
    };
    
    static DbLinetypeRef byLayer() { return DbLinetypeRef(kByLayer); }
    static DbLinetypeRef byBlock() { return DbLinetypeRef(kByBlock); }
    static DbLinetypeRef continuous() { return DbLinetypeRef(kContinuous); }
    static DbLinetypeRef fromId(ObjectId id) {
        DbLinetypeRef ref(kLinetypeId);
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

// ============================================================================
// JSON 序列化工具函数
// ============================================================================

namespace DbJsonUtils {

// 基本类型
void writeDouble(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, double value);
void readDouble(const rapidjson::Value& value, double& out);

void writeInt(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, int value);
void readInt(const rapidjson::Value& value, int& out);

void writeUint(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, uint32_t value);
void readUint(const rapidjson::Value& value, uint32_t& out);

void writeUint64(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, uint64_t value);
void readUint64(const rapidjson::Value& value, uint64_t& out);

void writeBool(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, bool value);
void readBool(const rapidjson::Value& value, bool& out);

// 字符串
void writeString(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const char* str);
void writeString(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const std::string& str);
void readString(const rapidjson::Value& value, std::string& out);

// 2D 向量/点 [x, y]
void writeVectorPoint2d(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const glm::dvec2& v);
void readVectorPoint2d(const rapidjson::Value& value, glm::dvec2& out);

// 3D 向量/点 [x, y, z]
void writeVectorPoint3d(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const glm::dvec3& v);
void readVectorPoint3d(const rapidjson::Value& value, glm::dvec3& out);

// 颜色 [type, rgb?]
void writeColor(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const DbColor& color);
void readColor(const rapidjson::Value& value, DbColor& out);

// 线型 [type, id?]
void writeLinetype(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const DbLinetypeRef& linetype);
void readLinetype(const rapidjson::Value& value, DbLinetypeRef& out);

// 线宽
void writeLineWeight(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, DbLineWeight lw);
void readLineWeight(const rapidjson::Value& value, DbLineWeight& out);

} // namespace DbJsonUtils

} // namespace tch
