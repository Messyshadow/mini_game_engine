/**
 * PlatformPhysics.cpp - 平台物理系统实现
 */

#include "PlatformPhysics.h"
#include <algorithm>
#include <cmath>

namespace MiniEngine {

// ============================================================================
// 构造函数
// ============================================================================

PlatformPhysics::PlatformPhysics()
    : gravity(0.0f, -1500.0f)       // 默认重力向下
    , maxFallSpeed(800.0f)          // 最大下落速度
    , groundCheckDistance(2.0f)     // 地面检测距离
    , wallCheckDistance(1.0f)       // 墙壁检测距离
{
}

// ============================================================================
// 碰撞体管理
// ============================================================================

void PlatformPhysics::AddBody(PhysicsBody2D* body) {
    if (body) {
        m_bodies.push_back(body);
    }
}

void PlatformPhysics::RemoveBody(PhysicsBody2D* body) {
    auto it = std::find(m_bodies.begin(), m_bodies.end(), body);
    if (it != m_bodies.end()) {
        m_bodies.erase(it);
    }
}

void PlatformPhysics::Clear() {
    m_bodies.clear();
}

// ============================================================================
// 物理更新
// ============================================================================

void PlatformPhysics::UpdateBody(PhysicsBody2D* body, float deltaTime) {
    if (!body || !body->enabled || body->IsStatic()) {
        return;
    }
    
    // 应用重力
    body->velocity += gravity * body->gravityScale * deltaTime;
    
    // 限制下落速度
    if (body->velocity.y < -maxFallSpeed) {
        body->velocity.y = -maxFallSpeed;
    }
    
    // 应用加速度
    body->velocity += body->acceleration * deltaTime;
}

MoveResult PlatformPhysics::MoveAndCollide(PhysicsBody2D* body, float deltaTime) {
    MoveResult result;
    
    if (!body || !body->enabled) {
        result.finalPosition = body ? body->position : Math::Vec2::Zero();
        result.finalVelocity = body ? body->velocity : Math::Vec2::Zero();
        return result;
    }
    
    // 1. 应用重力和更新速度
    UpdateBody(body, deltaTime);
    
    // 2. 计算位移
    Math::Vec2 displacement = body->velocity * deltaTime;
    
    // 3. 执行移动
    return MoveAndCollideNoGravity(body, displacement);
}

MoveResult PlatformPhysics::MoveAndCollideNoGravity(PhysicsBody2D* body, const Math::Vec2& displacement) {
    MoveResult result;
    result.finalPosition = body->position;
    result.finalVelocity = body->velocity;
    
    if (!body || !body->enabled || body->IsStatic()) {
        return result;
    }
    
    /**
     * 分轴碰撞检测：
     * 
     * 为什么要分轴？
     * 如果同时移动X和Y，当发生碰撞时，我们不知道是撞到了墙还是地面。
     * 分轴处理可以准确判断碰撞方向。
     */
    
    // ========================================================================
    // 步骤1：先移动X轴
    // ========================================================================
    
    if (std::abs(displacement.x) > 0.0001f) {
        body->position.x += displacement.x;
        
        // 检测X轴碰撞
        AABB bodyAABB = body->GetAABB();
        
        for (PhysicsBody2D* other : m_bodies) {
            if (other == body || !other->enabled) continue;
            if (!body->CanCollideWith(*other)) continue;
            
            // 单向平台只检测Y轴碰撞，跳过X轴
            if (other->isOneWayPlatform) continue;
            
            AABB otherAABB = other->GetAABB();
            
            if (bodyAABB.Intersects(otherAABB)) {
                // 计算穿透深度
                Math::Vec2 penetration = bodyAABB.GetPenetration(otherAABB);
                
                // X轴碰撞
                if (displacement.x > 0) {
                    // 向右移动，撞到左边
                    body->position.x -= std::abs(penetration.x);
                    result.hitWallRight = true;
                    body->velocity.x = 0;
                } else {
                    // 向左移动，撞到右边
                    body->position.x += std::abs(penetration.x);
                    result.hitWallLeft = true;
                    body->velocity.x = 0;
                }
                
                // 记录碰撞
                CollisionInfo info;
                info.collided = true;
                info.other = other;
                info.penetration = std::abs(penetration.x);
                info.normal = (displacement.x > 0) ? Math::Vec2(-1, 0) : Math::Vec2(1, 0);
                info.fromRight = displacement.x > 0;
                info.fromLeft = displacement.x < 0;
                result.collisions.push_back(info);
                
                // 更新AABB
                bodyAABB = body->GetAABB();
            }
        }
    }
    
    // ========================================================================
    // 步骤2：再移动Y轴
    // ========================================================================
    
    if (std::abs(displacement.y) > 0.0001f) {
        body->position.y += displacement.y;
        
        // 检测Y轴碰撞
        AABB bodyAABB = body->GetAABB();
        
        for (PhysicsBody2D* other : m_bodies) {
            if (other == body || !other->enabled) continue;
            if (!body->CanCollideWith(*other)) continue;
            
            // 检查单向平台
            if (other->isOneWayPlatform) {
                if (!ShouldCollideWithOneWay(body, other, Math::Vec2(0, displacement.y))) {
                    continue;
                }
            }
            
            AABB otherAABB = other->GetAABB();
            
            if (bodyAABB.Intersects(otherAABB)) {
                // 计算穿透深度
                Math::Vec2 penetration = bodyAABB.GetPenetration(otherAABB);
                
                // Y轴碰撞
                if (displacement.y < 0) {
                    // 向下移动，撞到地面
                    body->position.y += std::abs(penetration.y);
                    result.hitGround = true;
                    body->velocity.y = 0;
                } else {
                    // 向上移动，撞到天花板
                    body->position.y -= std::abs(penetration.y);
                    result.hitCeiling = true;
                    body->velocity.y = 0;
                }
                
                // 记录碰撞
                CollisionInfo info;
                info.collided = true;
                info.other = other;
                info.penetration = std::abs(penetration.y);
                info.normal = (displacement.y < 0) ? Math::Vec2(0, 1) : Math::Vec2(0, -1);
                info.fromTop = displacement.y < 0;
                info.fromBottom = displacement.y > 0;
                result.collisions.push_back(info);
                
                // 更新AABB
                bodyAABB = body->GetAABB();
            }
        }
    }
    
    result.finalPosition = body->position;
    result.finalVelocity = body->velocity;
    
    return result;
}

// ============================================================================
// 检测方法
// ============================================================================

bool PlatformPhysics::IsOnGround(const PhysicsBody2D* body) const {
    if (!body || !body->enabled) return false;
    
    /**
     * 地面检测原理：
     * 
     * 在角色脚下创建一个薄薄的检测盒，检查是否与地面重叠
     * 
     *     ┌─────┐
     *     │角色 │
     *     └─────┘
     *     ┌─────┐  ← 检测盒（高度很小，比如2像素）
     *     └─────┘
     *   ══════════  地面
     */
    
    AABB footBox;
    footBox.center = Math::Vec2(body->position.x, body->GetBottom() - groundCheckDistance * 0.5f);
    footBox.halfSize = Math::Vec2(body->size.x * 0.4f, groundCheckDistance * 0.5f);
    
    for (const PhysicsBody2D* other : m_bodies) {
        if (other == body || !other->enabled) continue;
        if (!body->CanCollideWith(*other)) continue;
        
        // 单向平台也算地面
        AABB otherAABB = other->GetAABB();
        
        if (footBox.Intersects(otherAABB)) {
            // 确保是站在上面而不是悬在空中
            if (body->GetBottom() >= other->GetTop() - groundCheckDistance - 1.0f) {
                return true;
            }
        }
    }
    
    return false;
}

bool PlatformPhysics::IsOnWall(const PhysicsBody2D* body, bool checkLeft) const {
    if (!body || !body->enabled) return false;
    
    /**
     * 墙壁检测原理：
     * 
     * 在角色侧面创建一个检测盒
     * 
     *   ┌┐┌─────┐
     *   │││角色 │
     *   └┘└─────┘
     *   ↑
     *   检测盒
     */
    
    AABB sideBox;
    float offsetX = checkLeft ? 
        (body->GetLeft() - wallCheckDistance * 0.5f) :
        (body->GetRight() + wallCheckDistance * 0.5f);
    
    sideBox.center = Math::Vec2(offsetX, body->position.y);
    sideBox.halfSize = Math::Vec2(wallCheckDistance * 0.5f, body->size.y * 0.4f);
    
    for (const PhysicsBody2D* other : m_bodies) {
        if (other == body || !other->enabled) continue;
        if (!body->CanCollideWith(*other)) continue;
        if (other->isOneWayPlatform) continue;  // 单向平台不算墙
        
        if (sideBox.Intersects(other->GetAABB())) {
            return true;
        }
    }
    
    return false;
}

bool PlatformPhysics::IsHittingCeiling(const PhysicsBody2D* body) const {
    if (!body || !body->enabled) return false;
    
    AABB headBox;
    headBox.center = Math::Vec2(body->position.x, body->GetTop() + groundCheckDistance * 0.5f);
    headBox.halfSize = Math::Vec2(body->size.x * 0.4f, groundCheckDistance * 0.5f);
    
    for (const PhysicsBody2D* other : m_bodies) {
        if (other == body || !other->enabled) continue;
        if (!body->CanCollideWith(*other)) continue;
        if (other->isOneWayPlatform) continue;  // 单向平台不算天花板
        
        if (headBox.Intersects(other->GetAABB())) {
            return true;
        }
    }
    
    return false;
}

bool PlatformPhysics::PointInSolid(const Math::Vec2& point) const {
    for (const PhysicsBody2D* body : m_bodies) {
        if (!body->enabled) continue;
        if (body->GetAABB().Contains(point)) {
            return true;
        }
    }
    return false;
}

PhysicsBody2D* PlatformPhysics::GetGroundBelow(const Math::Vec2& position, float maxDistance) const {
    PhysicsBody2D* closestGround = nullptr;
    float closestDistance = maxDistance;
    
    for (PhysicsBody2D* body : m_bodies) {
        if (!body->enabled || body->IsDynamic()) continue;
        
        AABB aabb = body->GetAABB();
        
        // 检查X是否在范围内
        if (position.x < aabb.GetMin().x || position.x > aabb.GetMax().x) {
            continue;
        }
        
        // 检查是否在下方
        float groundTop = aabb.GetMax().y;
        if (groundTop > position.y) continue;
        
        float distance = position.y - groundTop;
        if (distance < closestDistance) {
            closestDistance = distance;
            closestGround = body;
        }
    }
    
    return closestGround;
}

// ============================================================================
// 射线检测
// ============================================================================

CollisionInfo PlatformPhysics::Raycast(const Math::Vec2& origin, const Math::Vec2& direction,
                                        float maxDistance, unsigned int layerMask) const {
    CollisionInfo result;
    float closestT = maxDistance;
    
    Math::Vec2 dir = direction.Normalized();
    
    for (PhysicsBody2D* body : m_bodies) {
        if (!body->enabled) continue;
        if ((body->layer & layerMask) == 0) continue;
        
        AABB aabb = body->GetAABB();
        Math::Vec2 min = aabb.GetMin();
        Math::Vec2 max = aabb.GetMax();
        
        // 简单的射线-AABB相交检测
        float tMin = 0.0f;
        float tMax = maxDistance;
        
        // X轴
        if (std::abs(dir.x) > 0.0001f) {
            float t1 = (min.x - origin.x) / dir.x;
            float t2 = (max.x - origin.x) / dir.x;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
        } else {
            if (origin.x < min.x || origin.x > max.x) continue;
        }
        
        // Y轴
        if (std::abs(dir.y) > 0.0001f) {
            float t1 = (min.y - origin.y) / dir.y;
            float t2 = (max.y - origin.y) / dir.y;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);
        } else {
            if (origin.y < min.y || origin.y > max.y) continue;
        }
        
        if (tMin <= tMax && tMin >= 0 && tMin < closestT) {
            closestT = tMin;
            result.collided = true;
            result.other = body;
            result.penetration = tMin;
            
            // 计算法线
            Math::Vec2 hitPoint = origin + dir * tMin;
            Math::Vec2 center = aabb.center;
            Math::Vec2 toHit = hitPoint - center;
            
            if (std::abs(toHit.x / aabb.halfSize.x) > std::abs(toHit.y / aabb.halfSize.y)) {
                result.normal = Math::Vec2(toHit.x > 0 ? 1.0f : -1.0f, 0.0f);
            } else {
                result.normal = Math::Vec2(0.0f, toHit.y > 0 ? 1.0f : -1.0f);
            }
        }
    }
    
