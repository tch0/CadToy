// 对应头文件
#include "DbCommon.h"

// C++ 标准库

// 第三方库

// 项目头文件


namespace tch {

// ============================================================================
// DbColor 预定义颜色定义
// ============================================================================

const DbColor DbColor::White = DbColor::fromRgb(255, 255, 255);
const DbColor DbColor::Red = DbColor::fromRgb(255, 0, 0);
const DbColor DbColor::Yellow = DbColor::fromRgb(255, 255, 0);
const DbColor DbColor::Green = DbColor::fromRgb(0, 255, 0);
const DbColor DbColor::Cyan = DbColor::fromRgb(0, 255, 255);
const DbColor DbColor::Blue = DbColor::fromRgb(0, 0, 255);
const DbColor DbColor::Magenta = DbColor::fromRgb(255, 0, 255);

namespace DbJsonUtils {

// ============================================================================
// 基本类型
// ============================================================================

void writeDouble(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, double value) {
    writer.Double(value);
}

void readDouble(const rapidjson::Value& value, double& out) {
    if (value.IsNumber()) {
        out = value.GetDouble();
    }
}

void writeInt(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, int value) {
    writer.Int(value);
}

void readInt(const rapidjson::Value& value, int& out) {
    if (value.IsInt()) {
        out = value.GetInt();
    }
}

void writeUint(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, uint32_t value) {
    writer.Uint(value);
}

void readUint(const rapidjson::Value& value, uint32_t& out) {
    if (value.IsUint()) {
        out = value.GetUint();
    }
}

void writeUint64(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, uint64_t value) {
    writer.Uint64(value);
}

void readUint64(const rapidjson::Value& value, uint64_t& out) {
    if (value.IsUint64()) {
        out = value.GetUint64();
    }
}

void writeBool(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, bool value) {
    writer.Bool(value);
}

void readBool(const rapidjson::Value& value, bool& out) {
    if (value.IsBool()) {
        out = value.GetBool();
    }
}

// ============================================================================
// 字符串
// ============================================================================

void writeString(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const char* str) {
    writer.String(str);
}

void writeString(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const std::string& str) {
    writer.String(str.c_str());
}

void readString(const rapidjson::Value& value, std::string& out) {
    if (value.IsString()) {
        out = value.GetString();
    }
}

// ============================================================================
// 2D 向量/点 [x, y]
// ============================================================================

void writeVectorPoint2d(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const glm::dvec2& v) {
    writer.StartArray();
    writer.Double(v.x);
    writer.Double(v.y);
    writer.EndArray();
}

void readVectorPoint2d(const rapidjson::Value& value, glm::dvec2& out) {
    if (value.IsArray() && value.Size() >= 2) {
        const auto& x = value[0];
        const auto& y = value[1];
        if (x.IsNumber() && y.IsNumber()) {
            out.x = x.GetDouble();
            out.y = y.GetDouble();
        }
    }
}

// ============================================================================
// 3D 向量/点 [x, y, z]
// ============================================================================

void writeVectorPoint3d(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const glm::dvec3& v) {
    writer.StartArray();
    writer.Double(v.x);
    writer.Double(v.y);
    writer.Double(v.z);
    writer.EndArray();
}

void readVectorPoint3d(const rapidjson::Value& value, glm::dvec3& out) {
    if (value.IsArray() && value.Size() >= 3) {
        const auto& x = value[0];
        const auto& y = value[1];
        const auto& z = value[2];
        if (x.IsNumber() && y.IsNumber() && z.IsNumber()) {
            out.x = x.GetDouble();
            out.y = y.GetDouble();
            out.z = z.GetDouble();
        }
    }
}

// ============================================================================
// 颜色 [type, rgb?]
// ============================================================================

void writeColor(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const DbColor& color) {
    writer.StartArray();
    writer.Int(static_cast<int>(color.type()));
    if (color.type() == DbColor::kRGB) {
        writer.Uint(color.rgb());
    }
    writer.EndArray();
}

void readColor(const rapidjson::Value& value, DbColor& out) {
    if (!value.IsArray() || value.Size() < 1) {
        return;
    }
    const auto& typeVal = value[0];
    if (!typeVal.IsInt()) {
        return;
    }
    int type = typeVal.GetInt();
    if (type == DbColor::kByLayer) {
        out = DbColor::byLayer();
    } else if (type == DbColor::kByBlock) {
        out = DbColor::byBlock();
    } else if (type == DbColor::kRGB && value.Size() >= 2 && value[1].IsUint()) {
        out = DbColor::fromRgb(value[1].GetUint());
    }
}

// ============================================================================
// 线型 [type, id?]
// ============================================================================

void writeLinetype(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const DbLinetypeRef& linetype) {
    writer.StartArray();
    writer.Int(static_cast<int>(linetype.type()));
    if (linetype.type() == DbLinetypeRef::kLinetypeId) {
        writer.Uint64(linetype.linetypeId());
    }
    writer.EndArray();
}

void readLinetype(const rapidjson::Value& value, DbLinetypeRef& out) {
    if (!value.IsArray() || value.Size() < 1) {
        return;
    }
    const auto& typeVal = value[0];
    if (!typeVal.IsInt()) {
        return;
    }
    int type = typeVal.GetInt();
    if (type == DbLinetypeRef::kByLayer) {
        out = DbLinetypeRef::byLayer();
    } else if (type == DbLinetypeRef::kByBlock) {
        out = DbLinetypeRef::byBlock();
    } else if (type == DbLinetypeRef::kContinuous) {
        out = DbLinetypeRef::continuous();
    } else if (type == DbLinetypeRef::kLinetypeId && value.Size() >= 2 && value[1].IsUint64()) {
        out = DbLinetypeRef::fromId(value[1].GetUint64());
    }
}

// ============================================================================
// 线宽
// ============================================================================

void writeLineWeight(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, DbLineWeight lw) {
    writer.Int(static_cast<int>(lw));
}

void readLineWeight(const rapidjson::Value& value, DbLineWeight& out) {
    if (value.IsInt()) {
        out = static_cast<DbLineWeight>(value.GetInt());
    }
}

} // namespace DbJsonUtils
} // namespace tch
