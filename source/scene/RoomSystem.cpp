/**
 * RoomSystem.cpp - 房间与关卡系统实现
 */

#include "RoomSystem.h"
#include <cmath>
#include <iostream>

namespace MiniEngine {

// ============================================================================
// 房间注册
// ============================================================================

void RoomManager::AddRoom(const Room& room) {
    m_rooms.push_back(room);
}

Room* RoomManager::GetRoom(const std::string& name) {
    for (auto& r : m_rooms) {
        if (r.name == name) return &r;
    }
    return nullptr;
}

// ============================================================================
// 房间切换
// ============================================================================

void RoomManager::LoadRoom(const std::string& name, const Vec2& spawnPos) {
    Room* room = GetRoom(name);
    if (!room) {
        std::cerr << "[RoomManager] Room not found: " << name << std::endl;
        return;
    }
    
    m_currentRoom = room;
    
    // 重置所有触发器状态
    for (auto& t : room->triggers) {
        t.triggered = false;
    }
    
    std::cout << "[RoomManager] Loaded room: " << room->displayName 
              << " (" << room->name << ")" << std::endl;
    
    if (onRoomLoaded) {
        onRoomLoaded(room);
    }
}

void RoomManager::TransitionToRoom(const std::string& name, const Vec2& spawnPos) {
    if (m_transState != TransitionState::None) return;  // 已经在过渡中
    
    m_pendingRoom = name;
    m_pendingSpawn = spawnPos;
    m_transState = TransitionState::FadeOut;
    m_transAlpha = 0;
    
    std::cout << "[RoomManager] Transition to: " << name << std::endl;
}

void RoomManager::UpdateTransition(float dt) {
    if (m_transState == TransitionState::None) return;
    
    if (m_transState == TransitionState::FadeOut) {
        // 逐渐变黑
        m_transAlpha += m_transSpeed * dt;
        if (m_transAlpha >= 1.0f) {
            m_transAlpha = 1.0f;
            // 完全黑屏时，执行房间切换
            LoadRoom(m_pendingRoom, m_pendingSpawn);
            m_transState = TransitionState::FadeIn;
        }
    } else if (m_transState == TransitionState::FadeIn) {
        // 逐渐显现
        m_transAlpha -= m_transSpeed * dt;
        if (m_transAlpha <= 0) {
            m_transAlpha = 0;
            m_transState = TransitionState::None;
        }
    }
}

// ============================================================================
// 触发器检测
// ============================================================================

Trigger* RoomManager::CheckTriggers(const Vec2& playerPos, const Vec2& playerSize) {
    if (!m_currentRoom) return nullptr;
    
    Vec2 pHalf = playerSize * 0.5f;
    float pMinX = playerPos.x - pHalf.x, pMaxX = playerPos.x + pHalf.x;
    float pMinY = playerPos.y - pHalf.y, pMaxY = playerPos.y + pHalf.y;
    
    for (auto& t : m_currentRoom->triggers) {
        if (!t.repeatable && t.triggered) continue;
        
        Vec2 tHalf = t.size * 0.5f;
        float tMinX = t.position.x - tHalf.x, tMaxX = t.position.x + tHalf.x;
        float tMinY = t.position.y - tHalf.y, tMaxY = t.position.y + tHalf.y;
        
        // AABB重叠检测
        if (pMinX < tMaxX && pMaxX > tMinX && pMinY < tMaxY && pMaxY > tMinY) {
            // Door类型不自动标记（需要玩家按键确认）
            if (t.type != TriggerType::Door) {
                t.triggered = true;
            }
            return &t;
        }
    }
    return nullptr;
}

} // namespace MiniEngine