    return result;
}

// ============================================================================
// 单向平台处理
// ============================================================================

bool PlatformPhysics::ShouldCollideWithOneWay(const PhysicsBody2D* body, const PhysicsBody2D* platform,
                                               const Math::Vec2& velocity) const {
    /**
     * 单向平台碰撞条件：
     * 
     * 1. 玩家必须正在下落（velocity.y < 0）
     * 2. 玩家底部必须在平台顶部上方或刚好在顶部
     * 
     *     ┌─────┐  玩家
     *     │     │
     *     └──┬──┘
     *        │ ↓ 下落
     *   ═════════════  平台顶部
     *     单向平台
     * 
     * 这样玩家可以从下方跳上平台，但从上方落下时会站在平台上
     */
    
    // 必须正在下落
    if (velocity.y >= 0) {
        return false;
    }
    
    // 玩家底部必须在平台顶部上方（有一点容差）
    float playerBottom = body->GetBottom();
    float platformTop = platform->GetTop();
    
    // 容差：允许玩家底部比平台顶部低一点点（因为移动过程中可能会有穿透）
    const float tolerance = 4.0f;
    
    if (playerBottom < platformTop - tolerance) {
        return false;  // 玩家在平台下方，不碰撞
    }
    
    return true;
}

// ============================================================================
// 辅助方法
// ============================================================================

