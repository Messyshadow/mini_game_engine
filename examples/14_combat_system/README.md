# 第14章：战斗系统

## 本章目标
实现银河恶魔城风格的近战战斗系统，包括：
- 攻击动画状态机
- 攻击碰撞判定（Hitbox）
- 伤害计算系统
- 受击反馈（闪烁、击退）
- 攻击音效

## 核心概念

### 1. 攻击状态机
```
Idle <---> Walk/Run
  |
  v
Attack (锁定状态，动画播放完才能行动)
  |
  v
Recovery (短暂恢复期)
  |
  v
Idle
```

### 2. Hitbox系统
- **Hurtbox**: 角色受击判定区域（始终存在）
- **Hitbox**: 攻击判定区域（仅攻击帧激活）

### 3. 伤害计算
```cpp
int finalDamage = baseDamage * attackPower - defense;
finalDamage = max(1, finalDamage); // 最少1点伤害
```

## 操作说明
| 按键 | 功能 |
|------|------|
| A/D | 左右移动 |
| Space | 跳跃 |
| J | 普通攻击 |
| Tab | 显示/隐藏碰撞框 |
| R | 重置假人 |

## 新增文件
- `source/combat/CombatSystem.h` - 战斗系统核心
- `source/combat/Hitbox.h` - 攻击/受击框
- `source/combat/DamageInfo.h` - 伤害信息结构
- `source/combat/CombatComponent.h` - 战斗组件

## 素材使用
- robot3_punch.png: 8列×4行=32帧攻击动画
- Sound_Axe.wav: 攻击音效
