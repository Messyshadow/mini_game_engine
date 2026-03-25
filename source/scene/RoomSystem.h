/**
 * RoomSystem.h - 房间与关卡系统
 *
 * ============================================================================
 * 概述
 * ============================================================================
 *
 * 管理多个游戏房间（关卡），支持：
 * 1. 房间数据定义（地图、敌人、触发器）
 * 2. 房间切换（淡入淡出过渡）
 * 3. 触发器系统（传送门、存档点、事件区域）
 *
 * ============================================================================
 * 房间结构示意
 * ============================================================================
 *
 *   Room A                         Room B
 *   ┌───────────────┐              ┌───────────────┐
 *   │               │   Door       │               │
 *   │   ☺  enemies  │──[Portal]──→ │  enemies  ☺   │
 *   │  ▓▓▓▓▓▓▓▓▓▓▓ │              │ ▓▓▓▓▓▓▓▓▓▓▓  │
 *   └───────────────┘              └───────────────┘
 */

#pragma once

#include "math/Math.h"
#include "ai/EnemyAI.h"
#include <string>
#include <vector>
#include <functional>

namespace MiniEngine {

using Math::Vec2;
using Math::Vec4;

// ============================================================================
// 触发器
// ============================================================================

enum class TriggerType {
    Door,       // 传送门 — 切换到另一个房间
    Checkpoint, // 存档点 — 设置重生位置
    Event       // 事件区域 — 触发自定义逻辑
};

/**
 * 触发器 — 玩家进入区域后触发效果
 */
struct Trigger {
    TriggerType type = TriggerType::Door;
    Vec2 position;              // 触发器中心位置
    Vec2 size = Vec2(40, 64);   // 触发器区域尺寸
    
    // Door类型
    std::string targetRoom;     // 目标房间名称
    Vec2 targetSpawn;           // 传送到目标房间的出生位置
    
    // 显示
    bool visible = true;        // 是否渲染提示
    Vec4 color = Vec4(0.2f, 0.8f, 1.0f, 0.5f); // 提示颜色
    std::string label;          // 显示文字
    
    bool triggered = false;     // 是否已触发（单次触发用）
    bool repeatable = true;     // 是否可重复触发
};

// ============================================================================
// 平台定义（简化版地图构建）
// ============================================================================

struct PlatformDef {
    int x, y, width, height;    // 瓦片坐标
    int tileID = 9;             // 使用哪个瓦片ID
};

// ============================================================================
// 敌人生成点
// ============================================================================

struct EnemySpawn {
    EnemyType type;
    Vec2 position;
    float patrolMinX, patrolMaxX;
    int health = 40;
    int attack = 8;
};

// ============================================================================
// 房间定义
// ============================================================================

/**
 * 房间 — 一个完整的游戏关卡区域
 */
struct Room {
    std::string name;               // 房间唯一名称
    std::string displayName;        // 显示名称（中文）
    int width = 40, height = 15;    // 瓦片尺寸
    int tileSize = 32;
    
    // 背景颜色
    Vec4 bgColor = Vec4(0.12f, 0.15f, 0.25f, 1.0f);
    
    // 地图构建数据
    std::vector<PlatformDef> platforms;
    
    // 敌人生成点
    std::vector<EnemySpawn> enemySpawns;
    
    // 触发器
    std::vector<Trigger> triggers;
    
    // 玩家出生点（首次进入时使用）
    Vec2 defaultSpawn = Vec2(100, 86);
    
    // BGM
    std::string bgmName;
    
    // 便捷构建方法
    void AddGround(int startX, int endX, int groundTile = 9, int dirtTile = 11) {
        platforms.push_back({startX, 0, endX - startX, 1, dirtTile});
        platforms.push_back({startX, 1, endX - startX, 1, groundTile});
    }
    
    void AddPlatform(int x, int y, int w, int tileID = 9) {
        platforms.push_back({x, y, w, 1, tileID});
    }
    
    void AddWalls(int height_tiles = 10, int wallTile = 17) {
        platforms.push_back({0, 2, 1, height_tiles, wallTile});
        platforms.push_back({width - 1, 2, 1, height_tiles, wallTile});
    }
    
