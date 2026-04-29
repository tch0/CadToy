#pragma once

// C++ 标准库
#include <vector>
#include <cmath>
#include <algorithm>

// 第三方库
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 项目头文件


namespace tch {
namespace Geometry {

// ============================================================================
// 常量、精度
// ============================================================================

/// 圆周率 π
constexpr double PI = 3.14159265358979323846;
/// 2π
constexpr double TWO_PI = 2.0 * PI;

/// 精度配置结构体，用于控制几何计算中的容差，支持绝对精度、相对精度和角度精度
struct Tolerance {
    double absolute = 1e-9;   ///< 绝对精度，用于距离比较、重合判断等
    double relative = 1e-12;  ///< 相对精度，用于大坐标下的比较
    double angle = 1e-8;      ///< 角度容差（弧度）

    static const Tolerance Default;  ///< 默认精度
    static const Tolerance Loose;    ///< 宽松精度（1e-6, 1e-9, 1e-6）
    static const Tolerance Strict;   ///< 严格精度（1e-12, 1e-14, 1e-10）
};

// 注意：需要 inline 避免多重定义
inline const Tolerance Tolerance::Default;
inline const Tolerance Tolerance::Loose{1e-6, 1e-9, 1e-6};
inline const Tolerance Tolerance::Strict{1e-12, 1e-14, 1e-10};

// ============================================================================
// 基础类型
// ============================================================================

using Point = glm::dvec3;   ///< 三维点 (x, y, z)
using Vector = glm::dvec3;  ///< 三维向量 (x, y, z)
using Matrix = glm::dmat4;  ///< 4x4 仿射变换矩阵

// ============================================================================
// 基础比较函数
// ============================================================================

/// 判断浮点数是否为零，绝对值小于绝对精度
inline bool isZero(double v, const Tolerance& tol = Tolerance::Default) {
    return std::abs(v) < tol.absolute;
}

/// 判断两个浮点数是否相等，小值用绝对精度，大值用相对精度
inline bool isEqual(double a, double b, const Tolerance& tol = Tolerance::Default) {
    double diff = std::abs(a - b);
    if (diff < tol.absolute) {
        return true;
    }
    double scale = std::max(std::abs(a), std::abs(b));
    return diff < tol.relative * scale;
}

/// 判断两个角度是否相等（弧度）
/// 注意：调用前应确保角度已归一化到 [0, 2π)
inline bool isAngleEqual(double a, double b, const Tolerance& tol = Tolerance::Default) {
    double diff = std::abs(a - b);
    return diff < tol.angle;
}

// ============================================================================
// 点与向量比较
// ============================================================================

/// 判断两点是否重合
inline bool isCoincident(const Point& a, const Point& b, const Tolerance& tol = Tolerance::Default) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    double dz = a.z - b.z;
    
    if (std::abs(dx) < tol.absolute && std::abs(dy) < tol.absolute && std::abs(dz) < tol.absolute) {
        return true;
    }
    
    double scale = std::max({std::abs(a.x), std::abs(a.y), std::abs(a.z),
                              std::abs(b.x), std::abs(b.y), std::abs(b.z)});
    double relTol = tol.relative * std::max(1.0, scale);
        
