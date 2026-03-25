/**
 * CharacterController2D.cpp - 2D角色控制器实现
 * 
 * ============================================================================
 * 实现说明
 * ============================================================================
 * 
 * 这个文件实现了角色控制器的所有功能。
 * 每个方法都有详细注释，解释其作用和原理。
 */

#include "CharacterController2D.h"
#include <algorithm>
#include <cmath>

namespace MiniEngine {

// ============================================================================
// 构造函数
// ============================================================================

CharacterController2D::CharacterController2D()
    : m_body(nullptr)
    , m_physics(nullptr)
    , m_state(CharacterState::Falling)
    , m_isGrounded(false)
    , m_wasGrounded(false)
    , m_isOnWallLeft(false)
    , m_isOnWallRight(false)
    , m_wasOnWall(false)
    , m_facingDirection(1)
    , m_lastWallDirection(0)
    , m_coyoteTimeLeft(0.0f)
    , m_jumpBufferTimeLeft(0.0f)
    , m_wallCoyoteTimeLeft(0.0f)
    , m_wallJumpControlDelayLeft(0.0f)
    , m_lastJumpType("None")
{
}

// ============================================================================
// 初始化
// ============================================================================

void CharacterController2D::Initialize(PhysicsBody2D* body, PlatformPhysics* physics) {
    m_body = body;
    m_physics = physics;
    m_state = CharacterState::Falling;
    m_isGrounded = false;
    m_coyoteTimeLeft = 0.0f;
    m_jumpBufferTimeLeft = 0.0f;
}

// ============================================================================
// 主更新方法
// ============================================================================

/**
 * Update - 每帧更新
 * 
 * 执行顺序很重要：
 * 1. 先检测当前状态（地面、墙壁）
 * 2. 更新计时器（土狼时间、跳跃缓冲）
 * 3. 处理输入（移动、跳跃）
 * 4. 应用物理
 * 5. 更新状态机
 */
void CharacterController2D::Update(const CharacterInput& input, float deltaTime) {
    if (!m_body || !m_physics) return;
    
    // 保存上一帧状态
    m_wasGrounded = m_isGrounded;
    m_wasOnWall = m_isOnWallLeft || m_isOnWallRight;
    
    // 步骤1：检测地面和墙壁
    UpdateGroundedState();
    
    // 步骤2：更新计时器
    UpdateCoyoteTime(deltaTime);
    UpdateJumpBuffer(input, deltaTime);
    
    // 步骤3：处理输入
    HandleHorizontalMovement(input, deltaTime);
    HandleJump(input);
    HandleVariableJump(input, deltaTime);
    
    // 步骤4：处理墙壁相关
    if (m_settings.wallJumpEnabled) {
        HandleWallSlide(deltaTime);
        HandleWallJump(input);
    }
    
    // 步骤5：应用物理
    ApplyPhysics(deltaTime);
    
    // 步骤6：更新状态
    UpdateState();
    
    // 更新面朝方向
    if (input.moveX > 0.1f) m_facingDirection = 1;
    else if (input.moveX < -0.1f) m_facingDirection = -1;
}

// ============================================================================
// 地面检测
// ============================================================================

/**
 * UpdateGroundedState - 更新地面和墙壁状态
 * 
 * 使用 PlatformPhysics 的检测方法
 */
void CharacterController2D::UpdateGroundedState() {
    m_isGrounded = m_physics->IsOnGround(m_body);
    m_isOnWallLeft = m_physics->IsOnWall(m_body, true);
    m_isOnWallRight = m_physics->IsOnWall(m_body, false);
    
    // 记录上次贴墙方向
    if (m_isOnWallLeft) m_lastWallDirection = -1;
    else if (m_isOnWallRight) m_lastWallDirection = 1;
}

// ============================================================================
// 土狼时间
// ============================================================================

/**
 * UpdateCoyoteTime - 更新土狼时间
 * 
 * 原理：
 * - 在地面时，土狼时间保持满值
 * - 离开地面后，开始倒计时
 * - 跳跃会消耗土狼时间
 * 
 * 同样的逻辑也用于墙壁土狼时间
 */
void CharacterController2D::UpdateCoyoteTime(float deltaTime) {
    // 地面土狼时间
    if (m_isGrounded) {
        // 在地面上，重置土狼时间
        m_coyoteTimeLeft = m_settings.coyoteTime;
    } else {
        // 离开地面，开始倒计时
        if (m_coyoteTimeLeft > 0) {
            m_coyoteTimeLeft -= deltaTime;
        }
    }
    
    // 墙壁土狼时间
    if (m_isOnWallLeft || m_isOnWallRight) {
        m_wallCoyoteTimeLeft = m_settings.wallCoyoteTime;
    } else {
        if (m_wallCoyoteTimeLeft > 0) {
            m_wallCoyoteTimeLeft -= deltaTime;
        }
    }
    
    // 蹬墙跳控制延迟倒计时
    if (m_wallJumpControlDelayLeft > 0) {
        m_wallJumpControlDelayLeft -= deltaTime;
    }
}

// ============================================================================
// 跳跃缓冲
// ============================================================================

/**
 * UpdateJumpBuffer - 更新跳跃缓冲
 * 
 * 原理：
 * - 按下跳跃键时，设置缓冲时间
 * - 缓冲时间持续倒计时
 * - 在缓冲时间内如果能跳，就执行跳跃
 */
void CharacterController2D::UpdateJumpBuffer(const CharacterInput& input, float deltaTime) {
    // 按下跳跃键，设置缓冲
    if (input.jumpPressed) {
        m_jumpBufferTimeLeft = m_settings.jumpBufferTime;
    }
    
    // 缓冲倒计时
    if (m_jumpBufferTimeLeft > 0) {
        m_jumpBufferTimeLeft -= deltaTime;
    }
}

// ============================================================================
// 水平移动
// ============================================================================

/**
 * HandleHorizontalMovement - 处理水平移动
 * 
 * 特点：
 * - 使用加速度曲线，不是直接设置速度
 * - 地面和空中有不同的加速度
 * - 转向时有额外加速（更快响应）
 */
void CharacterController2D::HandleHorizontalMovement(const CharacterInput& input, float deltaTime) {
    // 蹬墙跳后短暂禁止反向输入
    float effectiveInput = input.moveX;
    if (m_wallJumpControlDelayLeft > 0) {
        // 只允许同向或不输入，禁止反向
        if (effectiveInput * m_body->velocity.x < 0) {
            effectiveInput = 0;
        }
    }
    
    // 计算目标速度
    float targetSpeed = effectiveInput * m_settings.maxSpeed;
    
    // 如果禁用加速度曲线，直接设置速度
    if (!m_settings.accelerationEnabled) {
        m_body->velocity.x = targetSpeed;
        return;
    }
    
    // 选择加速度（地面/空中）
    float acceleration, deceleration;
    if (m_isGrounded) {
        acceleration = m_settings.groundAcceleration;
        deceleration = m_settings.groundDeceleration;
    } else {
        acceleration = m_settings.airAcceleration;
        deceleration = m_settings.airDeceleration;
    }
    
    // 判断是加速还是减速
    float currentSpeed = m_body->velocity.x;
    bool isAccelerating = std::abs(targetSpeed) > std::abs(currentSpeed) ||
                          (targetSpeed * currentSpeed < 0);  // 转向
    
    // 转向时使用更高的加速度
    if (targetSpeed * currentSpeed < 0 && std::abs(effectiveInput) > 0.1f) {
        acceleration *= m_settings.turnAroundBoost;
    }
    
    // 应用加速或减速
    float rate = isAccelerating ? acceleration : deceleration;
    m_body->velocity.x = MoveTowards(currentSpeed, targetSpeed, rate * deltaTime);
}

// ============================================================================
// 跳跃
// ============================================================================

/**
 * HandleJump - 处理跳跃
 * 
 * 跳跃条件（满足任一）：
 * 1. 在地面上
 * 2. 在土狼时间内（如果启用）
 * 
 * 跳跃触发（满足任一）：
 * 1. 刚按下跳跃键
 * 2. 有跳跃缓冲（如果启用）
 */
void CharacterController2D::HandleJump(const CharacterInput& input) {
    // 检查是否想要跳跃
    bool wantJump = input.jumpPressed;
    
    // 如果启用跳跃缓冲，检查缓冲
    if (m_settings.jumpBufferEnabled && m_jumpBufferTimeLeft > 0) {
        wantJump = true;
    }
    
    if (!wantJump) return;
    
    // 检查是否能跳跃
    bool canJump = m_isGrounded;
    
    // 如果启用土狼时间，检查土狼时间
    if (m_settings.coyoteTimeEnabled && m_coyoteTimeLeft > 0) {
        canJump = true;
    }
    
    // 执行跳跃
    if (canJump) {
        m_body->velocity.y = m_settings.jumpForce;
        
        // 消耗土狼时间和跳跃缓冲
        m_coyoteTimeLeft = 0;
        m_jumpBufferTimeLeft = 0;
        
        // 记录跳跃类型（调试用）
        if (m_isGrounded) {
            m_lastJumpType = "Ground Jump";
        } else {
            m_lastJumpType = "Coyote Jump";
        }
        
        m_state = CharacterState::Jumping;
    }
}

// ============================================================================
// 可变高度跳跃
// ============================================================================

/**
 * HandleVariableJump - 处理可变高度跳跃
 * 
 * 原理：
 * 1. 松开跳跃键时，如果还在上升，减少上升速度
 * 2. 下落时应用更大的重力，让角色落得更快
 * 3. 上升时如果没按住跳跃键，也应用更大重力（轻跳）
 */
void CharacterController2D::HandleVariableJump(const CharacterInput& input, float deltaTime) {
    float vy = m_body->velocity.y;
    
    // 松开跳跃键时减速（用于实现轻按小跳）
    if (input.jumpReleased && vy > 0) {
        m_body->velocity.y *= m_settings.jumpCutMultiplier;
    }
    
    // 下落时增加重力
    if (vy < 0) {
        // 正在下落，应用下落重力倍数
        float extraGravity = m_physics->gravity.y * (m_settings.fallMultiplier - 1.0f) * deltaTime;
        m_body->velocity.y += extraGravity;
    } 
    // 上升时如果没按住跳跃键，应用更大重力
    else if (vy > 0 && !input.jumpHeld) {
        float extraGravity = m_physics->gravity.y * (m_settings.lowJumpMultiplier - 1.0f) * deltaTime;
        m_body->velocity.y += extraGravity;
    }
}

// ============================================================================
// 墙壁滑落
// ============================================================================

/**
 * HandleWallSlide - 处理墙壁滑落
 * 
 * 当角色贴墙且下落时，限制下落速度
 */
void CharacterController2D::HandleWallSlide(float deltaTime) {
    bool onWall = m_isOnWallLeft || m_isOnWallRight;
    
    if (onWall && !m_isGrounded && m_body->velocity.y < 0) {
        // 限制下落速度
        if (m_body->velocity.y < -m_settings.wallSlideSpeed) {
            m_body->velocity.y = -m_settings.wallSlideSpeed;
        }
    }
}

// ============================================================================
// 蹬墙跳
// ============================================================================

/**
 * HandleWallJump - 处理蹬墙跳
 * 
 * 条件：
 * - 贴墙或在墙壁土狼时间内
 * - 不在地面
 * - 按下跳跃键
 */
void CharacterController2D::HandleWallJump(const CharacterInput& input) {
    if (!m_settings.wallJumpEnabled) return;
    
    // 检查是否想要跳跃
    bool wantJump = input.jumpPressed;
    if (m_settings.jumpBufferEnabled && m_jumpBufferTimeLeft > 0) {
        wantJump = true;
    }
    
    if (!wantJump) return;
    if (m_isGrounded) return;  // 在地面上用普通跳跃
    
    // 检查是否能蹬墙跳
    bool canWallJump = m_isOnWallLeft || m_isOnWallRight;
    
    // 墙壁土狼时间
    if (m_wallCoyoteTimeLeft > 0 && m_lastWallDirection != 0) {
        canWallJump = true;
    }
    
    if (!canWallJump) return;
    
    // 计算跳跃方向（向墙的反方向）
    int wallDir = m_isOnWallLeft ? -1 : (m_isOnWallRight ? 1 : m_lastWallDirection);
    int jumpDir = -wallDir;  // 反方向
    
    // 执行蹬墙跳
    m_body->velocity.x = jumpDir * m_settings.wallJumpForceX;
    m_body->velocity.y = m_settings.wallJumpForceY;
    
    // 消耗计时器
    m_coyoteTimeLeft = 0;
    m_jumpBufferTimeLeft = 0;
    m_wallCoyoteTimeLeft = 0;
    
    // 设置控制延迟（防止立即贴回墙上）
    m_wallJumpControlDelayLeft = m_settings.wallJumpControlDelay;
    
    m_lastJumpType = "Wall Jump";
    m_state = CharacterState::WallJumping;
}

// ============================================================================
// 应用物理
// ============================================================================

/**
 * ApplyPhysics - 应用物理更新
 * 
 * 调用物理系统的 MoveAndCollide
 */
void CharacterController2D::ApplyPhysics(float deltaTime) {
    m_physics->MoveAndCollide(m_body, deltaTime);
}

// ============================================================================
// 状态更新
// ============================================================================

/**
 * UpdateState - 更新状态机
 */
void CharacterController2D::UpdateState() {
    bool onWall = m_isOnWallLeft || m_isOnWallRight;
    
    if (m_isGrounded) {
        m_state = CharacterState::Grounded;
    } else if (onWall && !m_isGrounded && m_body->velocity.y < 0) {
        m_state = CharacterState::WallSliding;
    } else if (m_body->velocity.y > 0) {
        m_state = CharacterState::Jumping;
    } else {
        m_state = CharacterState::Falling;
    }
}

// ============================================================================
// 辅助方法
// ============================================================================

const char* CharacterController2D::GetStateName() const {
    switch (m_state) {
        case CharacterState::Grounded:    return "Grounded";
        case CharacterState::Jumping:     return "Jumping";
        case CharacterState::Falling:     return "Falling";
        case CharacterState::WallSliding: return "Wall Sliding";
        case CharacterState::WallJumping: return "Wall Jumping";
        default: return "Unknown";
    }
}

float CharacterController2D::MoveTowards(float current, float target, float maxDelta) {
    if (std::abs(target - current) <= maxDelta) {
        return target;
    }
    return current + (target > current ? maxDelta : -maxDelta);
}

} // namespace MiniEngine
