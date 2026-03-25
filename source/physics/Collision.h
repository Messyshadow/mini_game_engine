/**
 * Collision.h - 碰撞检测工具函数
 * 
 * ============================================================================
 * 碰撞检测系统概述
 * ============================================================================
 * 
 * 这个文件包含各种碰撞形状之间的检测函数。
 * 
 * 支持的碰撞对：
 * - AABB vs AABB
 * - Circle vs Circle
 * - AABB vs Circle
 * - Point vs AABB
 * - Point vs Circle
 * - Ray vs AABB
 * - Ray vs Circle
 * 
 * ============================================================================
 * 碰撞结果结构
 * ============================================================================
 * 
 * HitResult 包含碰撞的详细信息：
 * - hit: 是否发生碰撞
 * - point: 碰撞点
 * - normal: 碰撞法线（用于反弹）
 * - penetration: 穿透深度
 * - t: 射线参数（用于射线检测）
 */

#ifndef MINI_ENGINE_COLLISION_H
#define MINI_ENGINE_COLLISION_H

#include "math/Math.h"
#include "AABB.h"
#include "Circle.h"

namespace MiniEngine {

// ============================================================================
// 碰撞结果结构
// ============================================================================

/**
 * 碰撞检测结果
 */
struct HitResult {
    bool hit = false;           // 是否发生碰撞
    Math::Vec2 point;           // 碰撞点
    Math::Vec2 normal;          // 碰撞法线（从被撞物体指向外）
    float penetration = 0.0f;   // 穿透深度
    float t = 0.0f;             // 射线参数 (0-1范围)
    
    /** 创建一个"未碰撞"的结果 */
    static HitResult NoHit() {
        return HitResult();
    }
};

// ============================================================================
// 射线结构
// ============================================================================

/**
 * 2D射线
 * 
 * 射线由起点和方向定义：
 *   P(t) = origin + direction * t
 *   其中 t >= 0
 */
struct Ray2D {
    Math::Vec2 origin;      // 起点
    Math::Vec2 direction;   // 方向（应为单位向量）
    
    Ray2D() : origin(0, 0), direction(1, 0) {}
    
    Ray2D(const Math::Vec2& origin, const Math::Vec2& direction)
        : origin(origin)
        , direction(direction.Normalized())
    {}
    
