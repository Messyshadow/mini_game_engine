/**
 * CharacterController2D.h - 2D角色控制器
 * 
 * ============================================================================
 * 概述
 * ============================================================================
 * 
 * 角色控制器负责将玩家输入转换为角色移动。
 * 它不直接处理物理碰撞（那是 PlatformPhysics 的工作），
 * 而是专注于"控制手感"的优化。
 * 
 * 主要功能：
 * 1. 土狼时间（Coyote Time）- 离开平台后短暂可跳
 * 2. 跳跃缓冲（Jump Buffer）- 落地前按跳跃会记住
 * 3. 加速度曲线 - 平滑的加速和减速
 * 4. 可变高度跳跃 - 长按跳更高
 * 5. 空中控制 - 空中可以调整方向
 * 6. 蹬墙跳（可选）- 贴墙时可以反向跳
 * 
 * ============================================================================
 * 使用示例
 * ============================================================================
 * 
 *   // 创建控制器
 *   CharacterController2D controller;
 *   controller.Initialize(&physicsBody, &physicsWorld);
 *   
 *   // 每帧更新
 *   CharacterInput input;
 *   input.moveX = GetHorizontalInput();  // -1, 0, 或 1
 *   input.jumpPressed = IsJumpPressed();
 *   input.jumpHeld = IsJumpHeld();
 *   
 *   controller.Update(input, deltaTime);
 */

#ifndef MINI_ENGINE_CHARACTER_CONTROLLER_2D_H
#define MINI_ENGINE_CHARACTER_CONTROLLER_2D_H

#include "math/Math.h"
#include "physics/PhysicsBody2D.h"
#include "physics/PlatformPhysics.h"

namespace MiniEngine {

// ============================================================================
// 输入结构
// ============================================================================

/**
 * 角色输入
 * 
 * 将原始输入（按键状态）转换为标准化的输入结构
 */
struct CharacterInput {
    float moveX = 0.0f;         // 水平输入 (-1 到 1)
    float moveY = 0.0f;         // 垂直输入（用于下跳等）
    bool jumpPressed = false;   // 跳跃键刚按下
    bool jumpHeld = false;      // 跳跃键按住
    bool jumpReleased = false;  // 跳跃键刚松开
};

// ============================================================================
// 角色状态
// ============================================================================

/**
 * 角色状态枚举
 */
enum class CharacterState {
    Grounded,       // 在地面
    Jumping,        // 跳跃上升中
    Falling,        // 下落中
    WallSliding,    // 贴墙滑落
    WallJumping     // 蹬墙跳中
};

// ============================================================================
// 控制器设置
// ============================================================================

/**
 * 角色控制器设置
 * 
 * 所有可调节的参数都在这里
 */
struct CharacterSettings {
    // --------------------------------------------------------------------
    // 移动参数
    // --------------------------------------------------------------------
    
    float maxSpeed = 300.0f;            // 最大水平速度
    float groundAcceleration = 2000.0f; // 地面加速度
    float groundDeceleration = 2500.0f; // 地面减速度（松开按键时）
    float airAcceleration = 1200.0f;    // 空中加速度
    float airDeceleration = 600.0f;     // 空中减速度
    float turnAroundBoost = 1.5f;       // 转向加速倍数（更快响应）
    
    // --------------------------------------------------------------------
    // 跳跃参数
    // --------------------------------------------------------------------
    
    float jumpForce = 550.0f;           // 跳跃初速度
    float jumpCutMultiplier = 0.5f;     // 松开跳跃键时速度乘数
    float fallMultiplier = 1.5f;        // 下落时重力倍数（下落更快）
    float lowJumpMultiplier = 2.0f;     // 轻跳时重力倍数
    
    // --------------------------------------------------------------------
    // 土狼时间和跳跃缓冲
    // --------------------------------------------------------------------
    
    float coyoteTime = 0.1f;            // 土狼时间窗口（秒）
    float jumpBufferTime = 0.1f;        // 跳跃缓冲窗口（秒）
    
    // --------------------------------------------------------------------
    // 蹬墙跳参数（可选功能）
    // --------------------------------------------------------------------
    
    bool wallJumpEnabled = true;        // 是否启用蹬墙跳
    float wallSlideSpeed = 100.0f;      // 贴墙滑落最大速度
    float wallJumpForceX = 350.0f;      // 蹬墙跳水平力
    float wallJumpForceY = 450.0f;      // 蹬墙跳垂直力
    float wallJumpControlDelay = 0.15f; // 蹬墙跳后禁止反向输入的时间
    float wallCoyoteTime = 0.08f;       // 墙壁土狼时间
    
    // --------------------------------------------------------------------
    // 开关（用于调试）
    // --------------------------------------------------------------------
    
    bool coyoteTimeEnabled = true;      // 启用土狼时间
    bool jumpBufferEnabled = true;      // 启用跳跃缓冲
    bool accelerationEnabled = true;    // 启用加速度曲线
};

// ============================================================================
// 角色控制器类
// ============================================================================

/**
 * 2D角色控制器
 */
class CharacterController2D {
public:
    // ========================================================================
    // 构造和初始化
    // ========================================================================
    