    return std::abs(dx) < relTol && std::abs(dy) < relTol && std::abs(dz) < relTol;
}

/// 判断两个向量是否相等
inline bool isVectorEqual(const Vector& a, const Vector& b, const Tolerance& tol = Tolerance::Default) {
    return isEqual(a.x, b.x, tol) && 
           isEqual(a.y, b.y, tol) && 
           isEqual(a.z, b.z, tol);
}

/// 判断向量是否为零向量
inline bool isZeroVector(const Vector& v, const Tolerance& tol = Tolerance::Default) {
    return std::abs(v.x) < tol.absolute && 
           std::abs(v.y) < tol.absolute && 
           std::abs(v.z) < tol.absolute;
}

/// 判断两个向量是否平行（夹角接近 0 或 π）
inline bool isParallel(const Vector& a, const Vector& b, const Tolerance& tol = Tolerance::Default) {
    double lenA = glm::length(a);
    double lenB = glm::length(b);
    if (isZero(lenA, tol) || isZero(lenB, tol)) {
        return false;
    }
    double cosAngle = std::abs(glm::dot(a, b) / (lenA * lenB));
    cosAngle = std::min(1.0, cosAngle);  // 防止浮点误差导致略大于 1
    double sinAngleSq = 1.0 - cosAngle * cosAngle;
    return sinAngleSq < tol.angle * tol.angle;
}

/// 判断两个向量是否垂直（夹角接近 π/2 或 3π/2）
inline bool isPerpendicular(const Vector& a, const Vector& b, const Tolerance& tol = Tolerance::Default) {
    double lenA = glm::length(a);
    double lenB = glm::length(b);
    if (isZero(lenA, tol) || isZero(lenB, tol)) {
        return false;
    }
    // 当两向量接近垂直时，|cos(θ)| ≈ 0
    // |cos(θ)| < sin(angleTol) ≈ angleTol（当角度很小时）
    double cosAngle = std::abs(glm::dot(a, b) / (lenA * lenB));
    return cosAngle < tol.angle;
}

// ============================================================================
// 基本图元
// ============================================================================

/// 线段，由起点和终点定义
struct Segment {
    Point start;  ///< 起点
    Point end;    ///< 终点
    
    Segment() = default;
    Segment(const Point& s, const Point& e) : start(s), end(e) {}
    
    double length() const { return glm::distance(start, end); }           ///< 线段长度
    Point midpoint() const { return (start + end) * 0.5; }                ///< 中点
    Point pointAt(double t) const { return start + (end - start) * t; }   ///< 参数方程，t∈[0,1]
};

/// 射线，由原点和方向定义，参数 t ≥ 0
struct Ray {
    Point origin;     ///< 原点
    Vector direction; ///< 方向（单位向量）
    
    Ray() = default;
    Ray(const Point& o, const Vector& d) : origin(o), direction(glm::normalize(d)) {}
    
    Point pointAt(double t) const { return origin + direction * t; }  ///< 参数方程，t≥0
};

/// 无限直线，由原点和方向定义，参数 t ∈ R
struct Line {
    Point origin;     ///< 直线上一点
    Vector direction; ///< 方向（单位向量）
    
    Line() = default;
    Line(const Point& o, const Vector& d) : origin(o), direction(glm::normalize(d)) {}
    
    Point pointAt(double t) const { return origin + direction * t; }  ///< 参数方程，t∈R
};

/// 圆，由中心、法向量和半径定义，默认所有元素在XY平面
struct Circle {
    Point center;   ///< 圆心
    Vector normal;  ///< 圆所在平面的法向量（单位向量）
    double radius;  ///< 半径
    
    Circle() = default;
    Circle(const Point& c, const Vector& n, double r) : center(c), normal(glm::normalize(n)), radius(r) {}
    
    /// 创建XY平面上的圆（Z=0）
    static Circle xyPlane(const Point& c, double r) {
        return Circle(c, Vector(0, 0, 1), r);
    }

    /// 通过三点构造圆，返回 <是否成功, 圆>
    /// 三点共线或重合时返回 false，此时圆为默认构造
    static std::pair<bool, Circle> fromThreePoints(const Point& p1, const Point& p2, const Point& p3);

    double area() const { return PI * radius * radius; }           ///< 面积
    double circumference() const { return 2.0 * PI * radius; }     ///< 周长
    Point pointAt(double angle) const;  ///< 参数方程，angle∈[0,2π)
    /// 将圆上的点转换为参数（0 到 2π），注意：调用者需确保点在圆上
    double pointToParam(const Point& p) const;
};

/// 圆弧，圆的一部分，由起始角和终止角定义
struct Arc {
    Point center;       ///< 圆心
    Vector normal;      ///< 圆弧所在平面的法向量
    double radius;      ///< 半径
    double startAngle;  ///< 起始角（弧度）
    double endAngle;    ///< 终止角（弧度）
    
    Arc() = default;
    Arc(const Point& c, const Vector& n, double r, double sa, double ea)
        : center(c), normal(glm::normalize(n)), radius(r), startAngle(sa), endAngle(ea) {}
    
