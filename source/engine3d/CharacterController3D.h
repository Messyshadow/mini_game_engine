/**
 * CharacterController3D.h - 3D角色控制器
 *
 * ============================================================================
 * 核心概念
 * ============================================================================
 *
 * 1. 重力：每帧给垂直速度加上重力加速度
 *    velocityY -= gravity * dt
 *
 * 2. 地面检测：从角色脚底向下发射射线，检测是否碰到地面
 *    如果射线命中距离 < groundCheckDist → 角色在地面上
 *
 * 3. 移动方向：相对于摄像机朝向
 *    按W → 向摄像机前方移动（忽略Y）
 *    按D → 向摄像机右方移动
 *
 * 4. 角色朝向：平滑转向移动方向
 *    targetYaw = atan2(moveDir.x, moveDir.z)
 *    currentYaw = lerp(currentYaw, targetYaw, smoothSpeed * dt)
 *
 * ============================================================================
 * 简化地面检测（射线 vs 平面）
 * ============================================================================
 *
 * 本章用最简单的方式：角色Y坐标不能低于地面高度。
 * 后续第27章引入PhysX后，会替换为真正的射线检测。
 *
 *   if (position.y < groundHeight + halfHeight)
 *       position.y = groundHeight + halfHeight
 *       grounded = true
 *       velocityY = 0
 */

#pragma once

#include "math/Math.h"
#include <cmath>
#include <algorithm>

namespace MiniEngine {

using Math::Vec3;

struct CharacterParams3D {
    // 移动
    float moveSpeed = 6.0f;
    float sprintSpeed = 12.0f;
    float acceleration = 20.0f;   // 加速度
    float deceleration = 15.0f;   // 减速（松手后）
    float airControl = 0.3f;      // 空中操控比例（1=完全控制，0=无控制）
    
    // 跳跃
    float jumpForce = 8.0f;
    float gravity = 20.0f;
    float maxFallSpeed = 30.0f;
    int maxJumps = 2;             // 1=单跳，2=二段跳
    
    // 冲刺
    float dashSpeed = 20.0f;
    float dashDuration = 0.2f;
    float dashCooldown = 1.0f;
    
    // 转向
    float turnSpeed = 15.0f;      // 角色朝向平滑转向速度
    
    // 体型
    float height = 1.8f;
    float radius = 0.3f;
    float groundHeight = 0.0f;   // 地面Y坐标
};

struct CharacterState3D {
    bool grounded = true;
    bool sprinting = false;
    bool dashing = false;
    int jumpCount = 0;
    float currentYaw = 0;         // 角色当前朝向（弧度）
    float targetYaw = 0;          // 目标朝向
    Vec3 velocity = Vec3(0,0,0);
    float dashTimer = 0;
    float dashCooldownTimer = 0;
    Vec3 dashDirection = Vec3(0,0,0);
};

class CharacterController3D {
public:
    CharacterParams3D params;
    CharacterState3D state;
    Vec3 position = Vec3(0, 0, 0);
    
