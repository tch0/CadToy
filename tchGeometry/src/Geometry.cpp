// 对应头文件
#include "Geometry.h"

// C++ 标准库

// 第三方库
#include <glm/gtx/norm.hpp>


namespace tch {
namespace Geometry {

// ============================================================================
// Circle 成员函数实现
// ============================================================================

Point Circle::pointAt(double angle) const {
    // 构建局部坐标系：u 和 v 是圆平面内的两个正交基向量
    Vector u, v;
    
    // 判断法向量是否平行于 Z 轴
    if (isParallel(normal, Vector(0, 0, 1))) {
        // 法向量平行于 Z 轴，使用 Y 轴作为参考
        u = glm::normalize(glm::cross(Vector(0, 1, 0), normal));
    } else {
        // 通用情况：使用 Z 轴作为参考
        u = glm::normalize(glm::cross(Vector(0, 0, 1), normal));
    }
    v = glm::normalize(glm::cross(normal, u));
    
    // 圆心 + (cosθ·u + sinθ·v) * 半径
    return center + (u * cos(angle) + v * sin(angle)) * radius;
}

double Circle::pointToParam(const Point& p) const {
    Vector local = p - center;
    
    // 构建局部坐标系
    Vector u, v;
    if (isParallel(normal, Vector(0, 0, 1))) {
        u = glm::normalize(glm::cross(Vector(0, 1, 0), normal));
    } else {
        u = glm::normalize(glm::cross(Vector(0, 0, 1), normal));
    }
    v = glm::normalize(glm::cross(normal, u));
    
    // 将点投影到局部坐标系
    double x = glm::dot(local, u);
    double y = glm::dot(local, v);
    
    // 计算参数角
    double angle = atan2(y, x);
    if (angle < 0) {
        angle += 2 * PI;
    }
    return angle;
}

// ============================================================================
// Arc 成员函数实现
// ============================================================================

double Arc::length() const {
    // 逆时针方向：从 startAngle 到 endAngle
    // 角度差 = endAngle - startAngle，若 < 0 则加 2π（跨零）
    double angleSpan = endAngle - startAngle;
    if (angleSpan < 0) {
        angleSpan += TWO_PI;
    }
    return radius * angleSpan;
}

double Arc::area() const {
    // 扇形面积 = 0.5 * r² * θ
    double angleSpan = endAngle - startAngle;
    if (angleSpan < 0) {
        angleSpan += TWO_PI;
    }
    return 0.5 * radius * radius * angleSpan;
}

Point Arc::pointAt(double t) const {
    // 参数 t∈[0,1] 对应参数角从 startAngle 逆时针到 endAngle
    double angleSpan = endAngle - startAngle;
    if (angleSpan < 0) {
        angleSpan += TWO_PI;
    }
    double angle = startAngle + t * angleSpan;
    
    // 构建局部坐标系（同 Circle）
    Vector u, v;
    if (isParallel(normal, Vector(0, 0, 1))) {
        u = glm::normalize(glm::cross(Vector(0, 1, 0), normal));
    } else {
        u = glm::normalize(glm::cross(Vector(0, 0, 1), normal));
    }
    v = glm::normalize(glm::cross(normal, u));
    
    return center + (u * cos(angle) + v * sin(angle)) * radius;
}

double Arc::pointToParam(const Point& p) const {
    Vector local = p - center;
    
    // 构建局部坐标系
    Vector u, v;
    if (isParallel(normal, Vector(0, 0, 1))) {
        u = glm::normalize(glm::cross(Vector(0, 1, 0), normal));
    } else {
        u = glm::normalize(glm::cross(Vector(0, 0, 1), normal));
    }
    v = glm::normalize(glm::cross(normal, u));
    
    // 将点投影到局部坐标系
    double x = glm::dot(local, u);
    double y = glm::dot(local, v);
    
    // 计算参数角
    double angle = atan2(y, x);
    if (angle < 0) {
        angle += 2 * PI;
    }
    return angle;
}

// ============================================================================
// Ellipse 成员函数实现
// ============================================================================

std::pair<Vector, Vector> Ellipse::getLocalAxes() const {
    // 构建椭圆所在平面的局部坐标系
    // u, v 是平面内的两个正交基向量，垂直于法向量 normal
    Vector u, v;
    if (isParallel(normal, Vector(0, 0, 1))) {
        // 法向量平行于 Z 轴，使用 Y 轴作为参考
        u = glm::normalize(glm::cross(Vector(0, 1, 0), normal));
    } else {
        // 通用情况：使用 Z 轴作为参考
        u = glm::normalize(glm::cross(Vector(0, 0, 1), normal));
    }
    v = glm::normalize(glm::cross(normal, u));
    
    // 应用椭圆自身的旋转角，得到旋转后的轴向量
    double cosRot = cos(rotation);
    double sinRot = sin(rotation);
    Vector u_rot = u * cosRot + v * sinRot;
    Vector v_rot = -u * sinRot + v * cosRot;
    
    return {u_rot, v_rot};
}

Point Ellipse::pointAt(double t) const {
    // 参数 t∈[0,1] 对应参数角从 startParam 逆时针到 endParam
    double angleSpan = endParam - startParam;
    if (angleSpan < 0) {
        angleSpan += TWO_PI;
    }
    double angle = startParam + t * angleSpan;
    auto [u_rot, v_rot] = getLocalAxes();
    // 标准椭圆参数方程：(rx*cosθ, ry*sinθ)
    double x = radiusX * cos(angle);
    double y = radiusY * sin(angle);
    return center + u_rot * x + v_rot * y;
}

double Ellipse::length() const {
    if (isFull()) {
        // 完整椭圆：拉马努金近似公式
        // 周长 ≈ π(a+b) * (1 + 3h/(10 + sqrt(4-3h)))，其中 h = (a-b)²/(a+b)²
        double h = (radiusX - radiusY) * (radiusX - radiusY) / ((radiusX + radiusY) * (radiusX + radiusY));
        return PI * (radiusX + radiusY) * (1 + 3 * h / (10 + sqrt(4 - 3 * h)));
    } else {
        // 椭圆弧长：按参数角均匀采样近似
        const int segments = 64;
        double len = 0.0;
        Point prev = pointAt(0.0);
        for (int i = 1; i <= segments; ++i) {
            double t = (double)i / segments;
            Point curr = pointAt(t);
            len += glm::distance(prev, curr);
            prev = curr;
        }
        return len;
    }
}

double Ellipse::pointToParam(const Point& p) const {
    auto [u_rot, v_rot] = getLocalAxes();
    
    // 将点变换到椭圆局部坐标系
    Vector local = p - center;
    double x = glm::dot(local, u_rot);
    double y = glm::dot(local, v_rot);
    
    // 计算参数角（考虑椭圆缩放）
    double angle = atan2(y / radiusY, x / radiusX);
    if (angle < 0) {
        angle += 2 * PI;
    }
    return angle;
}

// ============================================================================
// BezierCurve 成员函数实现
// ============================================================================

Point BezierCurve::evaluate(double t) const {
    if (controlPoints.empty()) {
        return Point(0, 0, 0);
    }
    
    int n = degree;
    std::vector<Point> points = controlPoints;
    
    // de Casteljau 算法：重复线性插值直到只剩一个点
    for (int r = 1; r <= n; ++r) {
        for (int i = 0; i <= n - r; ++i) {
            points[i] = points[i] * (1 - t) + points[i + 1] * t;
        }
    }
    
    return points[0];
}

Point BezierCurve::derivative(double t) const {
    if (degree <= 0 || controlPoints.size() < 2) {
        return Point(0, 0, 0);
    }
    
    // 导数曲线的控制点：ΔP_i = (P_{i+1} - P_i) * degree
    std::vector<Point> derivPoints;
    for (int i = 0; i < degree; ++i) {
        derivPoints.push_back((controlPoints[i + 1] - controlPoints[i]) * (double)degree);
    }
    
    BezierCurve derivCurve(degree - 1, derivPoints);
    return derivCurve.evaluate(t);
}

std::vector<BezierCurve> BezierCurve::subdivide(double t) const {
    if (controlPoints.empty()) { return {}; }
    
    int n = degree;
    // points[r][i] 表示第 r 层第 i 个插值点
    std::vector<std::vector<Point>> points(degree + 1);
    points[0] = controlPoints;
    
    // de Casteljau 递推
    for (int r = 1; r <= n; ++r) {
        points[r].resize(n - r + 1);
        for (int i = 0; i <= n - r; ++i) {
            points[r][i] = points[r - 1][i] * (1 - t) + points[r - 1][i + 1] * t;
        }
    }
    
    // 左侧曲线：取每层第一个点
    std::vector<Point> leftPoints;
    for (int i = 0; i <= n; ++i) {
        leftPoints.push_back(points[i][0]);
    }
    
    // 右侧曲线：取每层最后一个点（倒序）
    std::vector<Point> rightPoints;
    for (int i = 0; i <= n; ++i) {
        rightPoints.push_back(points[n - i][i]);
    }
    
    return {BezierCurve(degree, leftPoints), BezierCurve(degree, rightPoints)};
}

// ============================================================================
// BSplineCurve 成员函数实现
// ============================================================================

Point BSplineCurve::evaluate(double t) const {
    if (controlPoints.empty()) {
        return Point(0, 0, 0);
    }
    
    // 注意：调用者需确保节点向量合法
    // 1. 节点向量必须非递减：knots[i] <= knots[i+1]
    // 2. 首尾节点重复度应为 degree+1
    // 3. 分母 knots[i+degree-r+1] - knots[i] 应大于 0
    //    若等于 0，说明节点重复度过高，曲线在该处不连续
    
    // 节点向量范围：[knots[degree], knots[knots.size() - degree - 1]]
    int n = (int)knots.size() - degree - 1;
    
    // 查找 t 所在的节点区间
    int span = degree;
    for (int i = degree; i < n; ++i) {
        // 使用 <= 包含右端点，确保 t == knots[n] 时也能匹配
        if (t >= knots[i] && t <= knots[i + 1]) {
            span = i;
            break;
        }
    }
    
    // Cox-de Boor 递归算法
    // temp 存储当前层的控制点，逐层插值
    std::vector<Point> temp = controlPoints;
    for (int r = 1; r <= degree; ++r) {
        // i 从当前区间终点向下遍历
        for (int i = span; i >= span - degree + r; --i) {
            // 计算插值系数 alpha
            double alpha = (t - knots[i]) / (knots[i + degree - r + 1] - knots[i]);
            // 线性插值
            temp[i] = temp[i] * (1 - alpha) + temp[i - 1] * alpha;
        }
    }
    
    // 返回最终插值结果
    return temp[span];
}

// ============================================================================
// NURBSCurve 成员函数实现
// ============================================================================

Point NURBSCurve::evaluate(double t) const {
    if (controlPoints.empty()) { return Point(0, 0, 0); }
    
    // 加权控制点：P_i * w_i
    std::vector<Point> weightedPoints;
    for (size_t i = 0; i < controlPoints.size(); ++i) {
        weightedPoints.push_back(controlPoints[i] * weights[i]);
    }
    
    // 计算加权点的 B 样条值
    BSplineCurve weightedCurve(degree, knots, weightedPoints);
    Point num = weightedCurve.evaluate(t);
    
    // 计算权重的 B 样条值（作为三维点，xyz 都等于权重）
    std::vector<Point> weightPoints;
    for (double w : weights) {
        weightPoints.push_back(Point(w, w, w));
    }
    BSplineCurve weightCurve(degree, knots, weightPoints);
    Point denom = weightCurve.evaluate(t);
    
    // 有理除法：P = (加权和) / (权重和)
    if (std::abs(denom.x) < 1e-12) { return Point(0, 0, 0); }
    return Point(num.x / denom.x, num.y / denom.y, num.z / denom.z);
}

// ============================================================================
// Polyline 成员函数实现
// ============================================================================

double Polyline::length() const {
    double len = 0.0;
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        len += glm::distance(points[i], points[i + 1]);
    }
    if (closed && points.size() > 1) {
        len += glm::distance(points.back(), points.front());
    }
    return len;
}

std::vector<Segment> Polyline::toSegments() const {
    std::vector<Segment> segs;
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        segs.emplace_back(points[i], points[i + 1]);
    }
    if (closed && points.size() > 1) {
        segs.emplace_back(points.back(), points.front());
    }
    return segs;
}