PhysicsBody2D PlatformPhysics::CreateGround(float x, float y, float width, float height) {
    PhysicsBody2D body;
    body.position = Math::Vec2(x + width * 0.5f, y + height * 0.5f);
    body.size = Math::Vec2(width, height);
    body.bodyType = BodyType::Static;
    body.gravityScale = 0.0f;
    body.layer = LAYER_GROUND;
    body.isOneWayPlatform = false;
    return body;
}

PhysicsBody2D PlatformPhysics::CreateOneWayPlatform(float x, float y, float width, float height) {
    PhysicsBody2D body;
    body.position = Math::Vec2(x + width * 0.5f, y + height * 0.5f);
    body.size = Math::Vec2(width, height);
    body.bodyType = BodyType::Static;
    body.gravityScale = 0.0f;
    body.layer = LAYER_PLATFORM;
    body.isOneWayPlatform = true;
    return body;
}

PhysicsBody2D PlatformPhysics::CreateWall(float x, float y, float width, float height) {
    PhysicsBody2D body;
    body.position = Math::Vec2(x + width * 0.5f, y + height * 0.5f);
    body.size = Math::Vec2(width, height);
    body.bodyType = BodyType::Static;
    body.gravityScale = 0.0f;
    body.layer = LAYER_WALL;
    body.isOneWayPlatform = false;
    return body;
}

} // namespace MiniEngine