    /**
     * 处理移动输入
     * @param inputForward 前进输入(-1~1)
     * @param inputRight 右移输入(-1~1)
     * @param cameraYaw 摄像机Yaw角度（度），移动方向相对摄像机
     * @param sprint 是否冲刺
     * @param dt 帧时间
     */
    void ProcessMovement(float inputForward, float inputRight, float cameraYaw, bool sprint, float dt) {
        if (state.dashing) return; // 冲刺中不处理移动
        
        state.sprinting = sprint && state.grounded;
        float maxSpeed = state.sprinting ? params.sprintSpeed : params.moveSpeed;
        
        // 计算相对摄像机的移动方向
        float camRad = cameraYaw * 0.0174533f;
        Vec3 camFwd(std::sin(camRad), 0, std::cos(camRad));
        Vec3 camRight(std::cos(camRad), 0, -std::sin(camRad));
        
        Vec3 moveDir = camFwd * inputForward + camRight * inputRight;
        float moveLen = std::sqrt(moveDir.x*moveDir.x + moveDir.z*moveDir.z);
        
        if (moveLen > 0.001f) {
            moveDir = moveDir * (1.0f / moveLen); // 归一化
            if (moveLen > 1.0f) moveLen = 1.0f;   // 钳制（对角线不超速）
            
            // 更新目标朝向
            state.targetYaw = std::atan2(moveDir.x, moveDir.z);
            
            // 加速
            float accel = state.grounded ? params.acceleration : params.acceleration * params.airControl;
            float targetVelX = moveDir.x * maxSpeed * moveLen;
            float targetVelZ = moveDir.z * maxSpeed * moveLen;
            
            state.velocity.x += (targetVelX - state.velocity.x) * std::min(1.0f, accel * dt);
            state.velocity.z += (targetVelZ - state.velocity.z) * std::min(1.0f, accel * dt);
        } else {
            // 减速
            float decel = state.grounded ? params.deceleration : params.deceleration * params.airControl;
            state.velocity.x *= std::max(0.0f, 1.0f - decel * dt);
            state.velocity.z *= std::max(0.0f, 1.0f - decel * dt);
        }
        
        // 平滑转向
        if (moveLen > 0.1f) {
            float diff = state.targetYaw - state.currentYaw;
            // 处理角度环绕（-π到π）
            while (diff > 3.14159f) diff -= 6.28318f;
            while (diff < -3.14159f) diff += 6.28318f;
            state.currentYaw += diff * std::min(1.0f, params.turnSpeed * dt);
        }
    }
    
    /**
     * 处理跳跃
     * @param jumpPressed 跳跃键刚按下
     */
    void ProcessJump(bool jumpPressed) {
        if (!jumpPressed) return;
        
        if (state.grounded || state.jumpCount < params.maxJumps) {
            state.velocity.y = params.jumpForce;
            state.grounded = false;
            state.jumpCount++;
        }
    }
    
    /**
     * 处理冲刺
     * @param dashPressed 冲刺键刚按下
     */
    void ProcessDash(bool dashPressed) {
        if (!dashPressed || state.dashing || state.dashCooldownTimer > 0) return;
        
        state.dashing = true;
        state.dashTimer = params.dashDuration;
        state.dashCooldownTimer = params.dashCooldown;
        
        // 冲刺方向 = 角色当前朝向
        state.dashDirection = Vec3(
            std::sin(state.currentYaw),
            0,
            std::cos(state.currentYaw));
    }
    
    /**
     * 每帧更新（重力、冲刺、地面检测）
     */
    void Update(float dt) {
        // 冲刺更新
        if (state.dashing) {
            state.dashTimer -= dt;
            if (state.dashTimer <= 0) {
                state.dashing = false;
                state.velocity.x = state.dashDirection.x * params.moveSpeed * 0.5f;
                state.velocity.z = state.dashDirection.z * params.moveSpeed * 0.5f;
            } else {
                position = position + state.dashDirection * (params.dashSpeed * dt);
                return; // 冲刺中跳过重力
            }
        }
        
        if (state.dashCooldownTimer > 0) state.dashCooldownTimer -= dt;
        
        // 重力
        if (!state.grounded) {
            state.velocity.y -= params.gravity * dt;
            if (state.velocity.y < -params.maxFallSpeed)
                state.velocity.y = -params.maxFallSpeed;
        }
        
        // 移动
        position = position + state.velocity * dt;
        
        // 简易地面检测
        float feetY = position.y;
        float groundY = params.groundHeight + params.height * 0.5f;
        
        if (feetY <= groundY) {
            position.y = groundY;
            if (state.velocity.y < 0) state.velocity.y = 0;
            state.grounded = true;
            state.jumpCount = 0;
        } else {
            state.grounded = false;
        }
    }
    
    /** 重置 */
    void Reset() {
        position = Vec3(0, params.height * 0.5f, 0);
        state = CharacterState3D();
    }
    
    /** 获取角色朝向的前方向 */
    Vec3 GetForward() const {
        return Vec3(std::sin(state.currentYaw), 0, std::cos(state.currentYaw));
    }
};

} // namespace MiniEngine
