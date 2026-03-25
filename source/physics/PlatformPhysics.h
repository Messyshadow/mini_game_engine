/**
 * PlatformPhysics.h - 平台物理系统
 * 
 * 专门为2D平台跳跃游戏设计的物理系统，特点：
 * - 分轴碰撞检测（先X后Y）
 * - 单向平台支持
 * - 地面/墙壁/天花板检测
 * - 可变高度跳跃
 */

#ifndef MINI_ENGINE_PLATFORM_PHYSICS_H
#define MINI_ENGINE_PLATFORM_PHYSICS_H

#include "math/Math.h"
#include "physics/AABB.h"
#include "physics/PhysicsBody2D.h"
#include <vector>
#include <functional>

namespace MiniEngine {

/**
 * 碰撞信息
 */
struct CollisionInfo {
    bool collided = false;          // 是否发生碰撞
    Math::Vec2 normal;              // 碰撞法线
    float penetration = 0.0f;       // 穿透深度
    PhysicsBody2D* other = nullptr; // 碰撞的另一个物体
    
    // 碰撞方向
    bool fromTop = false;           // 从上方碰撞（落地）
    bool fromBottom = false;        // 从下方碰撞（撞头）
    bool fromLeft = false;          // 从左侧碰撞
    bool fromRight = false;         // 从右侧碰撞
};

/**
 * 移动结果
 */
struct MoveResult {
    Math::Vec2 finalPosition;       // 最终位置
    Math::Vec2 finalVelocity;       // 最终速度
    
    bool hitGround = false;         // 是否落地
    bool hitCeiling = false;        // 是否撞头
    bool hitWallLeft = false;       // 是否撞左墙
    bool hitWallRight = false;      // 是否撞右墙
    
    std::vector<CollisionInfo> collisions;  // 所有碰撞
};

/**
 * 平台物理系统
 */
class PlatformPhysics {
public:
    // ========================================================================
    // 物理参数
    // ========================================================================
    
    Math::Vec2 gravity;             // 重力加速度（默认向下）
    float maxFallSpeed;             // 最大下落速度
    float groundCheckDistance;      // 地面检测距离
    float wallCheckDistance;        // 墙壁检测距离
    
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    PlatformPhysics();
    ~PlatformPhysics() = default;
    
    // ========================================================================
    // 碰撞体管理
    // ========================================================================
    
    /**
     * 添加碰撞体
     * 
     * @param body 物理体指针（不会被系统释放，需要外部管理生命周期）
     */
    void AddBody(PhysicsBody2D* body);
    
    /**
     * 移除碰撞体
     */
    void RemoveBody(PhysicsBody2D* body);
    
    /**
     * 清除所有碰撞体
     */
    void Clear();
    
    /**
     * 获取所有碰撞体
     */
    const std::vector<PhysicsBody2D*>& GetBodies() const { return m_bodies; }
    
    // ========================================================================
    // 物理更新
    // ========================================================================
    
    /**
     * 更新单个动态物体（应用重力和速度）
     * 
     * @param body 要更新的物理体
     * @param deltaTime 时间步长
     */
    void UpdateBody(PhysicsBody2D* body, float deltaTime);
    
    /**
     * 移动并处理碰撞
     * 
     * 这是核心方法，执行以下步骤：
     * 1. 应用重力到速度
     * 2. 先移动X轴，检测X轴碰撞
     * 3. 再移动Y轴，检测Y轴碰撞
     * 4. 返回碰撞结果
     * 
     * @param body 要移动的物理体
     * @param deltaTime 时间步长
     * @return 移动结果
     */
    MoveResult MoveAndCollide(PhysicsBody2D* body, float deltaTime);
    
    /**
     * 仅移动，不应用重力
     */
    MoveResult MoveAndCollideNoGravity(PhysicsBody2D* body, const Math::Vec2& displacement);
    
    // ========================================================================
    // 检测方法
    // ========================================================================
    
    /**
     * 检测是否在地面上
     * 
     * @param body 要检测的物理体
     * @return 如果在地面上返回true
     */
    bool IsOnGround(const PhysicsBody2D* body) const;
    
    /**
     * 检测是否贴墙
     * 
     * @param body 要检测的物理体
     * @param checkLeft 检测左边还是右边
     * @return 如果贴墙返回true
     */
    bool IsOnWall(const PhysicsBody2D* body, bool checkLeft) const;
    
    /**
     * 检测是否碰到天花板
     */
    bool IsHittingCeiling(const PhysicsBody2D* body) const;
    
    /**
     * 检测点是否在任何碰撞体内
     */
    bool PointInSolid(const Math::Vec2& point) const;
    
    /**
     * 获取指定位置下方的地面
     * 
     * @param position 检测位置
     * @param maxDistance 最大检测距离
     * @return 地面物理体，没有则返回nullptr
     */
    PhysicsBody2D* GetGroundBelow(const Math::Vec2& position, float maxDistance) const;
    
    // ========================================================================
    // 射线检测
    // ========================================================================
    
    /**
     * 射线检测
     * 
     * @param origin 起点
     * @param direction 方向（单位向量）
     * @param maxDistance 最大距离
     * @param layerMask 层掩码
     * @return 碰撞信息
     */
    CollisionInfo Raycast(const Math::Vec2& origin, const Math::Vec2& direction,
                          float maxDistance, unsigned int layerMask = LAYER_ALL) const;
    
    // ========================================================================
    // 辅助方法
    // ========================================================================
    
    /**
     * 创建静态地面
     */
    static PhysicsBody2D CreateGround(float x, float y, float width, float height);
    
    /**
     * 创建单向平台
     */
    static PhysicsBody2D CreateOneWayPlatform(float x, float y, float width, float height);
    
    /**
     * 创建墙壁
     */
    static PhysicsBody2D CreateWall(float x, float y, float width, float height);
    
private:
    std::vector<PhysicsBody2D*> m_bodies;
    
    /**
     * 检测AABB与所有静态碰撞体的碰撞
     */
    CollisionInfo CheckCollisionWithStatics(const AABB& aabb, const PhysicsBody2D* self,
                                            const Math::Vec2& velocity) const;
    
    /**
     * 处理单向平台碰撞
     */
    bool ShouldCollideWithOneWay(const PhysicsBody2D* body, const PhysicsBody2D* platform,
                                  const Math::Vec2& velocity) const;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_PLATFORM_PHYSICS_H