// ============================================================================
// Rect 成员函数实现
// ============================================================================

std::vector<Point> Rect::vertices() const {
    // 左下、右下、右上、左上
    return {
        min,
        Point(max.x, min.y, min.z),
        max,
        Point(min.x, max.y, min.z)
    };
}

std::vector<Segment> Rect::edges() const {
    auto verts = vertices();
    std::vector<Segment> edges;
    for (int i = 0; i < 4; ++i) {
        edges.emplace_back(verts[i], verts[(i + 1) % 4]);
    }
    return edges;
}

// ============================================================================
// RegularPolygon 成员函数实现
// ============================================================================

std::vector<Point> RegularPolygon::vertices() const {
    std::vector<Point> verts;
    for (int i = 0; i < sides; ++i) {
        double angle = rotation + 2.0 * PI * i / sides;
        verts.emplace_back(
            center.x + radius * cos(angle),
            center.y + radius * sin(angle),
            center.z
        );
    }
    return verts;
}

double RegularPolygon::area() const {
    // 面积 = (n × r² × sin(2π/n)) / 2
    return 0.5 * sides * radius * radius * sin(2.0 * PI / sides);
}

// ============================================================================
// AABB 成员函数实现
// ============================================================================

void AABB::expand(const Point& p) {
    min.x = std::min(min.x, p.x);
    min.y = std::min(min.y, p.y);
    min.z = std::min(min.z, p.z);
    max.x = std::max(max.x, p.x);
    max.y = std::max(max.y, p.y);
    max.z = std::max(max.z, p.z);
}

void AABB::merge(const AABB& other) {
    expand(other.min);
    expand(other.max);
}

bool AABB::contains(const Point& p, const Tolerance& tol) const {
    return p.x >= min.x - tol.absolute && p.x <= max.x + tol.absolute &&
           p.y >= min.y - tol.absolute && p.y <= max.y + tol.absolute &&
           p.z >= min.z - tol.absolute && p.z <= max.z + tol.absolute;
}

bool AABB::intersects(const AABB& other) const {
    return !(max.x < other.min.x || min.x > other.max.x ||
             max.y < other.min.y || min.y > other.max.y ||
             max.z < other.min.z || min.z > other.max.z);
}

// ============================================================================
// 距离计算实现
// ============================================================================

double distance(const Point& p, const Segment& seg) {
    // 计算投影参数 t = (AP·AB) / |AB|²，限制在 [0,1]
    Vector ab = seg.end - seg.start;
    Vector ap = p - seg.start;
    
    double t = glm::dot(ap, ab) / glm::dot(ab, ab);
    t = std::max(0.0, std::min(1.0, t));
    
    Point closest = seg.start + ab * t;
    return glm::distance(p, closest);
}

double distance(const Point& p, const Line& line) {
    // 点到直线距离 = |(P - O) × D|
    Vector w = p - line.origin;
    return glm::length(glm::cross(w, line.direction));
}

double distance(const Point& p, const Ray& ray) {
    // 投影参数 t 限制为 t≥0
    Vector w = p - ray.origin;
    double t = glm::dot(w, ray.direction);
    if (t < 0) { t = 0; }
    Point closest = ray.origin + ray.direction * t;
    return glm::distance(p, closest);
}

double distance(const Point& p, const Circle& circle) {
    // 点到圆的距离 = |点到圆心距离 - 半径|
    double d = glm::distance(p, circle.center);
    return std::abs(d - circle.radius);
}

double distance(const Point& p, const Ellipse& ellipse) {
    // TODO: 实现精确的椭圆距离计算，当前为近似值
    return std::abs(glm::distance(p, ellipse.center) - ellipse.radiusX);
}

double distance(const Segment& a, const Segment& b) {
    // 先判断是否相交
    auto result = intersect(a, b);
    if (result.hasIntersection()) { return 0.0; }
    
    // 否则计算各端点到另一线段的最小距离
    double d1 = distance(a.start, b);
    double d2 = distance(a.end, b);
    double d3 = distance(b.start, a);
    double d4 = distance(b.end, a);
    return std::min({d1, d2, d3, d4});
}

double distance(const Segment& seg, const Circle& circle) {
    // 线段到圆的距离 = 线段上离圆心最近的点到圆的距离
    Point closest = closestPoint(circle.center, seg);
    return std::abs(glm::distance(closest, circle.center) - circle.radius);
}

// ============================================================================
// 投影计算实现
// ============================================================================