    /// 判断是否为完整圆
    bool isFull() const {
        double angleSpan = endAngle - startAngle;
        if (angleSpan < 0) {
            angleSpan += TWO_PI;
        }
        return std::abs(angleSpan - TWO_PI) < Tolerance::Default.angle;
    }
    
    double length() const;                      ///< 弧长
    double area() const;                        ///< 扇形面积
    Point pointAt(double t) const;              ///< 参数方程，t∈[0,1]
    /// 将圆弧上的点转换为参数角度（弧度），注意：调用者需确保点在圆弧所在圆上
    double pointToParam(const Point& p) const;
};

/// 椭圆，支持完整椭圆和椭圆弧，由中心、法向量、长短半轴、旋转角以及参数范围定义
struct Ellipse {
    Point center;       ///< 中心
    Vector normal;      ///< 椭圆所在平面的法向量
    double radiusX;     ///< X方向半轴长度（长半轴）
    double radiusY;     ///< Y方向半轴长度（短半轴）
    double rotation;    ///< 旋转角（弧度）
    double startParam;  ///< 起始参数（弧度），默认 0
    double endParam;    ///< 终止参数（弧度），默认 2π
    
    Ellipse() : startParam(0), endParam(2 * PI) {}
    
    Ellipse(const Point& c, const Vector& n, double rx, double ry, double rot)
        : center(c), normal(glm::normalize(n)), radiusX(rx), radiusY(ry), rotation(rot),
          startParam(0), endParam(2 * PI) {}
    
    Ellipse(const Point& c, const Vector& n, double rx, double ry, double rot,
            double start, double end)
        : center(c), normal(glm::normalize(n)), radiusX(rx), radiusY(ry), rotation(rot),
          startParam(start), endParam(end) {}
    
    /// 判断是否为完整椭圆
    bool isFull() const {
        double angleSpan = endParam - startParam;
        if (angleSpan < 0) {
            angleSpan += TWO_PI;
        }
        return std::abs(angleSpan - TWO_PI) < Tolerance::Default.angle;
    }
    
    /// 判断是否为椭圆弧
    bool isArc() const {
        return !isFull();
    }
    
    /// 获取椭圆局部坐标系（旋转后的轴向量）
    std::pair<Vector, Vector> getLocalAxes() const;
    
    double length() const;                      ///< 弧长（椭圆弧近似）
    double area() const { return PI * radiusX * radiusY; }  ///< 面积（完整椭圆）
    Point pointAt(double t) const;              ///< 参数方程，t∈[0,1] 对应 startParam 到 endParam
    /// 将椭圆上的点转换为参数（0 到 2π），注意：调用者需确保点在椭圆上
    double pointToParam(const Point& p) const;
};

/// 贝塞尔曲线，使用 de Casteljau 算法
struct BezierCurve {
    int degree;                         ///< 次数
    std::vector<Point> controlPoints;   ///< 控制点列表（size = degree + 1）
    
    BezierCurve() = default;
    BezierCurve(int deg, const std::vector<Point>& points) : degree(deg), controlPoints(points) {}
    
    Point evaluate(double t) const;                     ///< 求值，t∈[0,1]
    Point derivative(double t) const;                   ///< 求导（切线向量）
    std::vector<BezierCurve> subdivide(double t) const; ///< 在 t 处分割为两条曲线
};

/// B样条曲线，使用 Cox-de Boor 算法
struct BSplineCurve {
    int degree;                         ///< 次数
    std::vector<double> knots;          ///< 节点向量
    std::vector<Point> controlPoints;   ///< 控制点列表
    
    BSplineCurve() = default;
    BSplineCurve(int deg, const std::vector<double>& k, const std::vector<Point>& pts)
        : degree(deg), knots(k), controlPoints(pts) {}
    
    Point evaluate(double t) const;  ///< 求值，t∈[knots[degree], knots[n]]
};

/// NURBS曲线（非均匀有理B样条），带权重的 B样条曲线
struct NURBSCurve {
    int degree;                         ///< 次数
    std::vector<double> knots;          ///< 节点向量
    std::vector<Point> controlPoints;   ///< 控制点列表
    std::vector<double> weights;        ///< 权重列表（size = controlPoints.size()）
    
