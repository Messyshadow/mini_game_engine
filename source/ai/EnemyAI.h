/**
 * EnemyAI.h - 敌人AI系统
 * 
 * ============================================================================
 * 概述
 * ============================================================================
 * 
 * 基于有限状态机（FSM）的敌人AI系统。
 * 每个敌人拥有独立的AI组件，驱动其行为决策。
 * 
 * ============================================================================
 * 状态转换图
 * ============================================================================
 * 
 *                 ┌─────────────────────────┐
 *                 │                         │
 *    ┌──────┐  发现玩家  ┌──────┐  进入攻击范围  ┌──────┐
 *    │ 巡逻 │ ────────→ │ 追击 │ ────────────→ │ 攻击 │
 *    │Patrol│ ←──────── │Chase │ ←──────────── │Attack│
 *    └──────┘  丢失目标  └──────┘  目标离开范围  └──────┘
 *        │                  │                      │
 *        │              ┌──────┐                   │
 *        └────────────→ │ 死亡 │ ←─────────────────┘
 *           HP <= 0     │ Dead │      HP <= 0
 *                       └──────┘
 * 
 *   还有两个辅助状态：
 *   - Idle（空闲）：巡逻路径端点停留片刻
 *   - Hurt（受击）：被击中时短暂硬直
 * 
 * ============================================================================
 * 使用示例
 * ============================================================================
 * 
 *   EnemyAI ai;
 *   ai.SetPatrolRange(startX, endX);
 *   ai.SetDetectionRange(200.0f);
 *   ai.SetAttackRange(60.0f);
 *   
 *   // 每帧更新
 *   ai.Update(dt, enemyPos, playerPos);
 *   float moveDir = ai.GetMoveDirection();  // -1, 0, 1
 *   bool wantAttack = ai.WantsToAttack();
 */

#pragma once

#include "math/Math.h"
#include <string>

namespace MiniEngine {

using Math::Vec2;

/**
 * AI状态枚举
 */
enum class AIState {
    Idle,       // 空闲（巡逻端点停留）
    Patrol,     // 巡逻（来回走动）
    Chase,      // 追击（发现玩家后跑向玩家）
    Attack,     // 攻击（进入攻击范围后发起攻击）
    Hurt,       // 受击（硬直中，不能行动）
    Dead        // 死亡
};

/**
 * 敌人类型 — 决定行为细节
 */
enum class EnemyType {
    Melee,      // 近战型（robot4）：追上去打
    Ranged,     // 远程型（robot8）：保持距离射击
    Hybrid      // 混合型（robot6）：远距离射击，近距离切换近战
};

/**
 * AI配置参数
 */
struct AIConfig {
    EnemyType type = EnemyType::Melee;
    
    // 感知
    float detectionRange = 250.0f;   // 发现玩家的距离
    float loseRange = 350.0f;        // 丢失玩家的距离（大于发现距离，防止抖动）
    float attackRange = 70.0f;       // 发起攻击的距离
    
    // 巡逻
    float patrolMinX = 0.0f;         // 巡逻范围左端
    float patrolMaxX = 300.0f;       // 巡逻范围右端
    float idleDuration = 1.5f;       // 巡逻端点停留时间
    
    // 移动速度
    float patrolSpeed = 80.0f;       // 巡逻走路速度
    float chaseSpeed = 180.0f;       // 追击跑步速度
    
    // 攻击
    float attackCooldown = 1.0f;     // 攻击冷却时间
    float attackDuration = 0.3f;     // 攻击持续时间
    
    // 远程型特有
    float preferredDistance = 180.0f; // 远程型偏好的战斗距离
    float retreatDistance = 80.0f;    // 太近了就后退的距离
    
    // 混合型特有
    float meleeRange = 80.0f;        // 混合型切换到近战的距离
    bool lastAttackWasMelee = false;  // 上次攻击是近战还是远程
};

/**
 * 敌人AI组件
 */
class EnemyAI {
public:
    EnemyAI();
    ~EnemyAI() = default;
    
    // ========================================================================
    // 配置
    // ========================================================================
    
    void SetConfig(const AIConfig& config) { m_config = config; }
    AIConfig& GetConfig() { return m_config; }
    const AIConfig& GetConfig() const { return m_config; }
    
    void SetPatrolRange(float minX, float maxX) {
        m_config.patrolMinX = minX;
        m_config.patrolMaxX = maxX;
    }
    
    void SetDetectionRange(float range) { m_config.detectionRange = range; }
    void SetAttackRange(float range) { m_config.attackRange = range; }
    
    // ========================================================================
    // 更新（每帧调用）
    // ========================================================================
    
    /**
     * 更新AI逻辑
     * @param dt 帧时间
     * @param myPos 自己的位置
     * @param playerPos 玩家的位置
     */
    void Update(float dt, const Vec2& myPos, const Vec2& playerPos);
    
    // ========================================================================
    // 查询AI决策结果
    // ========================================================================
    
    /** 当前状态 */
    AIState GetState() const { return m_state; }
    std::string GetStateName() const;
    
    /** 获取移动方向：-1=向左, 0=不动, 1=向右 */
    float GetMoveDirection() const { return m_moveDirection; }
    
    /** 获取当前应该使用的移动速度 */
    float GetMoveSpeed() const;
    
    /** AI是否想要攻击（刚进入攻击状态的那一帧） */
    bool WantsToAttack() const { return m_wantAttack; }
    
    /** 本次攻击是否是远程攻击（混合型用） */
    bool IsRangedAttack() const { return m_rangedAttack; }
    
    /** 是否面朝右 */
    bool IsFacingRight() const { return m_facingRight; }
    
    /** 是否在追击状态（用于选择走/跑动画） */
    bool IsChasing() const { return m_state == AIState::Chase; }
    
    // ========================================================================
    // 外部事件通知
    // ========================================================================
    
    /** 通知AI被击中 */
    void OnHurt(float stunDuration = 0.3f);
    
    /** 通知AI死亡 */
    void OnDeath();
    
    /** 重置AI状态 */
    void Reset();
    
private:
    AIConfig m_config;
    AIState m_state = AIState::Patrol;
    
    // 移动决策
    float m_moveDirection = 1.0f;   // -1=左, 0=停, 1=右
    bool m_facingRight = true;
    bool m_wantAttack = false;
    bool m_rangedAttack = false;    // 本次攻击是否远程
    
    // 巡逻
    bool m_patrolGoingRight = true;
    
    // 计时器
    float m_idleTimer = 0;
    float m_attackCooldownTimer = 0;
    float m_attackTimer = 0;
    float m_hurtTimer = 0;
    
    // 状态处理
    void UpdateIdle(float dt, const Vec2& myPos, const Vec2& playerPos);
    void UpdatePatrol(float dt, const Vec2& myPos, const Vec2& playerPos);
    void UpdateChase(float dt, const Vec2& myPos, const Vec2& playerPos);
    void UpdateAttack(float dt, const Vec2& myPos, const Vec2& playerPos);
    void UpdateHurt(float dt);
    
    // 辅助
    float DistanceTo(const Vec2& myPos, const Vec2& targetPos) const;
    bool CanSeePlayer(const Vec2& myPos, const Vec2& playerPos) const;
    void FaceToward(const Vec2& myPos, const Vec2& targetPos);
    void TransitionTo(AIState newState);
};

} // namespace MiniEngine