Point project(const Point& p, const Line& line) {
    // 投影点 = O + ( (P-O)·D ) * D
    Vector w = p - line.origin;
    double t = glm::dot(w, line.direction);
    return line.origin + line.direction * t;
}

Point project(const Point& p, const Segment& seg) {
    // 投影参数 t 限制在 [0,1]
    Vector ab = seg.end - seg.start;
    Vector ap = p - seg.start;
    
    double t = glm::dot(ap, ab) / glm::dot(ab, ab);
    t = std::max(0.0, std::min(1.0, t));
    
    return seg.start + ab * t;
}

Point closestPoint(const Point& p, const Segment& seg) {
    return project(p, seg);
}

Point closestPoint(const Point& p, const Circle& circle) {
    // 圆上离点最近的点 = 圆心 + 方向 × 半径
    // 若点在圆心，返回圆上任意点（取 (radius, 0, 0) 方向）
    Vector diff = p - circle.center;
    if (isZeroVector(diff)) {
        // 点在圆心，返回圆上第一个点
        return circle.pointAt(0.0);
    }
    Vector dir = glm::normalize(diff);
    return circle.center + dir * circle.radius;
}

// ============================================================================
// 共线/共面检查
// ============================================================================

/// 判断点是否在无限直线上（直线由原点+方向定义）
bool isPointOnLine(const Point& p, const Point& lineOrigin, const Vector& lineDirection,
                   const Tolerance& tol) {
    Vector w = p - lineOrigin;
    Vector cross = glm::cross(w, lineDirection);
    double cross_len = glm::length(cross);
    double w_len = glm::length(w);
    double dir_len = glm::length(lineDirection);
    
    // 取绝对精度与相对精度的较大值，适用于靠近原点或方向向量很短的情况
    double tolerance = std::max(tol.absolute,
                                tol.relative * w_len * dir_len);
    return cross_len < tolerance;
}

/// 判断两条直线是否共线
bool isCollinearLines(const Point& origin1, const Vector& dir1,
                      const Point& origin2, const Vector& dir2) {
    // 方向必须平行
    if (!isParallel(dir1, dir2)) {
        return false;
    }
    // 原点2必须在直线1上
    return isPointOnLine(origin2, origin1, dir1);
}

// 两条直线共面（直线由原点+方向定义）
bool isCoplanarLines(const Point& origin1, const Vector& dir1,
                     const Point& origin2, const Vector& dir2) {
    // 方向平行则必然共面
    if (isParallel(dir1, dir2)) {
        return true;
    }
    
    // 不平行，检查是否共面
    Vector w = origin2 - origin1;
    Vector n = glm::cross(dir1, dir2);
    double w_len = glm::length(w);
    double n_len = glm::length(n);
    double relativeTol = Tolerance::Default.relative * w_len * n_len;
    return std::abs(glm::dot(w, n)) < relativeTol;
}

// 直线与圆/椭圆共面
bool isCoplanarLineCurve(const Point& center, const Vector& normal,
                         const Point& lineOrigin, const Vector& lineDirection) {
    // 法向量与直线方向垂直（值域 [-1,1]，使用绝对精度）
    if (std::abs(glm::dot(normal, lineDirection)) > Tolerance::Default.absolute) {
        return false;
    }
    // 中心到直线的距离为0（使用相对精度）
    Vector w = center - lineOrigin;
    double cross_len = glm::length(glm::cross(w, lineDirection));
    double w_len = glm::length(w);
    double dir_len = glm::length(lineDirection);
    double relativeTol = Tolerance::Default.relative * w_len * dir_len;
    return cross_len < relativeTol;
}

// 两个圆/椭圆共面
bool isCoplanarCurves(const Point& center1, const Vector& normal1,
                      const Point& center2, const Vector& normal2) {
    // 法向量平行
    if (!isParallel(normal1, normal2)) {
        return false;
    }
    // 中心到另一平面的距离为0（使用相对精度）
    double dist = std::abs(glm::dot(center1 - center2, normal1));
    double center_len = glm::length(center1);
    double relativeTol = Tolerance::Default.relative * std::max(1.0, center_len);
    return dist < relativeTol;
}

// ============================================================================
// 交点计算实现
// ============================================================================

// 计算共线情况下两参数区间 [a1, a2] 和 [b1, b2] 的重叠
// 返回重叠区间 [start, end]，若无重叠返回 false
static bool computeOverlap(double a1, double a2, double b1, double b2,
                           double& start, double& end) {
    // 确保 a1 <= a2, b1 <= b2
    if (a1 > a2) { std::swap(a1, a2); }
    if (b1 > b2) { std::swap(b1, b2); }
    
    start = std::max(a1, b1);
    end = std::min(a2, b2);
    
    // 使用相对精度判断重叠
    double range = std::max(a2 - a1, b2 - b1);
    double tol = Tolerance::Default.relative * std::max(1.0, range);
    
    if (end < start - tol) {
        return false;  // 无重叠
    }
    
    // 修正边界
    if (end < start) {
        start = end = (start + end) * 0.5;
    }
    return true;
}

// 线段与线段求交（支持 3D，自动处理共面检查）
LineIntersectionResult intersect(const Segment& a, const Segment& b) {
    LineIntersectionResult result;
    
    Vector ab = a.end - a.start;
    Vector cd = b.end - b.start;
    Vector ac = b.start - a.start;
    
    double ab_len = glm::length(ab);
    double cd_len = glm::length(cd);
    
    Vector n = glm::cross(ab, cd);
    double n_len = glm::length(n);
    
    // 检查是否接近平行
    if (isParallel(ab, cd)) {
        // 平行，检查是否共线
        double ac_len = glm::length(ac);
        double collinearTol = Tolerance::Default.relative * ac_len * ab_len;
        if (glm::length(glm::cross(ac, ab)) > collinearTol) {
            return result;  // 平行但不共线
        }
        
        // 共线，计算参数区间重叠
        // 将端点投影到 a 的参数线上
        double t1 = 0.0;
        double t2 = 1.0;
        double t3 = glm::dot(b.start - a.start, ab) / glm::dot(ab, ab);
        double t4 = glm::dot(b.end - a.start, ab) / glm::dot(ab, ab);
        
        double start, end;
        if (!computeOverlap(t1, t2, t3, t4, start, end)) {
            return result;  // 无重叠
        }
        
        // 检查是单点还是线段
        double tol = Tolerance::Default.relative;
        if (std::abs(end - start) < tol) {
            result.type = LineIntersectionResult::kSinglePoint;
            result.p1 = a.start + ab * start;
        } else {
            result.type = LineIntersectionResult::kOverlapSegment;
            result.p1 = a.start + ab * start;
            result.p2 = a.start + ab * end;
        }
        return result;
    }
    
    // 不平行，检查共面
    double ac_len = glm::length(ac);
    double coplanarTol = Tolerance::Default.relative * ac_len * n_len;
    if (std::abs(glm::dot(ac, n)) > coplanarTol) {
        return result;  // 异面
    }
    
    // 共面，投影到坐标平面用 2D 方法求解
    int projAxis = 0;
    if (std::abs(n.x) < std::abs(n.y)) {
        projAxis = 1;
    }
    if (std::abs(n[projAxis]) < std::abs(n.z)) {
        projAxis = 2;
    }
    
    int u = (projAxis + 1) % 3;
    int v = (projAxis + 2) % 3;
    
    auto get2D = [&](const Point& p) -> std::pair<double, double> {
        if (u == 0 && v == 1) {
            return {p.x, p.y};
        }
        if (u == 0 && v == 2) {
            return {p.x, p.z};
        }
        return {p.y, p.z};
    };
    
    auto [ax, ay] = get2D(a.start);
    auto [bx, by] = get2D(a.end);
    auto [cx, cy] = get2D(b.start);
    auto [dx, dy] = get2D(b.end);
    
    double cross_ab_cd = (bx - ax) * (dy - cy) - (by - ay) * (dx - cx);
    double crossTol = Tolerance::Default.relative * ab_len * cd_len;
    if (std::abs(cross_ab_cd) < crossTol) {
        return result;  // 数值问题，返回无交点
    }
    
    double t = ((cx - ax) * (dy - cy) - (cy - ay) * (dx - cx)) / cross_ab_cd;
    double u_param = ((cx - ax) * (by - ay) - (cy - ay) * (bx - ax)) / cross_ab_cd;
    
    if (t >= 0 && t <= 1 && u_param >= 0 && u_param <= 1) {
        result.type = LineIntersectionResult::kSinglePoint;
        result.p1 = a.start + ab * t;
    }
    return result;
}

