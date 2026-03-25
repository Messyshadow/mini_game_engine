/**
 * PlatformController.h - 高品质2D平台跳跃控制器
 *
 * 包含所有银河恶魔城需要的移动特性：
 * - 加速度曲线移动（非瞬间满速）
 * - 可变高度跳跃（短按低跳，长按高跳）
 * - 土狼时间（Coyote Time）
 * - 跳跃缓冲（Jump Buffer）
 * - 蹬墙跳 + 墙面滑行
 * - 二段跳
 */

#pragma once

#include "math/Math.h"

namespace MiniEngine {

using Math::Vec2;

/**
 * 平台物理参数（全部可在编辑模式中实时调节）
 */
struct PlatformParams {
    // 水平移动
    float moveSpeed = 260.0f;       // 最大移动速度
    float acceleration = 1800.0f;   // 地面加速度
    float deceleration = 2200.0f;   // 地面减速度（松手后）
    float airAccel = 900.0f;        // 空中加速度（空中操控感弱一些）
    float airDecel = 600.0f;        // 空中减速度
    
    // 跳跃
    float jumpForce = 540.0f;       // 跳跃初速度
    float jumpCutMultiplier = 0.4f; // 松手时速度保留比例（越小=短跳越矮）
    float gravity = 1400.0f;        // 重力
    float fallGravityMult = 1.6f;   // 下落时重力倍率（下落更快，手感更爽）
    float maxFallSpeed = 700.0f;    // 最大下落速度
    
    // 土狼时间 & 跳跃缓冲
    bool enableCoyoteTime = true;
    float coyoteTime = 0.1f;        // 离开平台后仍可跳跃的时间
    bool enableJumpBuffer = true;
    float jumpBufferTime = 0.12f;   // 落地前按跳的缓冲时间
    
    // 二段跳
    bool enableDoubleJump = false;   // 默认关闭（需要拾取解锁）
    int maxAirJumps = 1;
    float airJumpForce = 460.0f;    // 空中跳比地面跳弱一点
    
    // 蹬墙跳
    bool enableWallJump = false;     // 默认关闭（需要拾取解锁）
    float wallSlideSpeed = 80.0f;   // 墙面滑行最大速度
    float wallJumpForceX = 280.0f;  // 墙跳水平力
    float wallJumpForceY = 480.0f;  // 墙跳垂直力
    float wallJumpLockTime = 0.15f; // 墙跳后短暂锁定输入（防止立刻贴回墙）
};

/**
 * 平台控制器状态（只读，外部查询用）
 */
struct PlatformState {
    bool grounded = false;
    bool touchingWallL = false;
    bool touchingWallR = false;
    bool isWallSliding = false;
    bool isJumping = false;
    int airJumpsUsed = 0;
    float facingDir = 1.0f;  // 1=右, -1=左
};

/**
 * PlatformController - 平台跳跃物理控制器
 */
class PlatformController {
public:
    PlatformParams params;
    PlatformState state;
    Vec2 velocity = Vec2(0, 0);
    
    /**
     * 处理水平移动输入
     * @param inputX -1=左, 0=无输入, 1=右
     * @param dt 帧时间
     */
    void ProcessMovement(float inputX, float dt) {
        if (m_wallJumpLockTimer > 0) {
            m_wallJumpLockTimer -= dt;
            return;  // 墙跳锁定期间不处理移动输入
        }
        
        if (inputX != 0) state.facingDir = inputX > 0 ? 1.0f : -1.0f;
        
        float accel, decel;
        if (state.grounded) {
            accel = params.acceleration;
            decel = params.deceleration;
        } else {
            accel = params.airAccel;
            decel = params.airDecel;
        }
        
        float targetSpeed = inputX * params.moveSpeed;
        float diff = targetSpeed - velocity.x;
        
        if (std::abs(diff) < 1.0f) {
            velocity.x = targetSpeed;
        } else {
            float rate = (std::abs(targetSpeed) > std::abs(velocity.x)) ? accel : decel;
            // 如果转向（从正速到负速或反过来），用加速度更快响应
            if ((velocity.x > 0 && targetSpeed < 0) || (velocity.x < 0 && targetSpeed > 0)) {
                rate = accel * 1.5f;
            }
            float change = (diff > 0 ? 1.0f : -1.0f) * rate * dt;
            if (std::abs(change) > std::abs(diff)) change = diff;
            velocity.x += change;
        }
    }
    
