/**
 * EnemyAI.cpp - 敌人AI实现
 */

#include "EnemyAI.h"
#include <cmath>

namespace MiniEngine {

EnemyAI::EnemyAI() {}

// ============================================================================
// 主更新
// ============================================================================

void EnemyAI::Update(float dt, const Vec2& myPos, const Vec2& playerPos) {
    m_wantAttack = false;  // 每帧重置
    
    // 冷却计时
    if (m_attackCooldownTimer > 0) m_attackCooldownTimer -= dt;
    
    switch (m_state) {
        case AIState::Idle:    UpdateIdle(dt, myPos, playerPos);   break;
        case AIState::Patrol:  UpdatePatrol(dt, myPos, playerPos); break;
        case AIState::Chase:   UpdateChase(dt, myPos, playerPos);  break;
        case AIState::Attack:  UpdateAttack(dt, myPos, playerPos); break;
        case AIState::Hurt:    UpdateHurt(dt);                     break;
        case AIState::Dead:    m_moveDirection = 0;                break;
    }
}

// ============================================================================
// 各状态逻辑
// ============================================================================

void EnemyAI::UpdateIdle(float dt, const Vec2& myPos, const Vec2& playerPos) {
    m_moveDirection = 0;
    
    // 随时检测玩家
    if (CanSeePlayer(myPos, playerPos)) {
        TransitionTo(AIState::Chase);
        return;
    }
    
    // 停留计时
    m_idleTimer -= dt;
    if (m_idleTimer <= 0) {
        TransitionTo(AIState::Patrol);
    }
}

void EnemyAI::UpdatePatrol(float dt, const Vec2& myPos, const Vec2& playerPos) {
    // 随时检测玩家
    if (CanSeePlayer(myPos, playerPos)) {
        TransitionTo(AIState::Chase);
        return;
    }
    
    // 巡逻来回走
    if (m_patrolGoingRight) {
        m_moveDirection = 1.0f;
        m_facingRight = true;
        if (myPos.x >= m_config.patrolMaxX) {
            m_patrolGoingRight = false;
            m_idleTimer = m_config.idleDuration;
            TransitionTo(AIState::Idle);
        }
    } else {
        m_moveDirection = -1.0f;
        m_facingRight = false;
        if (myPos.x <= m_config.patrolMinX) {
            m_patrolGoingRight = true;
            m_idleTimer = m_config.idleDuration;
            TransitionTo(AIState::Idle);
        }
    }
}

void EnemyAI::UpdateChase(float dt, const Vec2& myPos, const Vec2& playerPos) {
    float dist = DistanceTo(myPos, playerPos);
    
    // 丢失目标 → 回去巡逻
    if (dist > m_config.loseRange) {
        TransitionTo(AIState::Patrol);
        return;
    }
    
    FaceToward(myPos, playerPos);
    
    if (m_config.type == EnemyType::Melee) {
        // 近战型：冲上去
        if (dist <= m_config.attackRange && m_attackCooldownTimer <= 0) {
            m_rangedAttack = false;
            TransitionTo(AIState::Attack);
            return;
        }
        m_moveDirection = (playerPos.x > myPos.x) ? 1.0f : -1.0f;
    } else if (m_config.type == EnemyType::Hybrid) {
        // 混合型：近距离用近战，远距离用远程
        if (dist <= m_config.meleeRange && m_attackCooldownTimer <= 0) {
            // 近距离 → 近战攻击
            m_rangedAttack = false;
            TransitionTo(AIState::Attack);
            return;
        } else if (dist <= m_config.attackRange && dist > m_config.meleeRange && m_attackCooldownTimer <= 0) {
            // 中距离 → 远程攻击
            m_rangedAttack = true;
            m_moveDirection = 0;
            TransitionTo(AIState::Attack);
            return;
        }
        // 向玩家移动
        m_moveDirection = (playerPos.x > myPos.x) ? 1.0f : -1.0f;
    } else {
        // 远程型：保持距离
        if (dist < m_config.retreatDistance) {
            m_moveDirection = (playerPos.x > myPos.x) ? -1.0f : 1.0f;
        } else if (dist > m_config.preferredDistance) {
            m_moveDirection = (playerPos.x > myPos.x) ? 1.0f : -1.0f;
        } else {
            m_moveDirection = 0;
            if (m_attackCooldownTimer <= 0) {
                m_rangedAttack = true;
                TransitionTo(AIState::Attack);
                return;
            }
        }
    }
}

void EnemyAI::UpdateAttack(float dt, const Vec2& myPos, const Vec2& playerPos) {
    m_moveDirection = 0;
    m_attackTimer -= dt;
    
    if (m_attackTimer <= 0) {
        m_attackCooldownTimer = m_config.attackCooldown;
        // 攻击结束后看看玩家还在不在
        float dist = DistanceTo(myPos, playerPos);
        if (dist <= m_config.loseRange) {
            TransitionTo(AIState::Chase);
        } else {
            TransitionTo(AIState::Patrol);
        }
    }
}

void EnemyAI::UpdateHurt(float dt) {
    m_moveDirection = 0;
    m_hurtTimer -= dt;
    
    if (m_hurtTimer <= 0) {
        TransitionTo(AIState::Chase);  // 被打了就进入追击
    }
}

// ============================================================================
// 外部事件
// ============================================================================

void EnemyAI::OnHurt(float stunDuration) {
    if (m_state == AIState::Dead) return;
    m_hurtTimer = stunDuration;
    TransitionTo(AIState::Hurt);
}

void EnemyAI::OnDeath() {
    TransitionTo(AIState::Dead);
    m_moveDirection = 0;
}

void EnemyAI::Reset() {
    m_state = AIState::Patrol;
    m_moveDirection = 1.0f;
    m_facingRight = true;
    m_patrolGoingRight = true;
    m_idleTimer = 0;
    m_attackCooldownTimer = 0;
    m_attackTimer = 0;
    m_hurtTimer = 0;
    m_wantAttack = false;
}

// ============================================================================
// 辅助函数
// ============================================================================

float EnemyAI::DistanceTo(const Vec2& myPos, const Vec2& targetPos) const {
    float dx = targetPos.x - myPos.x;
    float dy = targetPos.y - myPos.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool EnemyAI::CanSeePlayer(const Vec2& myPos, const Vec2& playerPos) const {
    return DistanceTo(myPos, playerPos) <= m_config.detectionRange;
}

void EnemyAI::FaceToward(const Vec2& myPos, const Vec2& targetPos) {
    m_facingRight = targetPos.x > myPos.x;
}

void EnemyAI::TransitionTo(AIState newState) {
    if (m_state == newState) return;
    
    // 进入新状态的初始化
    switch (newState) {
        case AIState::Attack:
            m_attackTimer = m_config.attackDuration;
            m_wantAttack = true;
            m_moveDirection = 0;
            break;
        case AIState::Hurt:
            m_moveDirection = 0;
            break;
        default:
            break;
    }
    
    m_state = newState;
}

float EnemyAI::GetMoveSpeed() const {
    switch (m_state) {
        case AIState::Patrol: return m_config.patrolSpeed;
        case AIState::Chase:  return m_config.chaseSpeed;
        default:              return 0;
    }
}

std::string EnemyAI::GetStateName() const {
    switch (m_state) {
        case AIState::Idle:    return "Idle";
        case AIState::Patrol:  return "Patrol";
        case AIState::Chase:   return "Chase";
        case AIState::Attack:  return "Attack";
        case AIState::Hurt:    return "Hurt";
        case AIState::Dead:    return "Dead";
        default:               return "Unknown";
    }
}

} // namespace MiniEngine