// 线段与直线
LineIntersectionResult intersect(const Segment& seg, const Line& line) {
    LineIntersectionResult result;
    
    Vector ab = seg.end - seg.start;
    Vector dir = line.direction;
    Vector ac = line.origin - seg.start;
    
    double ab_len = glm::length(ab);
    double dir_len = glm::length(dir);
    
    // 检查是否接近平行
    if (isParallel(ab, dir)) {
        // 平行，检查是否共线
        double ac_len = glm::length(ac);
        double collinearTol = Tolerance::Default.relative * ac_len * ab_len;
        if (glm::length(glm::cross(ac, ab)) > collinearTol) {
            return result;  // 平行但不共线
        }
        // 共线，线段就是重叠部分
        result.type = LineIntersectionResult::kOverlapSegment;
        result.p1 = seg.start;
        result.p2 = seg.end;
        return result;
    }
    
    // 不平行，检查共面
    Vector n = glm::cross(ab, dir);
    double n_len = glm::length(n);
    double ac_len = glm::length(ac);
    double coplanarTol = Tolerance::Default.relative * ac_len * n_len;
    if (std::abs(glm::dot(ac, n)) > coplanarTol) {
        return result;  // 异面
    }
    
    // 投影到坐标平面求解
    int projAxis = 0;
    if (std::abs(n.x) < std::abs(n.y)) { projAxis = 1; }
    if (std::abs(n[projAxis]) < std::abs(n.z)) { projAxis = 2; }
    
    int u = (projAxis + 1) % 3;
    int v = (projAxis + 2) % 3;
    
    auto get2D = [&](const Point& p) -> std::pair<double, double> {
        if (u == 0 && v == 1) { return {p.x, p.y}; }
        if (u == 0 && v == 2) { return {p.x, p.z}; }
        return {p.y, p.z};
    };
    
    auto [ax, ay] = get2D(seg.start);
    auto [bx, by] = get2D(seg.end);
    auto [cx, cy] = get2D(line.origin);
    auto [dx, dy] = get2D(line.origin + line.direction);
    
    double cross_ab_dir = (bx - ax) * (dy - cy) - (by - ay) * (dx - cx);
    double crossTol = Tolerance::Default.relative * ab_len * dir_len;
    if (std::abs(cross_ab_dir) < crossTol) {
        return result;
    }
    
    double t = ((cx - ax) * (dy - cy) - (cy - ay) * (dx - cx)) / cross_ab_dir;
    if (t >= 0 && t <= 1) {
        result.type = LineIntersectionResult::kSinglePoint;
        result.p1 = seg.start + ab * t;
    }
    return result;
}

// 线段与射线
LineIntersectionResult intersect(const Segment& seg, const Ray& ray) {
    LineIntersectionResult result;
    
    Vector ab = seg.end - seg.start;
    Vector dir = ray.direction;
    Vector ac = ray.origin - seg.start;
    
    double ab_len = glm::length(ab);
    double dir_len = glm::length(dir);
    
    // 检查是否接近平行
    if (isParallel(ab, dir)) {
        // 平行，检查是否共线
        double ac_len = glm::length(ac);
        double collinearTol = Tolerance::Default.relative * ac_len * ab_len;
        if (glm::length(glm::cross(ac, ab)) > collinearTol) {
            return result;  // 平行但不共线
        }
        
        // 共线，计算参数区间重叠
        // 线段参数范围 [0, 1]，射线参数范围 [t_ray, +∞)
        double t_ray = glm::dot(ray.origin - seg.start, ab) / glm::dot(ab, ab);
        
        // 重叠区间是 [max(0, t_ray), 1] 与 [t_ray, +∞) 的交集
        double start = std::max(0.0, t_ray);
        double end = 1.0;
        
        if (start > end + Tolerance::Default.relative) {
            return result;  // 无重叠
        }
        
        if (std::abs(end - start) < Tolerance::Default.relative) {
            result.type = LineIntersectionResult::kSinglePoint;
            result.p1 = seg.start + ab * start;
        } else {
            result.type = LineIntersectionResult::kOverlapSegment;
            result.p1 = seg.start + ab * start;
            result.p2 = seg.start + ab * end;
        }
        return result;
    }
    
    // 不平行，检查共面
    Vector n = glm::cross(ab, dir);
    double n_len = glm::length(n);
    double ac_len = glm::length(ac);
    double coplanarTol = Tolerance::Default.relative * ac_len * n_len;
    if (std::abs(glm::dot(ac, n)) > coplanarTol) {
        return result;  // 异面
    }
    
    // 投影到坐标平面求解
    int projAxis = 0;
    if (std::abs(n.x) < std::abs(n.y)) { projAxis = 1; }
    if (std::abs(n[projAxis]) < std::abs(n.z)) { projAxis = 2; }
    
    int u = (projAxis + 1) % 3;
    int v = (projAxis + 2) % 3;
    
    auto get2D = [&](const Point& p) -> std::pair<double, double> {
        if (u == 0 && v == 1) { return {p.x, p.y}; }
        if (u == 0 && v == 2) { return {p.x, p.z}; }
        return {p.y, p.z};
    };
    
    auto [ax, ay] = get2D(seg.start);
    auto [bx, by] = get2D(seg.end);
    auto [cx, cy] = get2D(ray.origin);
    auto [dx, dy] = get2D(ray.origin + ray.direction);
    
    double cross_ab_dir = (bx - ax) * (dy - cy) - (by - ay) * (dx - cx);
    double crossTol = Tolerance::Default.relative * ab_len * dir_len;
    if (std::abs(cross_ab_dir) < crossTol) {
        return result;
    }
    
    double t = ((cx - ax) * (dy - cy) - (cy - ay) * (dx - cx)) / cross_ab_dir;
    double u_param = ((cx - ax) * (by - ay) - (cy - ay) * (bx - ax)) / cross_ab_dir;
    
    if (t >= 0 && u_param >= 0 && u_param <= 1) {
        result.type = LineIntersectionResult::kSinglePoint;
        result.p1 = ray.origin + dir * t;
    }
    return result;
}

// 直线与直线求交
LineIntersectionResult intersect(const Line& a, const Line& b) {
    LineIntersectionResult result;
    
    Vector da = a.direction;
    Vector db = b.direction;
    Vector w = b.origin - a.origin;
    
    double da_len = glm::length(da);
    double w_len = glm::length(w);
    
    // 检查是否接近平行
    if (isParallel(da, db)) {
        // 平行，检查是否共线
        double collinearTol = Tolerance::Default.relative * w_len * da_len;
        if (glm::length(glm::cross(w, da)) > collinearTol) {
            return result;  // 平行但不共线
        }
        // 共线
        result.type = LineIntersectionResult::kOverlapLine;
        result.p1 = a.origin;
        result.direction = a.direction;
        return result;
    }
    
    // 检查共面
    Vector n = glm::cross(da, db);
    double n_len = glm::length(n);
    double coplanarTol = Tolerance::Default.relative * w_len * n_len;
    if (std::abs(glm::dot(w, n)) > coplanarTol) {
        return result;  // 异面
    }
    
    // 共面，解方程求交点
    double t = glm::dot(glm::cross(w, db), n) / glm::dot(n, n);
    result.type = LineIntersectionResult::kSinglePoint;
    result.p1 = a.origin + da * t;
    return result;
}

// 直线与射线
LineIntersectionResult intersect(const Line& line, const Ray& ray) {
    LineIntersectionResult result;
    
    Vector da = line.direction;
    Vector db = ray.direction;
    Vector w = ray.origin - line.origin;
    
    double da_len = glm::length(da);
    double w_len = glm::length(w);
    
    // 检查是否接近平行
    if (isParallel(da, db)) {
        // 平行，检查是否共线
        double collinearTol = Tolerance::Default.relative * w_len * da_len;
        if (glm::length(glm::cross(w, da)) > collinearTol) {
            return result;  // 平行但不共线
        }
        
        // 共线，射线就是重叠部分
        result.type = LineIntersectionResult::kOverlapRay;
        result.p1 = ray.origin;
        result.direction = ray.direction;
        return result;
    }
    
    // 检查共面
    Vector n = glm::cross(da, db);
    double n_len = glm::length(n);
    double coplanarTol = Tolerance::Default.relative * w_len * n_len;
    if (std::abs(glm::dot(w, n)) > coplanarTol) {
        return result;  // 异面
    }
    
    // 共面，解方程求交点
    double t = glm::dot(glm::cross(w, db), n) / glm::dot(n, n);
    double u = glm::dot(glm::cross(w, da), n) / glm::dot(n, n);
    
    if (u >= 0) {
        result.type = LineIntersectionResult::kSinglePoint;
        result.p1 = line.origin + da * t;
    }
    return result;
}

