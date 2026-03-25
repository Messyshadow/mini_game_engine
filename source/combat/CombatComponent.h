#pragma once

#include "DamageInfo.h"
#include "Hitbox.h"
#include "math/Math.h"
#include <functional>
#include <string>

namespace MiniEngine {

using Math::Vec2;

/**
 * 战斗状态
 */
enum class CombatState {
    Idle,       // 空闲（可以行动）
    Attacking,  // 攻击中（锁定输入）
    Recovery,   // 攻击后恢复
    Stunned,    // 硬直/被击中
    Dead        // 死亡
};

/**
 * 攻击定义
 */
struct AttackData {
    std::string name;           // 攻击名称
    int baseDamage = 10;        // 基础伤害
    float startupFrames = 0.05f;    // 起手时间（秒）
    float activeFrames = 0.1f;      // 攻击判定时间（秒）
    float recoveryFrames = 0.15f;   // 收招时间（秒）
    Vec2 knockback = Vec2(100, 50); // 击退力度
    float stunTime = 0.2f;          // 造成的硬直时间
    
    // 动画相关
    int animStartFrame = 0;     // 动画起始帧
    int animFrameCount = 8;     // 动画帧数
    float animFPS = 15.0f;      // 动画播放速度
    
    // Hitbox相关
    Vec2 hitboxOffset = Vec2(50, 0);  // 攻击框偏移
    Vec2 hitboxSize = Vec2(60, 80);   // 攻击框大小
    
    float TotalDuration() const {
        return startupFrames + activeFrames + recoveryFrames;
    }
};

/**
 * 战斗组件 - 附加到可战斗的实体上
 */
class CombatComponent {
public:
    // 基础属性
    CombatStats stats;
    HitboxSet hitboxes;
    
    // 状态
    CombatState state = CombatState::Idle;
    float stateTimer = 0;
    
    // 当前攻击
    AttackData* currentAttack = nullptr;
    float attackTimer = 0;
    bool hitConnected = false;  // 本次攻击是否已命中
    
    // 受击相关
    float invincibleTimer = 0;  // 无敌时间
    float flashTimer = 0;       // 闪烁计时
    bool isFlashing = false;    // 是否正在闪烁
    
    // 回调函数
    std::function<void(const DamageInfo&)> onTakeDamage;
    std::function<void(void* target, const DamageInfo&)> onDealDamage;
    std::function<void()> onDeath;
    std::function<void(const std::string&)> onAttackStart;
    std::function<void()> onAttackEnd;
    
    CombatComponent() {
        // 默认hurtbox
        hitboxes.AddHurtbox(Vec2(0, 0), Vec2(40, 80));
    }
    
    /**
     * 更新战斗组件
     */
    void Update(float dt) {
        // 更新无敌时间
        if (invincibleTimer > 0) {
            invincibleTimer -= dt;
            
            // 闪烁效果
            flashTimer += dt;
            if (flashTimer >= 0.05f) {
                flashTimer = 0;
                isFlashing = !isFlashing;
            }
        } else {
            isFlashing = false;
        }
        
        // 更新状态
        switch (state) {
            case CombatState::Attacking:
                UpdateAttacking(dt);
                break;
                
            case CombatState::Recovery:
                stateTimer -= dt;
                if (stateTimer <= 0) {
                    EndAttack();
                }
                break;
                
            case CombatState::Stunned:
                stateTimer -= dt;
                if (stateTimer <= 0) {
                    state = CombatState::Idle;
                }
                break;
                
            default:
                break;
        }
    }
    
    /**
     * 开始攻击
     */
    bool StartAttack(AttackData& attack) {
        if (state != CombatState::Idle && state != CombatState::Recovery) {
            return false;
        }
        
        currentAttack = &attack;
        attackTimer = 0;
        hitConnected = false;
        state = CombatState::Attacking;
        
        // 设置攻击hitbox
        hitboxes.boxes.clear();
        hitboxes.AddHurtbox(Vec2(0, 0), Vec2(40, 80));  // 重新添加hurtbox
        hitboxes.AddHitbox(attack.hitboxOffset, attack.hitboxSize);
        hitboxes.ActivateHitboxes(false);  // 起手阶段不激活
        
        if (onAttackStart) {
            onAttackStart(attack.name);
        }
        
        return true;
    }
    
    /**
     * 是否可以行动
     */
    bool CanAct() const {
        return state == CombatState::Idle;
    }
    
    /**
     * 是否正在攻击（hitbox激活）
     */
    bool IsAttackActive() const {
        if (state != CombatState::Attacking || !currentAttack) return false;
        return attackTimer >= currentAttack->startupFrames && 
               attackTimer < currentAttack->startupFrames + currentAttack->activeFrames;
    }
    
    /**
     * 受到伤害
     */
    void TakeDamage(const DamageInfo& info) {
        if (invincibleTimer > 0 || !stats.IsAlive()) return;
        
        // 计算实际伤害
        int finalDamage = info.amount - stats.defense;
        if (finalDamage < 1) finalDamage = 1;
        if (info.critical) finalDamage = (int)(finalDamage * 1.5f);
        
        stats.TakeDamage(finalDamage);
        
        // 设置无敌时间和硬直
        invincibleTimer = 0.5f;
        if (info.stunTime > 0) {
            state = CombatState::Stunned;
            stateTimer = info.stunTime;
        }
        
        // 回调
        if (onTakeDamage) {
            DamageInfo modifiedInfo = info;
            modifiedInfo.amount = finalDamage;
            onTakeDamage(modifiedInfo);
        }
        
        // 检查死亡
        if (!stats.IsAlive()) {
            state = CombatState::Dead;
            if (onDeath) {
                onDeath();
            }
        }
    }
    
    /**
     * 是否无敌
     */
    bool IsInvincible() const {
        return invincibleTimer > 0;
    }
    
    /**
     * 获取渲染时的alpha值（用于闪烁效果）
     */
    float GetRenderAlpha() const {
        if (isFlashing) return 0.3f;
        return 1.0f;
    }
    
private:
    void UpdateAttacking(float dt) {
        if (!currentAttack) {
            EndAttack();
            return;
        }
        
        attackTimer += dt;
        
        // 起手阶段
        if (attackTimer < currentAttack->startupFrames) {
            hitboxes.ActivateHitboxes(false);
        }
        // 攻击判定阶段
        else if (attackTimer < currentAttack->startupFrames + currentAttack->activeFrames) {
            hitboxes.ActivateHitboxes(true);
        }
        // 收招阶段
        else if (attackTimer < currentAttack->TotalDuration()) {
            hitboxes.ActivateHitboxes(false);
            state = CombatState::Recovery;
            stateTimer = currentAttack->recoveryFrames;
        }
        // 攻击结束
        else {
            EndAttack();
        }
    }
    
    void EndAttack() {
        hitboxes.ActivateHitboxes(false);
        currentAttack = nullptr;
        state = CombatState::Idle;
        
        if (onAttackEnd) {
            onAttackEnd();
        }
    }
};

} // namespace MiniEngine