    CharacterController2D();
    ~CharacterController2D() = default;
    
    /**
     * 初始化控制器
     * 
     * @param body 控制的物理体
     * @param physics 物理世界
     */
    void Initialize(PhysicsBody2D* body, PlatformPhysics* physics);
    
    // ========================================================================
    // 更新
    // ========================================================================
    
    /**
     * 更新控制器
     * 
     * 这是每帧必须调用的主要方法
     * 
     * @param input 玩家输入
     * @param deltaTime 时间步长
     */
    void Update(const CharacterInput& input, float deltaTime);
    
    // ========================================================================
    // 设置访问
    // ========================================================================
    
    /** 获取设置（可修改） */
    CharacterSettings& GetSettings() { return m_settings; }
    
    /** 获取设置（只读） */
    const CharacterSettings& GetSettings() const { return m_settings; }
    
    // ========================================================================
    // 状态查询
    // ========================================================================
    
    /** 获取当前状态 */
    CharacterState GetState() const { return m_state; }
    
    /** 获取状态名称（用于调试） */
    const char* GetStateName() const;
    
    /** 是否在地面上 */
    bool IsGrounded() const { return m_isGrounded; }
    
    /** 是否在跳跃中 */
    bool IsJumping() const { return m_state == CharacterState::Jumping; }
    
    /** 是否在下落中 */
    bool IsFalling() const { return m_state == CharacterState::Falling; }
    
    /** 是否贴墙 */
    bool IsOnWall() const { return m_isOnWallLeft || m_isOnWallRight; }
    
    /** 获取面朝方向（-1=左, 1=右） */
    int GetFacingDirection() const { return m_facingDirection; }
    
    // ========================================================================
    // 调试信息
    // ========================================================================
    
    /** 获取土狼时间剩余 */
    float GetCoyoteTimeLeft() const { return m_coyoteTimeLeft; }
    
    /** 获取跳跃缓冲剩余 */
    float GetJumpBufferLeft() const { return m_jumpBufferTimeLeft; }
    
    /** 获取墙壁土狼时间剩余 */
    float GetWallCoyoteTimeLeft() const { return m_wallCoyoteTimeLeft; }
    
    /** 获取蹬墙跳控制延迟剩余 */
    float GetWallJumpControlDelay() const { return m_wallJumpControlDelayLeft; }
    
    /** 获取最后一次跳跃类型（用于调试） */
    const char* GetLastJumpType() const { return m_lastJumpType; }
    
private:
    // ========================================================================
    // 内部方法
    // ========================================================================
    
    /** 更新地面和墙壁检测 */
    void UpdateGroundedState();
    
    /** 更新土狼时间 */
    void UpdateCoyoteTime(float deltaTime);
    
    /** 更新跳跃缓冲 */
    void UpdateJumpBuffer(const CharacterInput& input, float deltaTime);
    
    /** 处理水平移动 */
    void HandleHorizontalMovement(const CharacterInput& input, float deltaTime);
    
    /** 处理跳跃 */
    void HandleJump(const CharacterInput& input);
    
    /** 处理可变高度跳跃 */
    void HandleVariableJump(const CharacterInput& input, float deltaTime);
    
    /** 处理蹬墙跳 */
    void HandleWallJump(const CharacterInput& input);
    
    /** 处理墙壁滑落 */
    void HandleWallSlide(float deltaTime);
    
    /** 更新角色状态 */
    void UpdateState();
    
    /** 应用物理 */
    void ApplyPhysics(float deltaTime);
    
    /** 工具函数：趋近目标值 */
    static float MoveTowards(float current, float target, float maxDelta);
    
    // ========================================================================
    // 成员变量
    // ========================================================================
    
    // 引用
    PhysicsBody2D* m_body = nullptr;
    PlatformPhysics* m_physics = nullptr;
    
    // 设置
    CharacterSettings m_settings;
    
    // 状态
    CharacterState m_state = CharacterState::Grounded;
    bool m_isGrounded = false;
    bool m_wasGrounded = false;         // 上一帧是否在地面
    bool m_isOnWallLeft = false;
    bool m_isOnWallRight = false;
    bool m_wasOnWall = false;           // 上一帧是否贴墙
    int m_facingDirection = 1;          // 面朝方向
    int m_lastWallDirection = 0;        // 上次贴的墙方向
    
    // 计时器
    float m_coyoteTimeLeft = 0.0f;      // 土狼时间剩余
    float m_jumpBufferTimeLeft = 0.0f;  // 跳跃缓冲剩余
    float m_wallCoyoteTimeLeft = 0.0f;  // 墙壁土狼时间剩余
    float m_wallJumpControlDelayLeft = 0.0f;  // 蹬墙跳控制延迟剩余
    
    // 调试
    const char* m_lastJumpType = "None";
};

} // namespace MiniEngine

#endif // MINI_ENGINE_CHARACTER_CONTROLLER_2D_H