    NURBSCurve() = default;
    NURBSCurve(int deg, const std::vector<double>& k, const std::vector<Point>& pts, const std::vector<double>& w)
        : degree(deg), knots(k), controlPoints(pts), weights(w) {}
    
    Point evaluate(double t) const;  ///< 求值，t∈[knots[degree], knots[n]]
};

// ============================================================================
// 复合图元
// ============================================================================

/// 多段线，由连续点序列组成的折线
struct Polyline {
    std::vector<Point> points;  ///< 顶点列表
    bool closed = false;        ///< 是否闭合
    
    Polyline() = default;
    Polyline(const std::vector<Point>& pts, bool c = false) : points(pts), closed(c) {}
    
    double length() const;                  ///< 总长度
    std::vector<Segment> toSegments() const; ///< 转换为线段列表
};

using Polygon = Polyline;  ///< 多边形（封闭多段线）

/// 轴对齐矩形，由最小点和最大点定义
struct Rect {
    Point min;  ///< 最小点（左下）
    Point max;  ///< 最大点（右上）
    
    Rect() = default;
    Rect(const Point& m, const Point& M) : min(m), max(M) {}
    
    double width() const { return max.x - min.x; }       ///< 宽度
    double height() const { return max.y - min.y; }      ///< 高度
    double area() const { return width() * height(); }    ///< 面积
    std::vector<Point> vertices() const;                 ///< 四个顶点
    std::vector<Segment> edges() const;                  ///< 四条边
};

/// 正多边形，外接圆定义
struct RegularPolygon {
    Point center;       ///< 中心
    double radius;      ///< 外接圆半径
    int sides;          ///< 边数
    double rotation = 0.0;  ///< 旋转角（弧度）
    
    RegularPolygon() = default;
    RegularPolygon(const Point& c, double r, int s, double rot = 0.0)
        : center(c), radius(r), sides(s), rotation(rot) {}
    
    std::vector<Point> vertices() const;  ///< 顶点列表
    double area() const;                  ///< 面积
};

// ============================================================================
// 辅助结构
// ============================================================================

/// 轴对齐包围盒（AABB），用于快速剔除和空间索引
struct AABB {
    Point min;  ///< 最小点
    Point max;  ///< 最大点
    
    AABB() : min(Point(INFINITY, INFINITY, INFINITY)), max(Point(-INFINITY, -INFINITY, -INFINITY)) {}
    // 通过两点构造包围盒
    AABB(const Point& a, const Point& b) {
        min = Point(
            std::min(a.x, b.x),
            std::min(a.y, b.y),
            std::min(a.z, b.z)
        );
        max = Point(
            std::max(a.x, b.x),
            std::max(a.y, b.y),
            std::max(a.z, b.z)
        );
    }
    