    /** 获取射线上的点 */
    Math::Vec2 GetPoint(float t) const {
        return origin + direction * t;
    }
};

// ============================================================================
// 碰撞检测命名空间
// ============================================================================

namespace Collision {

// ========================================================================
// AABB vs AABB
// ========================================================================

/**
 * 检测两个AABB是否碰撞
 * 
 * @param a 第一个AABB
 * @param b 第二个AABB
 * @return true如果碰撞
 */
inline bool TestAABBvsAABB(const AABB& a, const AABB& b) {
    return a.Intersects(b);
}

/**
 * 检测两个AABB碰撞并返回详细结果
 * 
 * @param a 第一个AABB（移动物体）
 * @param b 第二个AABB（静态物体）
 * @return 碰撞结果
 */
inline HitResult AABBvsAABB(const AABB& a, const AABB& b) {
    HitResult result;
    
    // 计算重叠量
    float dx = b.center.x - a.center.x;
    float dy = b.center.y - a.center.y;
    
    float overlapX = (a.halfSize.x + b.halfSize.x) - std::abs(dx);
    float overlapY = (a.halfSize.y + b.halfSize.y) - std::abs(dy);
    
    // 检查是否有重叠
    if (overlapX <= 0 || overlapY <= 0) {
        return result;  // 不碰撞
    }
    
    result.hit = true;
    
    // 选择穿透最小的轴
    if (overlapX < overlapY) {
        // X轴穿透更小
        result.penetration = overlapX;
        result.normal = (dx > 0) ? Math::Vec2(-1, 0) : Math::Vec2(1, 0);
        
        // 碰撞点在两个AABB边界的中间
        float contactX = (dx > 0) ? 
            (a.center.x + a.halfSize.x + b.center.x - b.halfSize.x) * 0.5f :
            (a.center.x - a.halfSize.x + b.center.x + b.halfSize.x) * 0.5f;
        result.point = Math::Vec2(contactX, a.center.y);
    } else {
        // Y轴穿透更小
        result.penetration = overlapY;
        result.normal = (dy > 0) ? Math::Vec2(0, -1) : Math::Vec2(0, 1);
        
        float contactY = (dy > 0) ?
            (a.center.y + a.halfSize.y + b.center.y - b.halfSize.y) * 0.5f :
            (a.center.y - a.halfSize.y + b.center.y + b.halfSize.y) * 0.5f;
        result.point = Math::Vec2(a.center.x, contactY);
    }
    
    return result;
}

// ========================================================================
// Circle vs Circle
// ========================================================================

/**
 * 检测两个圆是否碰撞
 */
inline bool TestCirclevsCircle(const Circle& a, const Circle& b) {
    return a.Intersects(b);
}

/**
 * 检测两个圆碰撞并返回详细结果
 */
inline HitResult CirclevsCircle(const Circle& a, const Circle& b) {
    HitResult result;
    
    float dx = b.center.x - a.center.x;
    float dy = b.center.y - a.center.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = a.radius + b.radius;
    
    if (distanceSquared >= radiusSum * radiusSum) {
        return result;  // 不碰撞
    }
    
    float distance = std::sqrt(distanceSquared);
    
    result.hit = true;
    result.penetration = radiusSum - distance;
    
    // 计算法线
    if (distance > 0.0001f) {
        result.normal = Math::Vec2(-dx / distance, -dy / distance);
    } else {
        // 圆心重合，选择任意方向
        result.normal = Math::Vec2(1, 0);
    }
    
    // 碰撞点在两圆边界的中间
    result.point = a.center + Math::Vec2(dx, dy) * (a.radius / radiusSum);
    
    return result;
}

// ========================================================================
// AABB vs Circle
// ========================================================================

/**
 * 检测AABB和圆是否碰撞
 */
inline bool TestAABBvsCircle(const AABB& aabb, const Circle& circle) {
    return circle.Intersects(aabb);
}

/**
 * 检测圆和AABB碰撞并返回详细结果
 * 
 * @param circle 圆（移动物体）
 * @param aabb AABB（静态物体）
 */
inline HitResult CirclevsAABB(const Circle& circle, const AABB& aabb) {
    HitResult result;
    
    // 找到AABB上最近的点
    Math::Vec2 closest = aabb.GetClosestPoint(circle.center);
    
    float dx = circle.center.x - closest.x;
    float dy = circle.center.y - closest.y;
    float distanceSquared = dx * dx + dy * dy;
    
    // 检查圆心是否在AABB内部
    bool insideAABB = aabb.Contains(circle.center);
    
    if (!insideAABB && distanceSquared >= circle.radius * circle.radius) {
        return result;  // 不碰撞
    }
    
    result.hit = true;
    result.point = closest;
    
    if (insideAABB) {
        // 圆心在AABB内部，需要特殊处理
        // 找到距离最近的边
        Math::Vec2 toCenter = circle.center - aabb.center;
        
        float distToRight = aabb.halfSize.x - toCenter.x;
        float distToLeft = aabb.halfSize.x + toCenter.x;
        float distToTop = aabb.halfSize.y - toCenter.y;
        float distToBottom = aabb.halfSize.y + toCenter.y;
        
        float minDist = distToRight;
        result.normal = Math::Vec2(1, 0);
        
        if (distToLeft < minDist) {
            minDist = distToLeft;
            result.normal = Math::Vec2(-1, 0);
        }
        if (distToTop < minDist) {
            minDist = distToTop;
            result.normal = Math::Vec2(0, 1);
        }
        if (distToBottom < minDist) {
            minDist = distToBottom;
            result.normal = Math::Vec2(0, -1);
        }
        
        result.penetration = minDist + circle.radius;
        result.point = circle.center + result.normal * (-minDist);
    } else {
        float distance = std::sqrt(distanceSquared);
        result.penetration = circle.radius - distance;
        
        if (distance > 0.0001f) {
            result.normal = Math::Vec2(dx / distance, dy / distance);
        } else {
            result.normal = Math::Vec2(1, 0);
        }
    }
    
    return result;
}

// ========================================================================
// 射线检测
// ========================================================================

/**
 * 射线与AABB的相交检测
 * 
 * 使用Slab方法：
 * 将AABB看作X和Y两个"板"的交集，
 * 分别计算射线进入和离开每个板的t值，
 * 如果两个范围有交集，则射线穿过AABB
 * 
 * @param ray 射线
 * @param aabb AABB
 * @param maxDistance 最大检测距离
 * @return 碰撞结果
 */
inline HitResult RayvsAABB(const Ray2D& ray, const AABB& aabb, float maxDistance = 10000.0f) {
    HitResult result;
    
    Math::Vec2 min = aabb.GetMin();
    Math::Vec2 max = aabb.GetMax();
    
    float tMin = 0.0f;
    float tMax = maxDistance;
    
    Math::Vec2 normal;
    
    // X轴
    if (std::abs(ray.direction.x) < 0.0001f) {
        // 射线平行于Y轴
        if (ray.origin.x < min.x || ray.origin.x > max.x) {
            return result;  // 不相交
        }
    } else {
        float invD = 1.0f / ray.direction.x;
        float t1 = (min.x - ray.origin.x) * invD;
        float t2 = (max.x - ray.origin.x) * invD;
        
        bool swapped = false;
        if (t1 > t2) {
            std::swap(t1, t2);
            swapped = true;
        }
        
        if (t1 > tMin) {
            tMin = t1;
            normal = swapped ? Math::Vec2(1, 0) : Math::Vec2(-1, 0);
        }
        tMax = std::min(tMax, t2);
        
        if (tMin > tMax) return result;
    }
    
    // Y轴
    if (std::abs(ray.direction.y) < 0.0001f) {
        if (ray.origin.y < min.y || ray.origin.y > max.y) {
            return result;
        }
    } else {
        float invD = 1.0f / ray.direction.y;
        float t1 = (min.y - ray.origin.y) * invD;
        float t2 = (max.y - ray.origin.y) * invD;
        
        bool swapped = false;
        if (t1 > t2) {
            std::swap(t1, t2);
            swapped = true;
        }
        
        if (t1 > tMin) {
            tMin = t1;
            normal = swapped ? Math::Vec2(0, 1) : Math::Vec2(0, -1);
        }
        tMax = std::min(tMax, t2);
        
        if (tMin > tMax) return result;
    }
    
    if (tMin < 0) return result;  // 在射线后方
    
    result.hit = true;
    result.t = tMin;
    result.point = ray.GetPoint(tMin);
    result.normal = normal;
    
    return result;
}

/**
 * 射线与圆的相交检测
 * 
 * 使用几何方法：
 * 1. 计算射线到圆心的最近点
 * 2. 检查该点是否在圆内
 * 3. 如果在，计算交点
 * 
 * @param ray 射线
 * @param circle 圆
 * @param maxDistance 最大检测距离
 * @return 碰撞结果
 */
inline HitResult RayvsCircle(const Ray2D& ray, const Circle& circle, float maxDistance = 10000.0f) {
    HitResult result;
    
    // 从射线起点到圆心的向量
    Math::Vec2 oc = ray.origin - circle.center;
    
    // 二次方程系数：at² + bt + c = 0
    float a = ray.direction.Dot(ray.direction);  // 应该是1（单位向量）
    float b = 2.0f * oc.Dot(ray.direction);
    float c = oc.Dot(oc) - circle.radius * circle.radius;
    
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) {
        return result;  // 没有交点
    }
    