    /**
     * 处理跳跃输入
     * @param jumpPressed 跳跃键当前是否按下
     * @param jumpJustPressed 跳跃键这一帧刚按下
     * @param dt 帧时间
     */
    void ProcessJump(bool jumpPressed, bool jumpJustPressed, float dt) {
        // 更新计时器
        if (state.grounded) {
            m_coyoteTimer = params.coyoteTime;
            state.airJumpsUsed = 0;
            state.isJumping = false;
        } else {
            m_coyoteTimer -= dt;
        }
        
        if (jumpJustPressed) {
            m_jumpBufferTimer = params.jumpBufferTime;
        } else {
            m_jumpBufferTimer -= dt;
        }
        
        bool wantJump = m_jumpBufferTimer > 0 || jumpJustPressed;
        
        // 蹬墙跳
        if (wantJump && params.enableWallJump && state.isWallSliding) {
            float dir = state.touchingWallL ? 1.0f : -1.0f;
            velocity.x = dir * params.wallJumpForceX;
            velocity.y = params.wallJumpForceY;
            state.facingDir = dir;
            state.isJumping = true;
            m_coyoteTimer = 0;
            m_jumpBufferTimer = 0;
            m_wallJumpLockTimer = params.wallJumpLockTime;
            return;
        }
        
        // 普通跳（含土狼时间）
        bool canCoyoteJump = params.enableCoyoteTime ? (m_coyoteTimer > 0) : state.grounded;
        bool canBufferJump = params.enableJumpBuffer ? (m_jumpBufferTimer > 0) : jumpJustPressed;
        
        if (canBufferJump && canCoyoteJump) {
            velocity.y = params.jumpForce;
            state.isJumping = true;
            m_coyoteTimer = 0;
            m_jumpBufferTimer = 0;
            return;
        }
        
        // 二段跳
        if (jumpJustPressed && params.enableDoubleJump &&
            !state.grounded && m_coyoteTimer <= 0 &&
            state.airJumpsUsed < params.maxAirJumps && !state.isWallSliding) {
            velocity.y = params.airJumpForce;
            state.airJumpsUsed++;
            state.isJumping = true;
            m_jumpBufferTimer = 0;
            return;
        }
        
        // 可变高度跳跃：松手时削减上升速度
        if (!jumpPressed && velocity.y > 0 && state.isJumping) {
            velocity.y *= params.jumpCutMultiplier;
            state.isJumping = false;
        }
    }
    
    /**
     * 应用重力
     * @param dt 帧时间
     */
    void ApplyGravity(float dt) {
        // 墙面滑行
        if (state.isWallSliding) {
            if (velocity.y < -params.wallSlideSpeed) {
                velocity.y = -params.wallSlideSpeed;
            }
            return;
        }
        
        // 下落时加重重力（让跳跃弧线更有"重量感"）
        float grav = params.gravity;
        if (velocity.y < 0) {
            grav *= params.fallGravityMult;
        }
        
        velocity.y -= grav * dt;
        
        if (velocity.y < -params.maxFallSpeed) {
            velocity.y = -params.maxFallSpeed;
        }
    }
    
    /**
     * 碰撞后更新状态（在碰撞检测后调用）
     */
    void PostCollision(bool grounded, bool wallL, bool wallR, float inputX) {
        state.grounded = grounded;
        state.touchingWallL = wallL && !grounded;
        state.touchingWallR = wallR && !grounded;
        
        // 判断是否在墙面滑行
        state.isWallSliding = false;
        if (params.enableWallJump && !grounded) {
            if ((state.touchingWallL && inputX < 0) || (state.touchingWallR && inputX > 0)) {
                state.isWallSliding = true;
            }
        }
        
        if (grounded) {
            state.isJumping = false;
        }
    }
    
    /** 重置所有状态 */
    void Reset() {
        velocity = Vec2(0, 0);
        state = PlatformState();
        m_coyoteTimer = 0;
        m_jumpBufferTimer = 0;
        m_wallJumpLockTimer = 0;
    }
    
private:
    float m_coyoteTimer = 0;
    float m_jumpBufferTimer = 0;
    float m_wallJumpLockTimer = 0;
};

} // namespace MiniEngine