// 射线与射线
LineIntersectionResult intersect(const Ray& a, const Ray& b) {
    LineIntersectionResult result;
    
    Vector da = a.direction;
    Vector db = b.direction;
    Vector w = b.origin - a.origin;
    
    double da_len = glm::length(da);
    double w_len = glm::length(w);
    
    // 检查是否接近平行
    if (isParallel(da, db)) {
        // 平行，检查是否共线
        double collinearTol = Tolerance::Default.relative * w_len * da_len;
        if (glm::length(glm::cross(w, da)) > collinearTol) {
            return result;  // 平行但不共线
        }
        
        // 共线，检查方向
        double dotDir = glm::dot(da, db);
        
        if (dotDir > 0) {
            // 同向：重叠是从较远的原点开始的射线
            double t_a = glm::dot(w, da);  // b.origin 在 a 上的参数
            double t_b = -glm::dot(w, db); // a.origin 在 b 上的参数
            
            if (t_a >= 0 && t_b >= 0) {
                // 两原点都在对方的射线上，重叠从较远的原点开始
                result.type = LineIntersectionResult::kOverlapRay;
                if (t_a > 0) {
                    result.p1 = b.origin;
                } else {
                    result.p1 = a.origin;
                }
                result.direction = a.direction;
            } else if (t_a >= 0) {
                // b.origin 在 a 上，a.origin 不在 b 上
                result.type = LineIntersectionResult::kOverlapRay;
                result.p1 = b.origin;
                result.direction = a.direction;
            } else if (t_b >= 0) {
                // a.origin 在 b 上，b.origin 不在 a 上
                result.type = LineIntersectionResult::kOverlapRay;
                result.p1 = a.origin;
                result.direction = a.direction;
            }
            // else: 两原点都在对方的反方向，无重叠
        } else {
            // 反向：重叠是线段（如果有的话）
            double t_a = glm::dot(w, da);  // b.origin 在 a 上的参数
            // a.origin 参数为 0，b.origin 参数为 t_a
            // a 方向正半轴，b 方向负半轴
            // 重叠条件：t_a >= 0
            if (t_a >= -Tolerance::Default.relative) {
                double start = std::max(0.0, 0.0);
                double end = t_a;
                if (end < start - Tolerance::Default.relative) {
                    return result;  // 无重叠
                }
                if (std::abs(end - start) < Tolerance::Default.relative) {
                    result.type = LineIntersectionResult::kSinglePoint;
                    result.p1 = a.origin;
                } else {
                    result.type = LineIntersectionResult::kOverlapSegment;
                    result.p1 = a.origin;
                    result.p2 = b.origin;
                }
            }
        }
        return result;
    }
    
    // 检查共面
    Vector n = glm::cross(da, db);
    double n_len = glm::length(n);
    double coplanarTol = Tolerance::Default.relative * w_len * n_len;
    if (std::abs(glm::dot(w, n)) > coplanarTol) {
        return result;  // 异面
    }
    
    // 共面，解方程求交点
    double t = glm::dot(glm::cross(w, db), n) / glm::dot(n, n);
    double u = glm::dot(glm::cross(w, da), n) / glm::dot(n, n);
    
    if (t >= 0 && u >= 0) {
        result.type = LineIntersectionResult::kSinglePoint;
        result.p1 = a.origin + da * t;
    }
    return result;
}

// 线段与圆
bool intersect(const Segment& seg, const Circle& circle, std::vector<Point>& out) {
    // 先求圆心到线段所在直线的投影点，再用勾股定理求交点
    Point proj = project(circle.center, Line(seg.start, seg.end - seg.start));
    double dist = glm::distance(proj, circle.center);
    
    if (dist > circle.radius + 1e-9) { return false; }
    
    double d = std::sqrt(circle.radius * circle.radius - dist * dist);
    Vector dir = glm::normalize(seg.end - seg.start);
    
    double t1 = glm::dot(proj - seg.start, dir) - d;
    double t2 = glm::dot(proj - seg.start, dir) + d;
    
    if (t1 >= 0 && t1 <= 1) { out.push_back(seg.start + dir * t1); }
    if (t2 >= 0 && t2 <= 1 && std::abs(t2 - t1) > 1e-9) { out.push_back(seg.start + dir * t2); }
    
    return !out.empty();
}

// 线段与椭圆（数值迭代法）
bool intersect(const Segment& seg, const Ellipse& ellipse, std::vector<Point>& out) {
    // 将线段用参数方程表示 P(t) = A + t·AB, t∈[0,1]
    // 代入椭圆隐式方程 f(t) = 0，使用 Newton-Raphson 迭代求解
    auto [u_rot, v_rot] = ellipse.getLocalAxes();
    
    Vector AB = seg.end - seg.start;
    Point A = seg.start;
    
    double rx2 = ellipse.radiusX * ellipse.radiusX;
    double ry2 = ellipse.radiusY * ellipse.radiusY;
    
    // 椭圆隐式方程 f(t) = (x/rx)² + (y/ry)² - 1，及其导数 f'(t)
    auto f = [&](double t, double& df) -> double {
        Point P = A + AB * t;
        Vector local = P - ellipse.center;
        double x = glm::dot(local, u_rot);
        double y = glm::dot(local, v_rot);
        
        double f_val = (x * x) / rx2 + (y * y) / ry2 - 1.0;
        
        double dx_dt = glm::dot(AB, u_rot);
        double dy_dt = glm::dot(AB, v_rot);
        double df_val = 2.0 * (x * dx_dt) / rx2 + 2.0 * (y * dy_dt) / ry2;
        
        df = df_val;
        return f_val;
    };
    
    const double step = 0.01;
    const int maxIter = 50;
    const double tol = 1e-12;
    
    for (double t = 0.0; t < 1.0 - step; t += step) {
        double df1, df2;
        double f1 = f(t, df1);
        double f2 = f(t + step, df2);
        
        if (f1 * f2 < 0) {
            double root = t + step * 0.5;
            for (int iter = 0; iter < maxIter; ++iter) {
                double df;
                double f_val = f(root, df);
                
                if (std::abs(f_val) < tol) { break; }
                if (std::abs(df) < 1e-12) { break; }
                
                double delta_t = -f_val / df;
                root += delta_t;
                if (std::abs(delta_t) < 1e-12) { break; }
            }
            
            if (root >= -1e-12 && root <= 1.0 + 1e-12) {
                double t_clamped = std::max(0.0, std::min(1.0, root));
                Point P = A + AB * t_clamped;
                out.push_back(P);
            }
        }
    }
    
    // 检查端点
    for (double t : {0.0, 1.0}) {
        double df;
        double f_val = f(t, df);
        if (std::abs(f_val) < 1e-9) {
            Point P = A + AB * t;
            out.push_back(P);
        }
    }
    
    // 去重
    std::sort(out.begin(), out.end(), [](const Point& a, const Point& b) {
        return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
    });
    out.erase(std::unique(out.begin(), out.end(),
        [](const Point& a, const Point& b) { return isCoincident(a, b); }), out.end());
    
    return !out.empty();
}

// 直线与圆
bool intersect(const Line& line, const Circle& circle, std::vector<Point>& out) {
    Point proj = project(circle.center, line);
    double dist = glm::distance(proj, circle.center);
    
    if (dist > circle.radius + 1e-9) { return false; }
    
    if (std::abs(dist - circle.radius) < 1e-9) {
        out.push_back(proj);
        return true;
    }
    
    double d = std::sqrt(circle.radius * circle.radius - dist * dist);
    Vector dir = line.direction;
    
    out.push_back(proj + dir * d);
    out.push_back(proj - dir * d);
    return true;
}