    void expand(const Point& p);                 ///< 扩展包围盒以包含点 p
    void merge(const AABB& other);               ///< 合并另一个包围盒
    bool contains(const Point& p, const Tolerance& tol = Tolerance::Default) const;  ///< 是否包含点
    bool intersects(const AABB& other) const;    ///< 是否与另一个轴对齐包围盒相交
    bool intersectsSegment(const Point& p1, const Point& p2) const;  ///< 是否与线段相交
    Point center() const { return (min + max) * 0.5; }  ///< 中心点
    Vector size() const { return max - min; }            ///< 尺寸
};

// ============================================================================
// 距离计算
// ============================================================================

inline double distance(const Point& a, const Point& b) { return glm::distance(a, b); }

double distance(const Point& p, const Segment& seg);
double distance(const Point& p, const Line& line);
double distance(const Point& p, const Ray& ray);
double distance(const Point& p, const Circle& circle);
double distance(const Point& p, const Ellipse& ellipse);
double distance(const Segment& a, const Segment& b);
double distance(const Segment& seg, const Circle& circle);

// ============================================================================
// 投影计算
// ============================================================================

Point project(const Point& p, const Line& line);
Point project(const Point& p, const Segment& seg);
Point closestPoint(const Point& p, const Segment& seg);
Point closestPoint(const Point& p, const Circle& circle);

// ============================================================================
// 共线/共面检查
// ============================================================================

/// 判断点是否在无限直线上（直线由原点+方向定义）
bool isPointOnLine(const Point& p, const Point& lineOrigin, const Vector& lineDirection,
                   const Tolerance& tol = Tolerance::Default);

/// 判断两条直线是否共线
bool isCollinearLines(const Point& origin1, const Vector& dir1,
                      const Point& origin2, const Vector& dir2);

// 便捷包装
inline bool isCollinear(const Line& a, const Line& b) {
    return isCollinearLines(a.origin, a.direction, b.origin, b.direction);
}

inline bool isCollinear(const Ray& a, const Ray& b) {
    return isCollinearLines(a.origin, a.direction, b.origin, b.direction);
}

inline bool isCollinear(const Segment& a, const Segment& b) {
    return isCollinearLines(a.start, a.end - a.start, b.start, b.end - b.start);
}

// 两条直线共面（直线由原点+方向定义）
bool isCoplanarLines(const Point& origin1, const Vector& dir1,
                     const Point& origin2, const Vector& dir2);

// 直线与圆/椭圆共面
bool isCoplanarLineCurve(const Point& center, const Vector& normal,
                         const Point& lineOrigin, const Vector& lineDirection);

// 两个圆/椭圆共面
bool isCoplanarCurves(const Point& center1, const Vector& normal1,
                      const Point& center2, const Vector& normal2);

// 两条直线共面（具体类型包装）
inline bool isCoplanar(const Line& a, const Line& b) {
    return isCoplanarLines(a.origin, a.direction, b.origin, b.direction);
}

inline bool isCoplanar(const Ray& a, const Ray& b) {
    return isCoplanarLines(a.origin, a.direction, b.origin, b.direction);
}

inline bool isCoplanar(const Segment& a, const Segment& b) {
    return isCoplanarLines(a.start, a.end - a.start, b.start, b.end - b.start);
}

// 直线与圆/椭圆共面（具体类型包装）
inline bool isCoplanar(const Circle& circle, const Line& line) {
    return isCoplanarLineCurve(circle.center, circle.normal, line.origin, line.direction);
}

inline bool isCoplanar(const Circle& circle, const Ray& ray) {
    return isCoplanarLineCurve(circle.center, circle.normal, ray.origin, ray.direction);
}

inline bool isCoplanar(const Circle& circle, const Segment& seg) {
    return isCoplanarLineCurve(circle.center, circle.normal, seg.start, seg.end - seg.start);
}

inline bool isCoplanar(const Ellipse& ellipse, const Line& line) {
    return isCoplanarLineCurve(ellipse.center, ellipse.normal, line.origin, line.direction);
}

inline bool isCoplanar(const Ellipse& ellipse, const Ray& ray) {
    return isCoplanarLineCurve(ellipse.center, ellipse.normal, ray.origin, ray.direction);
}

inline bool isCoplanar(const Ellipse& ellipse, const Segment& seg) {
    return isCoplanarLineCurve(ellipse.center, ellipse.normal, seg.start, seg.end - seg.start);
}

// 两个圆/椭圆共面（具体类型包装）
inline bool isCoplanar(const Circle& a, const Circle& b) {
    return isCoplanarCurves(a.center, a.normal, b.center, b.normal);
}

inline bool isCoplanar(const Circle& circle, const Ellipse& ellipse) {
    return isCoplanarCurves(circle.center, circle.normal, ellipse.center, ellipse.normal);
}

inline bool isCoplanar(const Ellipse& a, const Ellipse& b) {
    return isCoplanarCurves(a.center, a.normal, b.center, b.normal);
}

// ============================================================================
// 直线相交结果
// ============================================================================

/// 直线相交结果结构体，用于 segment/line/ray 相交计算
struct LineIntersectionResult {
    enum Type {
        kNone,           ///< 无交点（异面或平行不共线）
        kSinglePoint,    ///< 有一个交点
        kOverlapSegment, ///< 有重叠线段
        kOverlapRay,     ///< 有重叠射线
        kOverlapLine     ///< 有重叠直线（两直线共线）
    };
    
    Type type = kNone;       ///< 相交类型
    Point p1;               ///< 第一个点（交点或重叠起点）
    Point p2;               ///< 第二个点（重叠终点，仅 OverlapSegment 使用）
    Vector direction;       ///< 方向向量（OverlapRay/OverlapLine 使用）
    
