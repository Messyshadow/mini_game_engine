#pragma once

#include "math/Math.h"
#include <cmath>
#include <string>

namespace MiniEngine {

using Math::Vec3;

enum class AIState { Idle, Patrol, Chase, Attack, Retreat, Dead };

struct EnemyAI3D {
    std::string name;
    AIState state = AIState::Patrol;
    Vec3 position;
    float hp = 100, maxHp = 100;
    float detectionRange = 10.0f;
    float loseRange = 15.0f;
    float attackRange = 2.0f;
    float moveSpeed = 3.0f;
    float attackCooldown = 1.5f;
    float attackTimer = 0;
    float attackDamage = 10.0f;

    Vec3 patrolPointA, patrolPointB;
    bool patrolToB = true;

    float fovAngle = 120.0f;
    Vec3 forward = Vec3(0, 0, 1);

    float hitTimer = 0;
    float invincibleTimer = 0;
    float hitStunTimer = 0;
    float hurtRadius = 0.8f;

    bool CanSeePlayer(Vec3 playerPos) const {
        float dx = playerPos.x - position.x;
        float dz = playerPos.z - position.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist > detectionRange) return false;
        if (dist < 0.001f) return true;

        float dirX = dx / dist, dirZ = dz / dist;
        float fLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
        if (fLen < 0.001f) return true;
        float fX = forward.x / fLen, fZ = forward.z / fLen;

        float dot = fX * dirX + fZ * dirZ;
        float halfFov = fovAngle * 0.5f * 3.14159f / 180.0f;
        return dot >= std::cos(halfFov);
    }

    bool InAttackRange(Vec3 playerPos) const {
        float dx = playerPos.x - position.x;
        float dz = playerPos.z - position.z;
        return (dx * dx + dz * dz) <= attackRange * attackRange;
    }

    float DistanceTo(Vec3 target) const {
        float dx = target.x - position.x;
        float dz = target.z - position.z;
        return std::sqrt(dx * dx + dz * dz);
    }

    void MoveToward(Vec3 target, float dt) {
        float dx = target.x - position.x;
        float dz = target.z - position.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist < 0.01f) return;
        float nx = dx / dist, nz = dz / dist;
        float step = moveSpeed * dt;
        if (step > dist) step = dist;
        position.x += nx * step;
        position.z += nz * step;
        forward = Vec3(nx, 0, nz);
    }

    void ReanchorPatrol() {
        Vec3 mid((patrolPointA.x + patrolPointB.x) * 0.5f, position.y,
                 (patrolPointA.z + patrolPointB.z) * 0.5f);
        Vec3 offset = position - mid;
        patrolPointA = patrolPointA + offset;
        patrolPointB = patrolPointB + offset;
    }

    void Update(float dt, Vec3 playerPos) {
        if (hitTimer > 0) hitTimer -= dt;
        if (invincibleTimer > 0) invincibleTimer -= dt;
        if (hitStunTimer > 0) hitStunTimer -= dt;

        if (state == AIState::Dead) return;
        if (hp <= 0) { state = AIState::Dead; return; }
        if (hitStunTimer > 0) return;
        if (attackTimer > 0) attackTimer -= dt;

        switch (state) {
        case AIState::Patrol: {
            Vec3 target = patrolToB ? patrolPointB : patrolPointA;
            MoveToward(target, dt);
            if (DistanceTo(target) < 0.3f) patrolToB = !patrolToB;
            if (CanSeePlayer(playerPos)) state = AIState::Chase;
            break;
        }
        case AIState::Chase: {
            MoveToward(playerPos, dt);
            if (InAttackRange(playerPos)) {
                state = AIState::Attack;
                attackTimer = 0;
            }
            if (DistanceTo(playerPos) > loseRange) {
                state = AIState::Patrol;
            }
            break;
        }
        case AIState::Attack: {
            float dx = playerPos.x - position.x;
            float dz = playerPos.z - position.z;
            float fl = std::sqrt(dx * dx + dz * dz);
            if (fl > 0.01f) forward = Vec3(dx / fl, 0, dz / fl);

            if (!InAttackRange(playerPos)) {
                state = AIState::Chase;
                break;
            }
            if (attackTimer <= 0) {
                attackTimer = attackCooldown;
            }
            break;
        }
        case AIState::Retreat: {
            Vec3 mid((patrolPointA.x + patrolPointB.x) * 0.5f, position.y,
                     (patrolPointA.z + patrolPointB.z) * 0.5f);
            MoveToward(mid, dt);
            if (DistanceTo(mid) < 1.0f) state = AIState::Patrol;
            break;
        }
        default: break;
        }
    }

    bool IsAttacking() const {
        return state == AIState::Attack && attackTimer >= (attackCooldown - 0.15f) && attackTimer > 0;
    }

    const char* StateName() const {
        switch (state) {
            case AIState::Idle:    return "Idle";
            case AIState::Patrol:  return "Patrol";
            case AIState::Chase:   return "Chase";
            case AIState::Attack:  return "Attack";
            case AIState::Retreat: return "Retreat";
            case AIState::Dead:    return "Dead";
            default: return "?";
        }
    }
};

} // namespace MiniEngine
