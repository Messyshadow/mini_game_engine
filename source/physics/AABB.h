/**
 * AABB.h - 轴对齐包围盒 (Axis-Aligned Bounding Box)
 * 
 * ============================================================================
 * 什么是AABB？
 * ============================================================================
 * 
 * AABB是最简单的碰撞形状，它是一个边与坐标轴平行的矩形。
 * 
 *              max (右上角)
 *     ┌──────────●
 *     │          │
 *     │   AABB   │  height
 *     │          │
 *     ●──────────┘
 *    min        width
 *   (左下角)
 * 
 * 优点：
 * - 碰撞检测非常快（只需比较坐标）
 * - 内存占用小（只需存储两个点或中心+尺寸）
 * - 容易计算和更新
 * 
 * 缺点：
 * - 不能旋转（旋转后需要重新计算包围盒）
 * - 对于非矩形物体可能不够精确
 * 
 * ============================================================================
 * 两种表示方法
 * ============================================================================
 * 
 * 方法1：最小点/最大点 (min/max)
 *   - min = 左下角坐标
 *   - max = 右上角坐标
 *   - 优点：直接用于碰撞检测
 * 
 * 方法2：中心点/半尺寸 (center/halfSize)
 *   - center = 中心坐标
 *   - halfSize = (宽度/2, 高度/2)
 *   - 优点：更适合物理模拟和变换
 * 
 * 我们两种都支持，可以互相转换。
 */

#ifndef MINI_ENGINE_AABB_H
#define MINI_ENGINE_AABB_H

#include "math/Math.h"

namespace MiniEngine {

/**
 * AABB类 - 轴对齐包围盒
 * 
 * 使用中心点+半尺寸的表示方法，同时提供min/max访问接口
 */
class AABB {
public:
    // ========================================================================
    // 成员变量（公开，方便直接访问）
    // ========================================================================
    
    Math::Vec2 center;      // 中心点坐标
    Math::Vec2 halfSize;    // 半尺寸 (宽度/2, 高度/2)
    
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    /**
     * 默认构造：原点位置，零尺寸
     */
    AABB() 
        : center(0.0f, 0.0f)
        , halfSize(0.0f, 0.0f) 
    {}
    
    /**
     * 从中心点和半尺寸构造
     * 
     * @param center 中心点坐标
     * @param halfSize 半尺寸 (halfWidth, halfHeight)
     */
    AABB(const Math::Vec2& center, const Math::Vec2& halfSize)
        : center(center)
        , halfSize(halfSize)
    {}
    
    /**
     * 从中心点和完整尺寸构造
     * 
     * @param centerX 中心X坐标
     * @param centerY 中心Y坐标
     * @param width 完整宽度
     * @param height 完整高度
     */
    AABB(float centerX, float centerY, float width, float height)
        : center(centerX, centerY)
        , halfSize(width * 0.5f, height * 0.5f)
    {}
    
    /**
     * 从最小点和最大点构造
     * 
     * @param min 左下角
     * @param max 右上角
     */
    static AABB FromMinMax(const Math::Vec2& min, const Math::Vec2& max) {
        AABB result;
        result.center = (min + max) * 0.5f;
        result.halfSize = (max - min) * 0.5f;
        return result;
    }
    
    /**
     * 从位置和尺寸构造（左下角为原点）
     * 
     * @param position 左下角位置
     * @param size 尺寸 (width, height)
     */
    static AABB FromPositionSize(const Math::Vec2& position, const Math::Vec2& size) {
        AABB result;
        result.halfSize = size * 0.5f;
        result.center = position + result.halfSize;
        return result;
    }
    
    // ========================================================================
    // 属性访问
    // ========================================================================
    
    /** 获取左下角（最小点） */
    Math::Vec2 GetMin() const { 
        return center - halfSize; 
    }
    
    /** 获取右上角（最大点） */
    Math::Vec2 GetMax() const { 
        return center + halfSize; 
    }
    
    /** 获取完整宽度 */
    float GetWidth() const { 
        return halfSize.x * 2.0f; 
    }
    
    /** 获取完整高度 */
    float GetHeight() const { 
        return halfSize.y * 2.0f; 
    }
    
    /** 获取尺寸 */
    Math::Vec2 GetSize() const { 
        return halfSize * 2.0f; 
    }
    
    /** 获取面积 */
    float GetArea() const {
        return GetWidth() * GetHeight();
    }
    
    /** 获取周长 */
    float GetPerimeter() const {
        return 2.0f * (GetWidth() + GetHeight());
    }
    
    // ========================================================================
    // 设置方法
    // ========================================================================
    
    /** 设置中心点 */
    void SetCenter(const Math::Vec2& newCenter) {
        center = newCenter;
    }
    
    /** 设置中心点 */
    void SetCenter(float x, float y) {
        center.x = x;
        center.y = y;
    }
    
    /** 设置尺寸（完整尺寸，内部转换为半尺寸） */
    void SetSize(const Math::Vec2& size) {
        halfSize = size * 0.5f;
    }
    
    /** 设置尺寸 */
    void SetSize(float width, float height) {
        halfSize.x = width * 0.5f;
        halfSize.y = height * 0.5f;
    }
    