    bool hasIntersection() const { return type != kNone; }
    bool isPoint() const { return type == kSinglePoint; }
    bool isOverlap() const { return type >= kOverlapSegment; }
};

// ============================================================================
// 交点计算
// ============================================================================

// segment/line/ray 相交，返回 LineIntersectionResult
LineIntersectionResult intersect(const Segment& a, const Segment& b);
LineIntersectionResult intersect(const Segment& seg, const Line& line);
LineIntersectionResult intersect(const Segment& seg, const Ray& ray);
LineIntersectionResult intersect(const Line& a, const Line& b);
LineIntersectionResult intersect(const Line& line, const Ray& ray);
LineIntersectionResult intersect(const Ray& a, const Ray& b);

// 其他相交函数，返回 bool 并通过输出参数返回交点
bool intersect(const Segment& seg, const Circle& circle, std::vector<Point>& out);
bool intersect(const Segment& seg, const Ellipse& ellipse, std::vector<Point>& out);
bool intersect(const Line& line, const Circle& circle, std::vector<Point>& out);
bool intersect(const Line& line, const Ellipse& ellipse, std::vector<Point>& out);
bool intersect(const Ray& ray, const Circle& circle, std::vector<Point>& out);
bool intersect(const Ray& ray, const Ellipse& ellipse, std::vector<Point>& out);
bool intersect(const Circle& a, const Circle& b, std::vector<Point>& out);
bool intersect(const Circle& circle, const Ellipse& ellipse, std::vector<Point>& out);
bool intersect(const Ellipse& a, const Ellipse& b, std::vector<Point>& out);

// ============================================================================
// 包含性测试
// ============================================================================

bool contains(const Segment& seg, const Point& p, const Tolerance& tol = Tolerance::Default);
bool contains(const Ray& ray, const Point& p, const Tolerance& tol = Tolerance::Default);
bool contains(const Line& line, const Point& p, const Tolerance& tol = Tolerance::Default);

/// 判断点是否在圆上（注意：调用者需确保点在圆所在平面内）
bool contains(const Circle& circle, const Point& p, const Tolerance& tol = Tolerance::Default);
/// 判断点是否在椭圆上（注意：调用者需确保点在椭圆所在平面内）
bool contains(const Ellipse& ellipse, const Point& p, const Tolerance& tol = Tolerance::Default);
/// 判断点是否在多边形内（注意：调用者需确保点在多边形所在平面内，且多边形在 XY 平面）
bool contains(const Polygon& poly, const Point& p, const Tolerance& tol = Tolerance::Default);
/// 判断圆 inner 是否完全在圆 outer 内（注意：调用者需确保两圆共面）
bool contains(const Circle& outer, const Circle& inner, const Tolerance& tol = Tolerance::Default);

// ============================================================================
// 曲线细分
// ============================================================================

std::vector<Segment> subdivide(const Circle& circle, int segments);
std::vector<Segment> subdivide(const Ellipse& ellipse, int segments);
std::vector<Segment> subdivide(const Arc& arc, int segments);
std::vector<Segment> subdivide(const BezierCurve& curve, int segments);
std::vector<Segment> subdivide(const BSplineCurve& curve, int segments);
std::vector<Segment> subdivide(const NURBSCurve& curve, int segments);

// ============================================================================
// 切线计算
// ============================================================================

/// 圆上某角度的切线方向（返回单位向量）
Vector tangentAt(const Circle& circle, double angle);

/// 圆上某点的切线方向（返回单位向量），调用者需确保点在圆上且在圆所在平面内
Vector tangentAt(const Circle& circle, const Point& p);

/// 过圆外一点的切点，返回切点列表（0、1或2个），点在圆内返回空，调用者需确保点在圆所在平面内
std::vector<Point> tangentPointsFromPoint(const Circle& circle, const Point& externalPoint);

/// 椭圆上某参数的切线方向（返回单位向量），t∈[0,1] 对应 startParam 到 endParam
Vector tangentAt(const Ellipse& ellipse, double t);

/// 椭圆上某点的切线方向（返回单位向量），调用者需确保点在椭圆上且在椭圆所在平面内
Vector tangentAt(const Ellipse& ellipse, const Point& p);

// ============================================================================
// 几何变换
// ============================================================================

/// 点平移
inline Point translate(const Point& p, const Vector& delta) {
    return p + delta;
}

/// 点/向量旋转（绕轴）
inline Point rotate(const Point& p, const Vector& axis, double angle) {
    glm::dmat4 rot = glm::rotate(glm::dmat4(1.0), angle, axis);
    glm::dvec4 result = rot * glm::dvec4(p.x, p.y, p.z, 1.0);
    return Point(result.x, result.y, result.z);
}

/// 点/向量缩放
inline Point scale(const Point& p, const Vector& factor) {
    return Point(p.x * factor.x, p.y * factor.y, p.z * factor.z);
}

/// 矩阵求逆
inline Matrix inverse(const Matrix& m) {
    return glm::inverse(m);
}

// ============================================================================
// 几何变换之镜像变换
// ============================================================================

/// 点关于平面的镜像（通用 3D）
inline Point mirrorPoint(const Point& p, const Point& planeOrigin, const Vector& planeNormal) {
    Vector normal = glm::normalize(planeNormal);
    Vector rel = p - planeOrigin;
    double t = glm::dot(rel, normal);
    return p - normal * (2.0 * t);
}

/// 向量关于平面的镜像（通用 3D）
inline Vector mirrorVector(const Vector& v, const Vector& planeNormal) {
    Vector normal = glm::normalize(planeNormal);
    double t = glm::dot(v, normal);
    return v - normal * (2.0 * t);
}

/// 关于 XY 平面的镜像（Z 反向，适用于点或向量）
inline glm::dvec3 mirrorXY(const glm::dvec3& v) {
    return glm::dvec3(v.x, v.y, -v.z);
}

/// 关于 XZ 平面的镜像（Y 反向，适用于点或向量）
inline glm::dvec3 mirrorXZ(const glm::dvec3& v) {
    return glm::dvec3(v.x, -v.y, v.z);
}

/// 关于 YZ 平面的镜像（X 反向，适用于点或向量）
inline glm::dvec3 mirrorYZ(const glm::dvec3& v) {
    return glm::dvec3(-v.x, v.y, v.z);
}

/// 关于 X 轴的镜像（2D，等价于 mirrorXZ，适用于点或向量）
inline glm::dvec3 mirrorX(const glm::dvec3& v) {
    return mirrorXZ(v);
}

/// 关于 Y 轴的镜像（2D，等价于 mirrorYZ，适用于点或向量）
inline glm::dvec3 mirrorY(const glm::dvec3& v) {
    return mirrorYZ(v);
}

/// 关于中心点的镜像（点：P' = 2*C - P；向量：V' = -V）
inline glm::dvec3 mirrorCenter(const glm::dvec3& v, const glm::dvec3& center) {
    return center * 2.0 - v;
}

/// 关于原点的镜像（点或向量）
inline glm::dvec3 mirrorCenter(const glm::dvec3& v) {
    return mirrorCenter(v, glm::dvec3(0, 0, 0));
}

// ============================================================================
// 角度计算
// ============================================================================

/// 计算两向量之间的夹角（弧度，返回值 [0, π]）
double angleBetween(const Vector& a, const Vector& b);

/// 计算三点形成的夹角（弧度，以 b 为顶点，返回值 [0, π]）
double angleAt(const Point& a, const Point& b, const Point& c);

/// 计算向量与 X 轴正方向的夹角（弧度，返回值 [0, 2π)）
double angleOf(const Vector& v);

// ============================================================================
// 辅助函数
// ============================================================================

/// 角度转弧度
inline double toRadians(double degrees) {
    return degrees * PI / 180.0;
}

/// 弧度转角度
inline double toDegrees(double radians) {
    return radians * 180.0 / PI;
}

/// 归一化到 [0, 2π)
inline double normalizeAngle(double angle) {
    angle = fmod(angle, TWO_PI);
    if (angle < 0) {
        angle += TWO_PI;
    }
    return angle;
}

} // namespace Geometry
} // namespace tch