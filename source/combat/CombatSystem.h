#pragma once

/**
 * Combat System - 战斗系统
 * 
 * 包含所有战斗相关的头文件
 */

#include "DamageInfo.h"
#include "Hitbox.h"
#include "CombatComponent.h"

namespace MiniEngine {

/**
 * 碰撞层定义
 */
const int LAYER_PLAYER = 1 << 0;
const int LAYER_ENEMY = 1 << 1;
const int LAYER_PLAYER_ATTACK = 1 << 2;
const int LAYER_ENEMY_ATTACK = 1 << 3;
const int LAYER_ENVIRONMENT = 1 << 4;

/**
 * 检测两个碰撞层是否应该碰撞
 */
inline bool ShouldCollide(int layerA, int layerB) {
    // 玩家攻击 vs 敌人
    if ((layerA & LAYER_PLAYER_ATTACK) && (layerB & LAYER_ENEMY)) return true;
    if ((layerA & LAYER_ENEMY) && (layerB & LAYER_PLAYER_ATTACK)) return true;
    
    // 敌人攻击 vs 玩家
    if ((layerA & LAYER_ENEMY_ATTACK) && (layerB & LAYER_PLAYER)) return true;
    if ((layerA & LAYER_PLAYER) && (layerB & LAYER_ENEMY_ATTACK)) return true;
    
    return false;
}

} // namespace MiniEngine