    void AddDoor(Vec2 pos, const std::string& target, Vec2 spawn, const std::string& lbl = "") {
        Trigger t;
        t.type = TriggerType::Door;
        t.position = pos;
        t.targetRoom = target;
        t.targetSpawn = spawn;
        t.label = lbl.empty() ? ("-> " + target) : lbl;
        t.color = Vec4(0.2f, 0.8f, 1.0f, 0.5f);
        triggers.push_back(t);
    }
    
    void AddCheckpoint(Vec2 pos) {
        Trigger t;
        t.type = TriggerType::Checkpoint;
        t.position = pos;
        t.color = Vec4(0.2f, 1.0f, 0.4f, 0.5f);
        t.label = "Checkpoint";
        triggers.push_back(t);
    }
    
    void AddEnemy(EnemyType type, Vec2 pos, float patrolMin, float patrolMax,
                  int hp = 40, int atk = 8) {
        enemySpawns.push_back({type, pos, patrolMin, patrolMax, hp, atk});
    }
    
    int GetPixelWidth() const { return width * tileSize; }
    int GetPixelHeight() const { return height * tileSize; }
};

// ============================================================================
// 过渡效果
// ============================================================================

enum class TransitionState {
    None,       // 无过渡
    FadeOut,    // 淡出（当前房间变黑）
    FadeIn      // 淡入（新房间显现）
};

// ============================================================================
// 房间管理器
// ============================================================================

/**
 * RoomManager — 管理所有房间和房间切换
 */
class RoomManager {
public:
    RoomManager() = default;
    
    // ========================================================================
    // 房间注册
    // ========================================================================
    
    /** 添加房间定义 */
    void AddRoom(const Room& room);
    
    /** 获取房间 */
    Room* GetRoom(const std::string& name);
    const Room* GetCurrentRoom() const { return m_currentRoom; }
    
    // ========================================================================
    // 房间切换
    // ========================================================================
    
    /** 立即切换房间（无过渡） */
    void LoadRoom(const std::string& name, const Vec2& spawnPos);
    
    /** 带过渡效果切换房间 */
    void TransitionToRoom(const std::string& name, const Vec2& spawnPos);
    
    /** 更新过渡动画 */
    void UpdateTransition(float dt);
    
    /** 当前是否在过渡中 */
    bool IsTransitioning() const { return m_transState != TransitionState::None; }
    
    /** 获取过渡黑屏的alpha值（0=透明, 1=全黑） */
    float GetTransitionAlpha() const { return m_transAlpha; }
    
    TransitionState GetTransitionState() const { return m_transState; }
    
    // ========================================================================
    // 触发器检测
    // ========================================================================
    
    /** 检测玩家是否触碰到任何触发器 */
    Trigger* CheckTriggers(const Vec2& playerPos, const Vec2& playerSize);
    
    // ========================================================================
    // 回调
    // ========================================================================
    
    /** 房间加载完成回调（用于重建地图、生成敌人等） */
    std::function<void(Room*)> onRoomLoaded;
    
    /** 存档点触发回调 */
    std::function<void(const Vec2&)> onCheckpoint;
    
    // ========================================================================
    // 存档点
    // ========================================================================
    
    Vec2 GetRespawnPos() const { return m_respawnPos; }
    std::string GetRespawnRoom() const { return m_respawnRoom; }
    void SetRespawn(const std::string& room, const Vec2& pos) {
        m_respawnRoom = room; m_respawnPos = pos;
    }
    
private:
    std::vector<Room> m_rooms;
    Room* m_currentRoom = nullptr;
    
    // 过渡
    TransitionState m_transState = TransitionState::None;
    float m_transAlpha = 0;
    float m_transSpeed = 3.0f;  // 淡入淡出速度
    std::string m_pendingRoom;
    Vec2 m_pendingSpawn;
    
    // 存档点
    std::string m_respawnRoom;
    Vec2 m_respawnPos;
};

} // namespace MiniEngine