// 直线与椭圆（数值迭代法）
bool intersect(const Line& line, const Ellipse& ellipse, std::vector<Point>& out) {
    // 使用 Newton-Raphson 迭代求解直线参数方程与椭圆隐式方程的交点
    auto [u_rot, v_rot] = ellipse.getLocalAxes();
    
    Point O = line.origin;
    Vector D = line.direction;
    
    double rx2 = ellipse.radiusX * ellipse.radiusX;
    double ry2 = ellipse.radiusY * ellipse.radiusY;
    
    // 椭圆隐式方程 f(t) 及其导数 f'(t)
    auto f = [&](double t, double& df) -> double {
        Point P = O + D * t;
        Vector local = P - ellipse.center;
        double x = glm::dot(local, u_rot);
        double y = glm::dot(local, v_rot);
        
        double f_val = (x * x) / rx2 + (y * y) / ry2 - 1.0;
        
        double dx_dt = glm::dot(D, u_rot);
        double dy_dt = glm::dot(D, v_rot);
        double df_val = 2.0 * (x * dx_dt) / rx2 + 2.0 * (y * dy_dt) / ry2;
        
        df = df_val;
        return f_val;
    };
    
    // 先计算直线与椭圆所在平面的交点，确定搜索范围
    double denom = glm::dot(ellipse.normal, D);
    if (std::abs(denom) > 1e-12) {
        double t_plane = glm::dot(ellipse.center - O, ellipse.normal) / denom;
        double halfRange = std::max(ellipse.radiusX, ellipse.radiusY) * 2.0;
        double t_min = t_plane - halfRange;
        double t_max = t_plane + halfRange;
        
        const double step = 1.0;
        const int maxIter = 50;
        const double tol = 1e-12;
        
        for (double t = t_min; t < t_max; t += step) {
            double df1, df2;
            double f1 = f(t, df1);
            double f2 = f(t + step, df2);
            
            if (f1 * f2 < 0) {
                double root = t + step * 0.5;
                for (int iter = 0; iter < maxIter; ++iter) {
                    double df;
                    double f_val = f(root, df);
                    
                    if (std::abs(f_val) < tol) { break; }
                    if (std::abs(df) < 1e-12) { break; }
                    
                    double delta_t = -f_val / df;
                    root += delta_t;
                    if (std::abs(delta_t) < 1e-12) { break; }
                }
                
                Point P = O + D * root;
                out.push_back(P);
            }
        }
    }
    
    // 去重
    std::sort(out.begin(), out.end(), [](const Point& a, const Point& b) {
        return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
    });
    out.erase(std::unique(out.begin(), out.end(),
        [](const Point& a, const Point& b) { return isCoincident(a, b); }), out.end());
    
    return !out.empty();
}

// 射线与圆
bool intersect(const Ray& ray, const Circle& circle, std::vector<Point>& out) {
    // 解二次方程 |O + t·D - C|² = R²
    Vector oc = ray.origin - circle.center;
    double a = glm::dot(ray.direction, ray.direction);
    double b = 2.0 * glm::dot(oc, ray.direction);
    double c = glm::dot(oc, oc) - circle.radius * circle.radius;
    double disc = b * b - 4 * a * c;
    
    if (disc < -1e-9) { return false; }
    if (disc < 0) { disc = 0; }
    
    double sqrtDisc = std::sqrt(disc);
    double t1 = (-b - sqrtDisc) / (2 * a);
    double t2 = (-b + sqrtDisc) / (2 * a);
    
    if (t1 >= 0) { out.push_back(ray.origin + ray.direction * t1); }
    if (t2 >= 0 && std::abs(t2 - t1) > 1e-9) { out.push_back(ray.origin + ray.direction * t2); }
    
    return !out.empty();
}

// 射线与椭圆（数值迭代法）
bool intersect(const Ray& ray, const Ellipse& ellipse, std::vector<Point>& out) {
    // 使用 Newton-Raphson 迭代求解，参数 t≥0 限制在射线范围内
    auto [u_rot, v_rot] = ellipse.getLocalAxes();
    
    Point O = ray.origin;
    Vector D = ray.direction;
    
    double rx2 = ellipse.radiusX * ellipse.radiusX;
    double ry2 = ellipse.radiusY * ellipse.radiusY;
    
    // 椭圆隐式方程 f(t) 及其导数 f'(t)
    auto f = [&](double t, double& df) -> double {
        Point P = O + D * t;
        Vector local = P - ellipse.center;
        double x = glm::dot(local, u_rot);
        double y = glm::dot(local, v_rot);
        
        double f_val = (x * x) / rx2 + (y * y) / ry2 - 1.0;
        
        double dx_dt = glm::dot(D, u_rot);
        double dy_dt = glm::dot(D, v_rot);
        double df_val = 2.0 * (x * dx_dt) / rx2 + 2.0 * (y * dy_dt) / ry2;
        
        df = df_val;
        return f_val;
    };
    
    double denom = glm::dot(ellipse.normal, D);
    if (std::abs(denom) > 1e-12) {
        double t_plane = glm::dot(ellipse.center - O, ellipse.normal) / denom;
        
        if (t_plane >= 0) {
            double halfRange = std::max(ellipse.radiusX, ellipse.radiusY) * 2.0;
            double t_min = std::max(0.0, t_plane - halfRange);
            double t_max = t_plane + halfRange;
            
            const double step = 1.0;
            const int maxIter = 50;
            const double tol = 1e-12;
            
            for (double t = t_min; t < t_max; t += step) {
                double df1, df2;
                double f1 = f(t, df1);
                double f2 = f(t + step, df2);
                
                if (f1 * f2 < 0) {
                    double root = t + step * 0.5;
                    for (int iter = 0; iter < maxIter; ++iter) {
                        double df;
                        double f_val = f(root, df);
                        
                        if (std::abs(f_val) < tol) { break; }
                        if (std::abs(df) < 1e-12) { break; }
                        
                        double delta_t = -f_val / df;
                        root += delta_t;
                        if (std::abs(delta_t) < 1e-12) { break; }
                    }
                    
                    if (root >= 0) {
                        Point P = O + D * root;
                        out.push_back(P);
                    }
                }
            }
        }
    }
    
    // 检查端点 t=0
    double df;
    double f0 = f(0.0, df);
    if (std::abs(f0) < 1e-9) {
        out.push_back(ray.origin);
    }
    
    // 去重
    std::sort(out.begin(), out.end(), [](const Point& a, const Point& b) {
        return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
    });
    out.erase(std::unique(out.begin(), out.end(),
        [](const Point& a, const Point& b) { return isCoincident(a, b); }), out.end());
    
    return !out.empty();
}

// 圆与圆
bool intersect(const Circle& a, const Circle& b, std::vector<Point>& out) {
    // 使用余弦定理求交点
    Vector d = b.center - a.center;
    double dist = glm::length(d);
    
    if (dist > a.radius + b.radius + 1e-9) { return false; }  // 相离
    if (dist < std::abs(a.radius - b.radius) - 1e-9) { return false; } // 内含
    if (std::abs(dist) < 1e-9) { return false; } // 同心圆
    
    double a2 = (a.radius * a.radius - b.radius * b.radius + dist * dist) / (2 * dist);
    double h2 = a.radius * a.radius - a2 * a2;
    
    if (h2 < -1e-9) { return false; }
    if (h2 < 0) { h2 = 0; }
    
    double h = std::sqrt(h2);
    Vector mid = a.center + d * (a2 / dist);
    Vector perp = Vector(-d.y, d.x, 0);
    
    if (std::abs(h) < 1e-9) {
        out.push_back(mid);
    } else {
        out.push_back(mid + perp * (h / glm::length(perp)));
        out.push_back(mid - perp * (h / glm::length(perp)));
    }
    return true;
}

