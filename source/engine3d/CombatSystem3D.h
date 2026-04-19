#pragma once

#include "math/Math.h"
#include <cmath>
#include <string>
#include <algorithm>

namespace MiniEngine {

using Math::Vec3;

// ============================================================================
// AABB碰撞体（场景物体碰撞）
// ============================================================================

struct AABB3D {
    Vec3 min, max;
    bool Intersects(const AABB3D& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

inline void ResolveCollision(Vec3& playerPos, float playerRadius, const AABB3D& obstacle) {
    Vec3 closest;
    closest.x = std::max(obstacle.min.x, std::min(playerPos.x, obstacle.max.x));
    closest.y = std::max(obstacle.min.y, std::min(playerPos.y, obstacle.max.y));
    closest.z = std::max(obstacle.min.z, std::min(playerPos.z, obstacle.max.z));

    Vec3 diff(playerPos.x - closest.x, playerPos.y - closest.y, playerPos.z - closest.z);
    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    if (dist < playerRadius && dist > 0.001f) {
        Vec3 pushDir(diff.x / dist, diff.y / dist, diff.z / dist);
        playerPos.x = closest.x + pushDir.x * playerRadius;
        playerPos.y = closest.y + pushDir.y * playerRadius;
        playerPos.z = closest.z + pushDir.z * playerRadius;
    }
}

// ============================================================================
// 球体碰撞体
// ============================================================================

struct SphereCollider {
    Vec3 center;
    float radius;
};

struct CapsuleCollider {
    Vec3 top, bottom;
    float radius;
};

// ============================================================================
// 碰撞检测函数
// ============================================================================

inline float PointToSegmentDistance(const Vec3& point, const Vec3& segA, const Vec3& segB) {
    Vec3 ab(segB.x - segA.x, segB.y - segA.y, segB.z - segA.z);
    Vec3 ap(point.x - segA.x, point.y - segA.y, point.z - segA.z);
    float abLenSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;
    if (abLenSq < 0.0001f) {
        return std::sqrt(ap.x * ap.x + ap.y * ap.y + ap.z * ap.z);
    }
    float t = (ap.x * ab.x + ap.y * ab.y + ap.z * ab.z) / abLenSq;
    t = std::max(0.0f, std::min(1.0f, t));
    Vec3 closest(segA.x + ab.x * t, segA.y + ab.y * t, segA.z + ab.z * t);
    Vec3 diff(point.x - closest.x, point.y - closest.y, point.z - closest.z);
    return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
}

inline bool SphereSphereCollision(const SphereCollider& a, const SphereCollider& b) {
    Vec3 d(a.center.x - b.center.x, a.center.y - b.center.y, a.center.z - b.center.z);
    float distSq = d.x * d.x + d.y * d.y + d.z * d.z;
    float rSum = a.radius + b.radius;
    return distSq <= rSum * rSum;
}

inline bool SphereCapsuleCollision(const SphereCollider& sphere, const CapsuleCollider& capsule) {
    float dist = PointToSegmentDistance(sphere.center, capsule.bottom, capsule.top);
    return dist <= (sphere.radius + capsule.radius);
}

// ============================================================================
// 攻击数据
// ============================================================================

struct AttackData {
    float damage = 10;
    float knockback = 5;
    float hitStopDuration = 0.1f;
    SphereCollider hitbox;
};

// ============================================================================
// 可被攻击的实体
// ============================================================================

struct CombatEntity {
    Vec3 position;
    float hp = 100, maxHp = 100;
    SphereCollider hurtbox;
    bool alive = true;
    float hitTimer = 0;
    float invincibleTimer = 0;
    std::string name;
};

// ============================================================================
// Hit Stop效果（全局时间缩放）
// ============================================================================

struct HitStop {
    float timer = 0;
    float duration = 0;
    float timeScale = 0.05f;

    void Trigger(float dur) { timer = dur; duration = dur; }
    float GetTimeScale() const { return timer > 0 ? timeScale : 1.0f; }
    void Update(float realDt) { if (timer > 0) timer -= realDt; }
};

} // namespace MiniEngine
