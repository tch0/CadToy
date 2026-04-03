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
// 零、常量、精度
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
// 一、基础类型
// ============================================================================

using Point = glm::dvec3;   ///< 三维点 (x, y, z)
using Vector = glm::dvec3;  ///< 三维向量 (x, y, z)
using Matrix = glm::dmat4;  ///< 4x4 仿射变换矩阵

// ============================================================================
// 二、基础比较函数
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
// 三、点与向量比较
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
    Vector na = a / lenA;
    Vector nb = b / lenB;
    double dot = std::abs(glm::dot(na, nb));
    return dot > 1.0 - tol.absolute;
}

/// 判断两个向量是否垂直（夹角接近 π/2 或 3π/2）
inline bool isPerpendicular(const Vector& a, const Vector& b, const Tolerance& tol = Tolerance::Default) {
    double lenA = glm::length(a);
    double lenB = glm::length(b);
    if (isZero(lenA, tol) || isZero(lenB, tol)) {
        return false;
    }
    Vector na = a / lenA;
    Vector nb = b / lenB;
    double dot = std::abs(glm::dot(na, nb));
    return dot < tol.absolute;
}

// ============================================================================
// 四、基本图元
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
    
    double area() const { return PI * radius * radius; }           ///< 面积
    double circumference() const { return 2.0 * PI * radius; }     ///< 周长
    Point pointAt(double angle) const;  ///< 参数方程，angle∈[0,2π)
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
    
    double length() const;                      ///< 弧长
    Point pointAt(double t) const;              ///< 参数方程，t∈[0,1]
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
    double pointToParam(const Point& p) const;  ///< 将椭圆上的点转换为参数（0 到 2π）
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
// 五、复合图元
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
// 六、辅助结构
// ============================================================================

/// 轴对齐包围盒（AABB），用于快速剔除和空间索引
struct AABB {
    Point min;  ///< 最小点
    Point max;  ///< 最大点
    
    AABB() : min(Point(INFINITY, INFINITY, INFINITY)), max(Point(-INFINITY, -INFINITY, -INFINITY)) {}
    AABB(const Point& m, const Point& M) : min(m), max(M) {}
    
    void expand(const Point& p);                 ///< 扩展包围盒以包含点 p
    void merge(const AABB& other);               ///< 合并另一个包围盒
    bool contains(const Point& p, const Tolerance& tol = Tolerance::Default) const;  ///< 是否包含点
    bool intersects(const AABB& other) const;    ///< 是否与另一个包围盒相交
    Point center() const { return (min + max) * 0.5; }  ///< 中心点
    Vector size() const { return max - min; }            ///< 尺寸
};

// ============================================================================
// 七、距离计算
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
// 八、投影计算
// ============================================================================

Point project(const Point& p, const Line& line);
Point project(const Point& p, const Segment& seg);
Point closestPoint(const Point& p, const Segment& seg);
Point closestPoint(const Point& p, const Circle& circle);

// ============================================================================
// 九、交点计算
// ============================================================================

bool intersect(const Segment& a, const Segment& b, Point& out);
bool intersect(const Segment& seg, const Line& line, Point& out);
bool intersect(const Segment& seg, const Ray& ray, Point& out);
bool intersect(const Segment& seg, const Circle& circle, std::vector<Point>& out);
bool intersect(const Segment& seg, const Ellipse& ellipse, std::vector<Point>& out);

bool intersect(const Line& a, const Line& b, Point& out);
bool intersect(const Line& line, const Ray& ray, Point& out);
bool intersect(const Line& line, const Circle& circle, std::vector<Point>& out);
bool intersect(const Line& line, const Ellipse& ellipse, std::vector<Point>& out);

bool intersect(const Ray& a, const Ray& b, Point& out);
bool intersect(const Ray& ray, const Segment& seg, Point& out);
bool intersect(const Ray& ray, const Line& line, Point& out);
bool intersect(const Ray& ray, const Circle& circle, std::vector<Point>& out);
bool intersect(const Ray& ray, const Ellipse& ellipse, std::vector<Point>& out);

bool intersect(const Circle& a, const Circle& b, std::vector<Point>& out);
bool intersect(const Circle& circle, const Line& line, std::vector<Point>& out);
bool intersect(const Circle& circle, const Ray& ray, std::vector<Point>& out);
bool intersect(const Circle& circle, const Segment& seg, std::vector<Point>& out);
bool intersect(const Circle& circle, const Ellipse& ellipse, std::vector<Point>& out);

bool intersect(const Ellipse& a, const Ellipse& b, std::vector<Point>& out);
bool intersect(const Ellipse& ellipse, const Line& line, std::vector<Point>& out);
bool intersect(const Ellipse& ellipse, const Ray& ray, std::vector<Point>& out);
bool intersect(const Ellipse& ellipse, const Segment& seg, std::vector<Point>& out);
bool intersect(const Ellipse& ellipse, const Circle& circle, std::vector<Point>& out);

// ============================================================================
// 十、包含性测试
// ============================================================================

bool contains(const Segment& seg, const Point& p, const Tolerance& tol = Tolerance::Default);
bool contains(const Circle& circle, const Point& p, const Tolerance& tol = Tolerance::Default);
bool contains(const Ellipse& ellipse, const Point& p, const Tolerance& tol = Tolerance::Default);
bool contains(const Polygon& poly, const Point& p, const Tolerance& tol = Tolerance::Default);
bool contains(const Circle& outer, const Circle& inner, const Tolerance& tol = Tolerance::Default);

// ============================================================================
// 十一、曲线细分
// ============================================================================

std::vector<Segment> subdivide(const Circle& circle, int segments);
std::vector<Segment> subdivide(const Ellipse& ellipse, int segments);
std::vector<Segment> subdivide(const Arc& arc, int segments);
std::vector<Segment> subdivide(const BezierCurve& curve, int segments);
std::vector<Segment> subdivide(const BSplineCurve& curve, int segments);
std::vector<Segment> subdivide(const NURBSCurve& curve, int segments);

// ============================================================================
// 十二、几何变换
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
// 十三、几何变换之镜像变换
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
// 十四、辅助函数
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