// 圆与椭圆（数值迭代法）
bool intersect(const Circle& circle, const Ellipse& ellipse, std::vector<Point>& out) {
    // 沿椭圆参数 t 遍历，求解椭圆上点到圆心距离等于半径的点
    auto [u_rot, v_rot] = ellipse.getLocalAxes();
    
    // f(t) = |椭圆上点 - 圆心|² - R²，求解 f(t) = 0
    auto f = [&](double t, double& df) -> double {
        double cosT = cos(t);
        double sinT = sin(t);
        
        double x_local = ellipse.radiusX * cosT;
        double y_local = ellipse.radiusY * sinT;
        Point p = ellipse.center + u_rot * x_local + v_rot * y_local;
        
        double dx = p.x - circle.center.x;
        double dy = p.y - circle.center.y;
        double dz = p.z - circle.center.z;
        
        double f_val = dx*dx + dy*dy + dz*dz - circle.radius * circle.radius;
        
        double dx_dt = -ellipse.radiusX * sinT;
        double dy_dt = ellipse.radiusY * cosT;
        Point dp = u_rot * dx_dt + v_rot * dy_dt;
        
        double df_val = 2.0 * (dx * dp.x + dy * dp.y + dz * dp.z);
        
        df = df_val;
        return f_val;
    };
    
    const double step = PI / 180.0;  // 1度步长
    const int maxIter = 50;
    const double tol = 1e-12;
    
    for (double t = 0; t < 2.0 * PI - step; t += step) {
        double df1, df2;
        double f1 = f(t, df1);
        double f2 = f(t + step, df2);
        
        if (f1 * f2 < 0) {
            double root = t + step * 0.5;
            for (int iter = 0; iter < maxIter; ++iter) {
                double df;
                double f_val = f(root, df);
                
                if (std::abs(f_val) < tol) { break; }
                if (std::abs(df) < 1e-12) { break; }
                
                double delta_t = -f_val / df;
                root += delta_t;
                if (std::abs(delta_t) < 1e-12) { break; }
            }
            
            double root_norm = fmod(root, 2.0 * PI);
            if (root_norm < 0) { root_norm += 2.0 * PI; }
            
            double cosRoot = cos(root_norm);
            double sinRoot = sin(root_norm);
            Point p = ellipse.center + u_rot * (ellipse.radiusX * cosRoot) 
                                   + v_rot * (ellipse.radiusY * sinRoot);
            out.push_back(p);
        }
    }
    
    // 去重
    std::sort(out.begin(), out.end(), [](const Point& a, const Point& b) {
        return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
    });
    out.erase(std::unique(out.begin(), out.end(),
        [](const Point& a, const Point& b) { return isCoincident(a, b); }), out.end());
    
    return !out.empty();
}

// 椭圆与椭圆（数值迭代法）
bool intersect(const Ellipse& a, const Ellipse& b, std::vector<Point>& out) {
    // 沿椭圆 a 参数 t 遍历，求解 a 上的点是否在椭圆 b 上
    auto [ua_rot, va_rot] = a.getLocalAxes();
    auto [ub_rot, vb_rot] = b.getLocalAxes();
    
    // f(t) = 椭圆 a 上的点代入椭圆 b 的隐式方程
    auto f = [&](double t, double& df) -> double {
        double cosT = cos(t);
        double sinT = sin(t);
        
        double xa_local = a.radiusX * cosT;
        double ya_local = a.radiusY * sinT;
        Point p = a.center + ua_rot * xa_local + va_rot * ya_local;
        
        Vector local = p - b.center;
        double xb = glm::dot(local, ub_rot);
        double yb = glm::dot(local, vb_rot);
        
        double f_val = (xb * xb) / (b.radiusX * b.radiusX) + 
                       (yb * yb) / (b.radiusY * b.radiusY) - 1.0;
        
        double dxa_dt = -a.radiusX * sinT;
        double dya_dt = a.radiusY * cosT;
        Point dp = ua_rot * dxa_dt + va_rot * dya_dt;
        
        double dxb_dt = glm::dot(dp, ub_rot);
        double dyb_dt = glm::dot(dp, vb_rot);
        
        double df_val = 2.0 * (xb * dxb_dt) / (b.radiusX * b.radiusX) +
                        2.0 * (yb * dyb_dt) / (b.radiusY * b.radiusY);
        
        df = df_val;
        return f_val;
    };
    
    const double step = PI / 180.0;
    const int maxIter = 50;
    const double tol = 1e-12;
    
    for (double t = 0; t < 2.0 * PI - step; t += step) {
        double df1, df2;
        double f1 = f(t, df1);
        double f2 = f(t + step, df2);
        
        if (f1 * f2 < 0) {
            double root = t + step * 0.5;
            for (int iter = 0; iter < maxIter; ++iter) {
                double df;
                double f_val = f(root, df);
                
                if (std::abs(f_val) < tol) { break; }
                if (std::abs(df) < 1e-12) { break; }
                
                double delta_t = -f_val / df;
                root += delta_t;
                if (std::abs(delta_t) < 1e-12) { break; }
            }
            
            double cosRoot = cos(root);
            double sinRoot = sin(root);
            Point p = a.center + ua_rot * (a.radiusX * cosRoot) 
                               + va_rot * (a.radiusY * sinRoot);
            out.push_back(p);
        }
    }
    
    // 去重
    std::sort(out.begin(), out.end(), [](const Point& a, const Point& b) {
        return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
    });
    out.erase(std::unique(out.begin(), out.end(),
        [](const Point& a, const Point& b) { return isCoincident(a, b); }), out.end());
    
    return !out.empty();
}

// ============================================================================
// 对称版本（调用已有实现）
// ============================================================================

bool intersect(const Circle& circle, const Line& line, std::vector<Point>& out) {
    return intersect(line, circle, out);
}

bool intersect(const Circle& circle, const Ray& ray, std::vector<Point>& out) {
    return intersect(ray, circle, out);
}

bool intersect(const Circle& circle, const Segment& seg, std::vector<Point>& out) {
    return intersect(seg, circle, out);
}

bool intersect(const Ellipse& ellipse, const Line& line, std::vector<Point>& out) {
    return intersect(line, ellipse, out);
}

bool intersect(const Ellipse& ellipse, const Ray& ray, std::vector<Point>& out) {
    return intersect(ray, ellipse, out);
}

bool intersect(const Ellipse& ellipse, const Segment& seg, std::vector<Point>& out) {
    return intersect(seg, ellipse, out);
}

bool intersect(const Ellipse& ellipse, const Circle& circle, std::vector<Point>& out) {
    return intersect(circle, ellipse, out);
}

// ============================================================================
// 包含性测试实现
// ============================================================================

bool contains(const Segment& seg, const Point& p, const Tolerance& tol) {
    // 点到线段距离（使用相对精度，基于线段长度）
    double d = distance(p, seg);
    double seg_len = seg.length();
    if (d > tol.relative * seg_len) {
        return false;
    }
    
    // 参数 t 范围（无量纲，用绝对精度）
    double t = glm::dot(p - seg.start, seg.end - seg.start) / glm::length2(seg.end - seg.start);
    return t >= -tol.absolute && t <= 1.0 + tol.absolute;
}

bool contains(const Ray& ray, const Point& p, const Tolerance& tol) {
    // 先检查点是否在直线上
    if (!isPointOnLine(p, ray.origin, ray.direction, tol)) {
        return false;
    }
    // 再检查参数 t 是否 >= 0
    double t = glm::dot(p - ray.origin, ray.direction) / glm::length2(ray.direction);
    return t >= -tol.absolute;
}

bool contains(const Line& line, const Point& p, const Tolerance& tol) {
    return isPointOnLine(p, line.origin, line.direction, tol);
}

bool contains(const Circle& circle, const Point& p, const Tolerance& tol) {
    double d = glm::distance(p, circle.center);
    double diff = std::abs(d - circle.radius);
    double scale = std::max(1.0, circle.radius);
    return diff < tol.relative * scale;
}

bool contains(const Ellipse& ellipse, const Point& p, const Tolerance& tol) {
    auto [u_rot, v_rot] = ellipse.getLocalAxes();
    
    // 将点变换到椭圆局部坐标系
    Vector local = p - ellipse.center;
    double x = glm::dot(local, u_rot);
    double y = glm::dot(local, v_rot);
    
    // 椭圆隐式方程 (x/rx)² + (y/ry)² ≤ 1 表示点在椭圆内
    double val = (x * x) / (ellipse.radiusX * ellipse.radiusX) + 
                 (y * y) / (ellipse.radiusY * ellipse.radiusY);
    return val <= 1.0 + tol.absolute;
}

bool contains(const Polygon& poly, const Point& p, const Tolerance& tol) {
    if (poly.points.size() < 3) { return false; }
    
    // 先检查是否在边上
    auto edges = poly.toSegments();
    for (const auto& edge : edges) {
        if (contains(edge, p, tol)) {
            return true;
        }
    }
    
    // 射线法判断内部
    bool inside = false;
    std::size_t n = poly.points.size();
    
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        const Point& pi = poly.points[i];
        const Point& pj = poly.points[j];
        
        bool yi_gt = pi.y > p.y + tol.absolute;
        bool yj_gt = pj.y > p.y + tol.absolute;
        
        if (yi_gt != yj_gt) {
            double x = pi.x + (p.y - pi.y) * (pj.x - pi.x) / (pj.y - pi.y);
            if (x > p.x + tol.absolute) {
                inside = !inside;
            }
        }
    }
    
    return inside;
}

