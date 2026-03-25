#pragma once

#include "math/Math.h"

namespace MiniEngine {

using Math::Vec2;

/**
 * 伤害类型
 */
enum class DamageType {
    Physical,   // 物理伤害
    Fire,       // 火焰伤害
    Ice,        // 冰冻伤害
    Electric,   // 电击伤害
    Poison      // 毒素伤害
};

/**
 * 伤害信息
 */
struct DamageInfo {
    int amount = 0;                          // 伤害数值
    DamageType type = DamageType::Physical;  // 伤害类型
    Vec2 knockback = Vec2(0, 0);             // 击退方向和力度
    float stunTime = 0.0f;                   // 硬直时间
    bool critical = false;                   // 是否暴击
    void* attacker = nullptr;                // 攻击者指针
    
    DamageInfo() = default;
    
    DamageInfo(int dmg, DamageType t = DamageType::Physical)
        : amount(dmg), type(t) {}
    
    DamageInfo& SetKnockback(const Vec2& kb) {
        knockback = kb;
        return *this;
    }
    
    DamageInfo& SetStunTime(float time) {
        stunTime = time;
        return *this;
    }
    
    DamageInfo& SetCritical(bool crit) {
        critical = crit;
        return *this;
    }
    
    DamageInfo& SetAttacker(void* atk) {
        attacker = atk;
        return *this;
    }
};

/**
 * 战斗统计
 */
struct CombatStats {
    int maxHealth = 100;      // 最大生命值
    int currentHealth = 100;  // 当前生命值
    int attack = 10;          // 攻击力
    int defense = 5;          // 防御力
    float critRate = 0.1f;    // 暴击率 (0.0 - 1.0)
    float critMultiplier = 1.5f; // 暴击倍率
    
    bool IsAlive() const { return currentHealth > 0; }
    
    float HealthPercent() const {
        return maxHealth > 0 ? (float)currentHealth / maxHealth : 0;
    }
    
    void TakeDamage(int amount) {
        currentHealth -= amount;
        if (currentHealth < 0) currentHealth = 0;
    }
    
    void Heal(int amount) {
        currentHealth += amount;
        if (currentHealth > maxHealth) currentHealth = maxHealth;
    }
    
    void FullHeal() {
        currentHealth = maxHealth;
    }
};

} // namespace MiniEngine
