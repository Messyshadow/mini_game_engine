/**
 * PhysicsBody2D.h - 2D物理体
 * 
 * 物理体是参与物理模拟的基本单位，包含：
 * - 位置、速度、加速度
 * - 碰撞盒
 * - 物理属性（重力缩放、是否静态等）
 */

#ifndef MINI_ENGINE_PHYSICS_BODY_2D_H
#define MINI_ENGINE_PHYSICS_BODY_2D_H

#include "math/Math.h"
#include "physics/AABB.h"

namespace MiniEngine {

/**
 * 物理体类型
 */
enum class BodyType {
    Static,     // 静态：不会移动（地面、墙壁）
    Dynamic,    // 动态：受力移动（玩家、敌人）
    Kinematic   // 运动学：按设定移动，不受力（移动平台）
};

/**
 * 碰撞层标记
 */
enum CollisionLayer : unsigned int {
    LAYER_NONE          = 0,
    LAYER_GROUND        = 1 << 0,   // 地面
    LAYER_WALL          = 1 << 1,   // 墙壁
    LAYER_PLATFORM      = 1 << 2,   // 单向平台
    LAYER_PLAYER        = 1 << 3,   // 玩家
    LAYER_ENEMY         = 1 << 4,   // 敌人
    LAYER_PROJECTILE    = 1 << 5,   // 投射物
    LAYER_TRIGGER       = 1 << 6,   // 触发器
    LAYER_ALL           = 0xFFFFFFFF
};

/**
 * 2D物理体
 */
class PhysicsBody2D {
public:
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    PhysicsBody2D()
        : position(0, 0)
        , velocity(0, 0)
        , acceleration(0, 0)
        , size(32, 32)
        , bodyType(BodyType::Dynamic)
        , gravityScale(1.0f)
        , friction(0.0f)
        , bounciness(0.0f)
        , layer(LAYER_NONE)
        , collisionMask(LAYER_ALL)
        , isOneWayPlatform(false)
        , enabled(true)
        , userData(nullptr)
    {}
    
    PhysicsBody2D(const Math::Vec2& pos, const Math::Vec2& sz, BodyType type = BodyType::Dynamic)
        : position(pos)
        , velocity(0, 0)
        , acceleration(0, 0)
        , size(sz)
        , bodyType(type)
        , gravityScale(type == BodyType::Dynamic ? 1.0f : 0.0f)
        , friction(0.0f)
        , bounciness(0.0f)
        , layer(LAYER_NONE)
        , collisionMask(LAYER_ALL)
        , isOneWayPlatform(false)
        , enabled(true)
        , userData(nullptr)
    {}
    
    // ========================================================================
    // 位置和速度
    // ========================================================================
    
    Math::Vec2 position;        // 中心位置
    Math::Vec2 velocity;        // 速度（像素/秒）
    Math::Vec2 acceleration;    // 加速度（像素/秒²）
    Math::Vec2 size;            // 碰撞盒尺寸
    
    // ========================================================================
    // 物理属性
    // ========================================================================
    
    BodyType bodyType;          // 物理体类型
    float gravityScale;         // 重力缩放（0=不受重力）
    float friction;             // 摩擦力（0-1）
    float bounciness;           // 弹性（0=不弹跳，1=完全弹跳）
    
    // ========================================================================
    // 碰撞属性
    // ========================================================================
    
    unsigned int layer;         // 所属层
    unsigned int collisionMask; // 碰撞掩码（与哪些层碰撞）
    bool isOneWayPlatform;      // 是否是单向平台
    bool enabled;               // 是否启用
    
    // ========================================================================
    // 用户数据
    // ========================================================================
    
    void* userData;             // 可以存储任意数据（如游戏对象指针）
    
    // ========================================================================
    // 辅助方法
    // ========================================================================
    
    /**
     * 获取AABB碰撞盒
     */
    AABB GetAABB() const {
        return AABB(position, size * 0.5f);
    }
    
    /**
     * 获取边界
     */
    float GetLeft() const   { return position.x - size.x * 0.5f; }
    float GetRight() const  { return position.x + size.x * 0.5f; }
    float GetBottom() const { return position.y - size.y * 0.5f; }
    float GetTop() const    { return position.y + size.y * 0.5f; }
    
    /**
     * 设置位置（左下角）
     */
    void SetPositionBottomLeft(const Math::Vec2& bottomLeft) {
        position.x = bottomLeft.x + size.x * 0.5f;
        position.y = bottomLeft.y + size.y * 0.5f;
    }
    
    /**
     * 检查是否与另一个物理体的层碰撞
     */
    bool CanCollideWith(const PhysicsBody2D& other) const {
        return (collisionMask & other.layer) != 0;
    }
    
    /**
     * 检查是否是静态物体
     */
    bool IsStatic() const {
        return bodyType == BodyType::Static;
    }
    
    /**
     * 检查是否是动态物体
     */
    bool IsDynamic() const {
        return bodyType == BodyType::Dynamic;
    }
};

} // namespace MiniEngine

#endif // MINI_ENGINE_PHYSICS_BODY_2D_H