bool contains(const Circle& outer, const Circle& inner, const Tolerance& tol) {
    double dist = glm::distance(outer.center, inner.center);
    return dist + inner.radius <= outer.radius + tol.absolute;
}

// ============================================================================
// 曲线细分实现
// ============================================================================

std::vector<Segment> subdivide(const Circle& circle, int segments) {
    std::vector<Segment> result;
    result.reserve(segments);
    
    for (int i = 0; i < segments; ++i) {
        double a1 = 2.0 * PI * i / segments;
        double a2 = 2.0 * PI * (i + 1) / segments;
        result.emplace_back(circle.pointAt(a1), circle.pointAt(a2));
    }
    return result;
}

std::vector<Segment> subdivide(const Ellipse& ellipse, int segments) {
    std::vector<Segment> result;
    result.reserve(segments);
    
    for (int i = 0; i < segments; ++i) {
        double t1 = (double)i / segments;
        double t2 = (double)(i + 1) / segments;
        result.emplace_back(ellipse.pointAt(t1), ellipse.pointAt(t2));
    }
    return result;
}

std::vector<Segment> subdivide(const Arc& arc, int segments) {
    std::vector<Segment> result;
    result.reserve(segments);
    
    for (int i = 0; i < segments; ++i) {
        double t1 = (double)i / segments;
        double t2 = (double)(i + 1) / segments;
        result.emplace_back(arc.pointAt(t1), arc.pointAt(t2));
    }
    return result;
}

std::vector<Segment> subdivide(const BezierCurve& curve, int segments) {
    std::vector<Segment> result;
    result.reserve(segments);
    
    Point prev = curve.evaluate(0.0);
    for (int i = 1; i <= segments; ++i) {
        double t = (double)i / segments;
        Point curr = curve.evaluate(t);
        result.emplace_back(prev, curr);
        prev = curr;
    }
    return result;
}

std::vector<Segment> subdivide(const BSplineCurve& curve, int segments) {
    std::vector<Segment> result;
    result.reserve(segments);
    
    Point prev = curve.evaluate(0.0);
    for (int i = 1; i <= segments; ++i) {
        double t = (double)i / segments;
        Point curr = curve.evaluate(t);
        result.emplace_back(prev, curr);
        prev = curr;
    }
    return result;
}

std::vector<Segment> subdivide(const NURBSCurve& curve, int segments) {
    std::vector<Segment> result;
    result.reserve(segments);
    
    Point prev = curve.evaluate(0.0);
    for (int i = 1; i <= segments; ++i) {
        double t = (double)i / segments;
        Point curr = curve.evaluate(t);
        result.emplace_back(prev, curr);
        prev = curr;
    }
    return result;
}

// ============================================================================
// 切线计算实现
// ============================================================================

Vector tangentAt(const Circle& circle, double angle) {
    // 圆上点的切线方向垂直于半径方向
    // 在圆的局部坐标系中，半径方向为 (cos(angle), sin(angle))
    // 切线方向为 (-sin(angle), cos(angle))，即逆时针旋转90度
    
    // 构建局部坐标系
    Vector u, v;
    if (isParallel(circle.normal, Vector(0, 0, 1))) {
        u = glm::normalize(glm::cross(Vector(0, 1, 0), circle.normal));
    } else {
        u = glm::normalize(glm::cross(Vector(0, 0, 1), circle.normal));
    }
    v = glm::normalize(glm::cross(circle.normal, u));
    
    // 切线方向：半径方向逆时针旋转90度
    return -u * std::sin(angle) + v * std::cos(angle);
}

Vector tangentAt(const Circle& circle, const Point& p) {
    // 计算点对应的角度
    Vector diff = p - circle.center;
    
    // 构建局部坐标系
    Vector u, v;
    if (isParallel(circle.normal, Vector(0, 0, 1))) {
        u = glm::normalize(glm::cross(Vector(0, 1, 0), circle.normal));
    } else {
        u = glm::normalize(glm::cross(Vector(0, 0, 1), circle.normal));
    }
    v = glm::normalize(glm::cross(circle.normal, u));
    
    // 计算角度
    double x = glm::dot(diff, u);
    double y = glm::dot(diff, v);
    double angle = std::atan2(y, x);
    
    return tangentAt(circle, angle);
}

std::vector<Point> tangentPointsFromPoint(const Circle& circle, const Point& externalPoint) {
    std::vector<Point> result;
    
    // 计算点到圆心的距离
    double dist = glm::distance(externalPoint, circle.center);
    
    // 点在圆内，无切点
    if (dist < circle.radius - Tolerance::Default.absolute) {
        return result;
    }
    
    // 点在圆上，只有一个切点（就是点本身）
    if (std::abs(dist - circle.radius) < Tolerance::Default.absolute) {
        result.push_back(externalPoint);
        return result;
    }
    
    // 点在圆外，有两个切点
    // 使用几何法：切点与圆心、外点形成直角三角形
    // 设切点为 T，则 |CT| = r，|PT| = sqrt(d² - r²)
    // 切点到圆心的距离为 r，到外点的距离为 sqrt(d² - r²)
    
    // 构建局部坐标系
    Vector u, v;
    if (isParallel(circle.normal, Vector(0, 0, 1))) {
        u = glm::normalize(glm::cross(Vector(0, 1, 0), circle.normal));
    } else {
        u = glm::normalize(glm::cross(Vector(0, 0, 1), circle.normal));
    }
    v = glm::normalize(glm::cross(circle.normal, u));
    
    // 外点在局部坐标系中的坐标
    Vector diff = externalPoint - circle.center;
    double px = glm::dot(diff, u);
    double py = glm::dot(diff, v);
    
    // 切点到圆心连线的角度
    double angleToCenter = std::atan2(py, px);
    
    // 切线与圆心连线的夹角
    double tangentAngle = std::acos(circle.radius / dist);
    
    // 两个切点对应的角度
    double angle1 = angleToCenter + tangentAngle;
    double angle2 = angleToCenter - tangentAngle;
    
    // 计算切点
    Point t1 = circle.center + u * (circle.radius * std::cos(angle1)) + v * (circle.radius * std::sin(angle1));
    Point t2 = circle.center + u * (circle.radius * std::cos(angle2)) + v * (circle.radius * std::sin(angle2));
    
    result.push_back(t1);
    result.push_back(t2);
    
    return result;
}

Vector tangentAt(const Ellipse& ellipse, double t) {
    // 椭圆参数方程的导数
    // x = rx * cos(θ), y = ry * sin(θ)
    // dx/dt = -rx * sin(θ) * dθ/dt
    // dy/dt = ry * cos(θ) * dθ/dt
    // 切线方向 = (-rx * sin(θ), ry * cos(θ))
    
    double angleSpan = ellipse.endParam - ellipse.startParam;
    if (angleSpan < 0) {
        angleSpan += TWO_PI;
    }
    double angle = ellipse.startParam + t * angleSpan;
    
    auto [u_rot, v_rot] = ellipse.getLocalAxes();
    
    // 切线方向（未归一化）
    Vector tangent = -u_rot * (ellipse.radiusX * std::sin(angle)) + v_rot * (ellipse.radiusY * std::cos(angle));
    
    return glm::normalize(tangent);
}

Vector tangentAt(const Ellipse& ellipse, const Point& p) {
    // 计算点对应的参数
    double t = ellipse.pointToParam(p);
    
    // 归一化到 [0, 1]
    double angleSpan = ellipse.endParam - ellipse.startParam;
    if (angleSpan < 0) {
        angleSpan += TWO_PI;
    }
    double normalizedT = (t - ellipse.startParam) / angleSpan;
    
    return tangentAt(ellipse, normalizedT);
}

// ============================================================================
// 角度计算实现
// ============================================================================

double angleBetween(const Vector& a, const Vector& b) {
    double lenA = glm::length(a);
    double lenB = glm::length(b);
    if (isZero(lenA) || isZero(lenB)) {
        return 0.0;
    }
    double dot = glm::dot(a, b) / (lenA * lenB);
    dot = std::max(-1.0, std::min(1.0, dot));  // 防止浮点误差
    return std::acos(dot);
}

double angleAt(const Point& a, const Point& b, const Point& c) {
    Vector ba = a - b;
    Vector bc = c - b;
    return angleBetween(ba, bc);
}

double angleOf(const Vector& v) {
    double angle = std::atan2(v.y, v.x);
    if (angle < 0) {
        angle += TWO_PI;
    }
    return angle;
}

} // namespace Geometry
} // namespace tch