    /** 移动（增量） */
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
     * 检测是否与另一个AABB碰撞
     * 
     * 原理：如果在X轴和Y轴上都有重叠，则碰撞
     * 
     * @param other 另一个AABB
     * @return true如果碰撞
     */
    bool Intersects(const AABB& other) const {
        // 计算两个中心点的距离
        float dx = std::abs(center.x - other.center.x);
        float dy = std::abs(center.y - other.center.y);
        
        // 计算两个半尺寸之和
        float sumHalfWidth = halfSize.x + other.halfSize.x;
        float sumHalfHeight = halfSize.y + other.halfSize.y;
        
        // 如果距离小于半尺寸之和，则重叠
        return (dx < sumHalfWidth) && (dy < sumHalfHeight);
    }
    
    /**
     * 检测点是否在AABB内部
     * 
     * @param point 要检测的点
     * @return true如果点在内部
     */
    bool Contains(const Math::Vec2& point) const {
        Math::Vec2 min = GetMin();
        Math::Vec2 max = GetMax();
        
        return (point.x >= min.x && point.x <= max.x &&
                point.y >= min.y && point.y <= max.y);
    }
    
    /**
     * 检测是否完全包含另一个AABB
     * 
     * @param other 另一个AABB
     * @return true如果完全包含
     */
    bool Contains(const AABB& other) const {
        Math::Vec2 myMin = GetMin();
        Math::Vec2 myMax = GetMax();
        Math::Vec2 otherMin = other.GetMin();
        Math::Vec2 otherMax = other.GetMax();
        
        return (otherMin.x >= myMin.x && otherMax.x <= myMax.x &&
                otherMin.y >= myMin.y && otherMax.y <= myMax.y);
    }
    
    // ========================================================================
    // 碰撞响应
    // ========================================================================
    
    /**
     * 计算与另一个AABB的穿透深度
     * 
     * 返回将this推出other所需的最小位移向量
     * 
     * @param other 另一个AABB
     * @return 穿透向量（如果不碰撞，返回零向量）
     * 
     * 示例：
     *     AABB a, b;
     *     Vec2 penetration = a.GetPenetration(b);
     *     if (penetration != Vec2::Zero()) {
     *         a.Move(penetration);  // 将a推出b
     *     }
     */
    Math::Vec2 GetPenetration(const AABB& other) const {
        // 计算X轴和Y轴的重叠量
        float dx = other.center.x - center.x;
        float dy = other.center.y - center.y;
        
        float overlapX = (halfSize.x + other.halfSize.x) - std::abs(dx);
        float overlapY = (halfSize.y + other.halfSize.y) - std::abs(dy);
        
        // 如果任一轴没有重叠，则不碰撞
        if (overlapX <= 0 || overlapY <= 0) {
            return Math::Vec2::Zero();
        }
        
        // 选择穿透最小的轴作为推出方向
        if (overlapX < overlapY) {
            // X轴穿透更小，从X轴推出
            float pushX = (dx > 0) ? -overlapX : overlapX;
            return Math::Vec2(pushX, 0);
        } else {
            // Y轴穿透更小，从Y轴推出
            float pushY = (dy > 0) ? -overlapY : overlapY;
            return Math::Vec2(0, pushY);
        }
    }
    
    /**
     * 计算点到AABB的最近点
     * 
     * @param point 要检测的点
     * @return AABB上距离point最近的点
     */
    Math::Vec2 GetClosestPoint(const Math::Vec2& point) const {
        Math::Vec2 min = GetMin();
        Math::Vec2 max = GetMax();
        
        Math::Vec2 closest;
        closest.x = Math::Clamp(point.x, min.x, max.x);
        closest.y = Math::Clamp(point.y, min.y, max.y);
        
        return closest;
    }
    
    // ========================================================================
    // 合并操作
    // ========================================================================
    
    /**
     * 扩展AABB以包含一个点
     * 
     * @param point 要包含的点
     */
    void ExpandToInclude(const Math::Vec2& point) {
        Math::Vec2 min = GetMin();
        Math::Vec2 max = GetMax();
        
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        
        *this = FromMinMax(min, max);
    }
    
    /**
     * 扩展AABB以包含另一个AABB
     * 
     * @param other 要包含的AABB
     */
    void ExpandToInclude(const AABB& other) {
        Math::Vec2 min = GetMin();
        Math::Vec2 max = GetMax();
        Math::Vec2 otherMin = other.GetMin();
        Math::Vec2 otherMax = other.GetMax();
        
        min.x = std::min(min.x, otherMin.x);
        min.y = std::min(min.y, otherMin.y);
        max.x = std::max(max.x, otherMax.x);
        max.y = std::max(max.y, otherMax.y);
        
        *this = FromMinMax(min, max);
    }
    
    /**
     * 返回两个AABB的合并
     */
    static AABB Merge(const AABB& a, const AABB& b) {
        AABB result = a;
        result.ExpandToInclude(b);
        return result;
    }
    
    // ========================================================================
    // 膨胀/收缩
    // ========================================================================
    
    /**
     * 膨胀AABB（向外扩展）
     * 
     * @param amount 扩展量（正值膨胀，负值收缩）
     */
    void Inflate(float amount) {
        halfSize.x += amount;
        halfSize.y += amount;
        
        // 防止负尺寸
        if (halfSize.x < 0) halfSize.x = 0;
        if (halfSize.y < 0) halfSize.y = 0;
    }
    
    /**
     * 返回膨胀后的AABB副本
     */
    AABB Inflated(float amount) const {
        AABB result = *this;
        result.Inflate(amount);
        return result;
    }
};

} // namespace MiniEngine

#endif // MINI_ENGINE_AABB_H
