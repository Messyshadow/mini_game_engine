/**
 * Circle.h - 圆形碰撞体
 * 
 * ============================================================================
 * 圆形碰撞的特点
 * ============================================================================
 * 
 * 圆形是第二简单的碰撞形状，由圆心和半径定义。
 * 
 *         ╭────────╮
 *       ╱            ╲
 *      │      ●───────│  ● = 圆心 (center)
 *       ╲      r     ╱   r = 半径 (radius)
 *         ╰────────╯
 * 
 * 优点：
 * - 旋转不变（旋转后碰撞体不变）
 * - 碰撞计算简单（比较距离）
 * - 适合表示球形物体
 * 
 * 缺点：
 * - 不适合长方形物体
 * - 可能浪费空间
 */

#ifndef MINI_ENGINE_CIRCLE_H
#define MINI_ENGINE_CIRCLE_H

#include "math/Math.h"
#include "AABB.h"

namespace MiniEngine {

/**
 * 圆形碰撞体
 */
class Circle {
public:
    // ========================================================================
    // 成员变量
    // ========================================================================
    
    Math::Vec2 center;  // 圆心
    float radius;       // 半径
    
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    /**
     * 默认构造：原点，零半径
     */
    Circle()
        : center(0.0f, 0.0f)
        , radius(0.0f)
    {}
    
    /**
     * 从圆心和半径构造
     */
    Circle(const Math::Vec2& center, float radius)
        : center(center)
        , radius(radius)
    {}
    
    /**
     * 从坐标和半径构造
     */
    Circle(float x, float y, float radius)
        : center(x, y)
        , radius(radius)
    {}
    
    // ========================================================================
    // 属性
    // ========================================================================
    
    /** 获取直径 */
    float GetDiameter() const {
        return radius * 2.0f;
    }
    
    /** 获取周长 */
    float GetCircumference() const {
        return 2.0f * Math::PI * radius;
    }
    
    /** 获取面积 */
    float GetArea() const {
        return Math::PI * radius * radius;
    }
    
    /** 获取包围盒 */
    AABB GetBoundingBox() const {
        return AABB(center, Math::Vec2(radius, radius));
    }
    
    // ========================================================================
    // 设置方法
    // ========================================================================
    
    /** 设置圆心 */
    void SetCenter(const Math::Vec2& newCenter) {
        center = newCenter;
    }
    
    /** 设置圆心 */
    void SetCenter(float x, float y) {
        center.x = x;
        center.y = y;
    }
    
    /** 设置半径 */
    void SetRadius(float newRadius) {
        radius = newRadius;
        if (radius < 0) radius = 0;
    }
    
    /** 移动 */
    void Move(const Math::Vec2& delta) {
        center += delta;
    }
    
    /** 移动 */
    void Move(float dx, float dy) {
        center.x += dx;
        center.y += dy;
    }
    
    // ========================================================================
    // 碰撞检测
    // ========================================================================
    
    /**
     * 检测是否与另一个圆形碰撞
     * 
     * 原理：两圆心距离 < 两半径之和
     * 
     * 优化：比较距离的平方，避免开方运算
     * 
     * @param other 另一个圆形
     * @return true如果碰撞
     */
    bool Intersects(const Circle& other) const {
        // 计算圆心距离的平方
        float dx = other.center.x - center.x;
        float dy = other.center.y - center.y;
        float distanceSquared = dx * dx + dy * dy;
        
        // 计算半径之和的平方
        float radiusSum = radius + other.radius;
        float radiusSumSquared = radiusSum * radiusSum;
        
        // 比较平方值（避免sqrt）
        return distanceSquared < radiusSumSquared;
    }
    
    /**
     * 检测是否与AABB碰撞
     * 
     * 原理：找到AABB上距离圆心最近的点，检查该点是否在圆内
     * 
     * @param aabb AABB碰撞体
     * @return true如果碰撞
     */
    bool Intersects(const AABB& aabb) const {
        // 找到AABB上最近的点
        Math::Vec2 closest = aabb.GetClosestPoint(center);
        
        // 计算该点到圆心的距离
        float dx = closest.x - center.x;
        float dy = closest.y - center.y;
        float distanceSquared = dx * dx + dy * dy;
        
        // 如果距离小于半径，则碰撞
        return distanceSquared < (radius * radius);
    }
    