    float sqrtD = std::sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2 * a);
    float t2 = (-b + sqrtD) / (2 * a);
    
    // 选择最近的正值t
    float t;
    if (t1 >= 0 && t1 <= maxDistance) {
        t = t1;
    } else if (t2 >= 0 && t2 <= maxDistance) {
        t = t2;
    } else {
        return result;  // 交点不在有效范围内
    }
    
    result.hit = true;
    result.t = t;
    result.point = ray.GetPoint(t);
    
    // 法线从圆心指向交点
    Math::Vec2 toPoint = result.point - circle.center;
    result.normal = toPoint.Normalized();
    
    return result;
}

// ========================================================================
// 移动碰撞（扫掠检测）
// ========================================================================

/**
 * 移动AABB与静态AABB的碰撞检测
 * 
 * 使用Minkowski差方法：
 * 将移动的AABB缩小为一个点，将静态AABB扩大
 * 
 * @param moving 移动的AABB
 * @param velocity 移动速度（位移向量）
 * @param stationary 静态AABB
 * @return 碰撞结果，t表示碰撞发生的时间（0-1）
 */
inline HitResult SweptAABBvsAABB(const AABB& moving, const Math::Vec2& velocity, const AABB& stationary) {
    HitResult result;
    
    // 如果没有速度，使用普通碰撞检测
    if (velocity.LengthSquared() < 0.0001f) {
        return AABBvsAABB(moving, stationary);
    }
    
    // 扩展静态AABB（Minkowski和）
    AABB expanded = stationary;
    expanded.halfSize.x += moving.halfSize.x;
    expanded.halfSize.y += moving.halfSize.y;
    
    // 从移动AABB的中心发射射线
    Ray2D ray(moving.center, velocity.Normalized());
    
    // 射线检测
    result = RayvsAABB(ray, expanded, velocity.Length());
    
    if (result.hit) {
        // 调整t值为归一化的（0-1范围）
        result.t = result.t / velocity.Length();
    }
    
    return result;
}

} // namespace Collision

} // namespace MiniEngine

#endif // MINI_ENGINE_COLLISION_H