    /**
     * 检测点是否在圆内
     * 
     * @param point 要检测的点
     * @return true如果点在圆内
     */
    bool Contains(const Math::Vec2& point) const {
        float dx = point.x - center.x;
        float dy = point.y - center.y;
        float distanceSquared = dx * dx + dy * dy;
        
        return distanceSquared <= (radius * radius);
    }
    
    /**
     * 检测是否完全包含另一个圆
     * 
     * @param other 另一个圆
     * @return true如果完全包含
     */
    bool Contains(const Circle& other) const {
        float dx = other.center.x - center.x;
        float dy = other.center.y - center.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        // 如果圆心距离 + 对方半径 <= 自己半径，则完全包含
        return (distance + other.radius) <= radius;
    }
    
    // ========================================================================
    // 碰撞响应
    // ========================================================================
    
    /**
     * 计算与另一个圆的穿透深度
     * 
     * @param other 另一个圆
     * @return 穿透向量（将this推出other）
     */
    Math::Vec2 GetPenetration(const Circle& other) const {
        float dx = center.x - other.center.x;
        float dy = center.y - other.center.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        // 避免除以零
        if (distance < 0.0001f) {
            // 圆心几乎重合，随便选一个方向
            return Math::Vec2(radius + other.radius, 0);
        }
        
        float radiusSum = radius + other.radius;
        float penetration = radiusSum - distance;
        
        // 如果没有穿透
        if (penetration <= 0) {
            return Math::Vec2::Zero();
        }
        
        // 计算推出方向（从other指向this的单位向量）
        float nx = dx / distance;
        float ny = dy / distance;
        
        // 返回穿透向量
        return Math::Vec2(nx * penetration, ny * penetration);
    }
    
    /**
     * 计算与AABB的穿透深度
     * 
     * @param aabb AABB碰撞体
     * @return 穿透向量（将this推出aabb）
     */
    Math::Vec2 GetPenetration(const AABB& aabb) const {
        // 找到AABB上最近的点
        Math::Vec2 closest = aabb.GetClosestPoint(center);
        
        float dx = center.x - closest.x;
        float dy = center.y - closest.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        // 特殊情况：圆心在AABB内部
        if (distance < 0.0001f) {
            // 需要特殊处理
            // 计算从AABB中心到圆心的方向
            dx = center.x - aabb.center.x;
            dy = center.y - aabb.center.y;
            
            // 计算到各边的距离
            float distToLeft = center.x - aabb.GetMin().x;
            float distToRight = aabb.GetMax().x - center.x;
            float distToBottom = center.y - aabb.GetMin().y;
            float distToTop = aabb.GetMax().y - center.y;
            
            // 找最小距离的边
            float minDist = distToLeft;
            Math::Vec2 pushDir(-1, 0);
            
            if (distToRight < minDist) {
                minDist = distToRight;
                pushDir = Math::Vec2(1, 0);
            }
            if (distToBottom < minDist) {
                minDist = distToBottom;
                pushDir = Math::Vec2(0, -1);
            }
            if (distToTop < minDist) {
                minDist = distToTop;
                pushDir = Math::Vec2(0, 1);
            }
            
            return pushDir * (minDist + radius);
        }
        
        float penetration = radius - distance;
        
        if (penetration <= 0) {
            return Math::Vec2::Zero();
        }
        
        // 计算推出方向
        float nx = dx / distance;
        float ny = dy / distance;
        
        return Math::Vec2(nx * penetration, ny * penetration);
    }
    
    /**
     * 获取圆上距离某点最近的点
     * 
     * @param point 目标点
     * @return 圆周上最近的点
     */
    Math::Vec2 GetClosestPoint(const Math::Vec2& point) const {
        float dx = point.x - center.x;
        float dy = point.y - center.y;
        float distance = std::sqrt(dx * dx + dy * dy);
        
        if (distance < 0.0001f) {
            // 点在圆心，返回任意边界点
            return Math::Vec2(center.x + radius, center.y);
        }
        
        // 返回圆周上的点
        float nx = dx / distance;
        float ny = dy / distance;
        
        return Math::Vec2(center.x + nx * radius, center.y + ny * radius);
    }
};

} // namespace MiniEngine

#endif // MINI_ENGINE_CIRCLE_H
