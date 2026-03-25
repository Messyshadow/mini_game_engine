/**
 * ParticleSystem.h - 简易2D粒子系统
 *
 * 用于：攻击命中火花、瞬移残影、BOSS技能特效、物品拾取光效
 */

#pragma once

#include "math/Math.h"
#include "engine/SpriteRenderer.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace MiniEngine {

using Math::Vec2;
using Math::Vec4;

struct Particle {
    Vec2 pos;
    Vec2 vel;
    Vec4 colorStart, colorEnd;
    float life, maxLife;
    float size, sizeEnd;
    float rotation, rotSpeed;
};

class ParticleSystem {
public:
    void Emit(Vec2 pos, int count, Vec4 color1, Vec4 color2,
              float speedMin, float speedMax, float lifeMin, float lifeMax,
              float sizeStart = 6, float sizeEnd = 1) {
        for (int i = 0; i < count; i++) {
            Particle p;
            p.pos = pos;
            float angle = RandF(0, 6.2832f);
            float speed = RandF(speedMin, speedMax);
            p.vel = Vec2(std::cos(angle) * speed, std::sin(angle) * speed);
            p.colorStart = color1; p.colorEnd = color2;
            p.maxLife = p.life = RandF(lifeMin, lifeMax);
            p.size = sizeStart; p.sizeEnd = sizeEnd;
            p.rotation = RandF(0, 3.14f); p.rotSpeed = RandF(-3, 3);
            m_particles.push_back(p);
        }
    }
    
    // 预设特效
    void EmitHitSpark(Vec2 pos) {
        Emit(pos, 8, Vec4(1,0.9f,0.3f,1), Vec4(1,0.3f,0,0), 80, 200, 0.15f, 0.35f, 5, 1);
    }
    void EmitTeleport(Vec2 pos) {
        Emit(pos, 15, Vec4(0.3f,0.7f,1,0.8f), Vec4(0.1f,0.3f,1,0), 40, 120, 0.3f, 0.6f, 8, 2);
    }
    void EmitDeath(Vec2 pos) {
        Emit(pos, 20, Vec4(1,0.5f,0,1), Vec4(0.5f,0,0,0), 60, 180, 0.4f, 0.8f, 6, 1);
    }
    void EmitPickup(Vec2 pos) {
        Emit(pos, 12, Vec4(1,1,0.5f,1), Vec4(0.5f,1,0.5f,0), 50, 140, 0.3f, 0.5f, 7, 2);
    }
    void EmitBossAttack(Vec2 pos) {
        Emit(pos, 25, Vec4(1,0.2f,0.1f,1), Vec4(1,0.6f,0,0), 100, 300, 0.2f, 0.5f, 10, 2);
    }
    
    void Update(float dt) {
        for (auto& p : m_particles) {
            p.life -= dt;
            p.pos = p.pos + p.vel * dt;
            p.vel.y -= 200 * dt; // 轻微重力
            p.vel = p.vel * 0.98f; // 空气阻力
            p.rotation += p.rotSpeed * dt;
        }
        m_particles.erase(
            std::remove_if(m_particles.begin(), m_particles.end(),
                [](const Particle& p) { return p.life <= 0; }),
            m_particles.end());
    }
    
    void Render(SpriteRenderer* renderer) {
        for (auto& p : m_particles) {
            float t = 1.0f - (p.life / p.maxLife); // 0→1
            Vec4 col;
            col.x = p.colorStart.x + (p.colorEnd.x - p.colorStart.x) * t;
            col.y = p.colorStart.y + (p.colorEnd.y - p.colorStart.y) * t;
            col.z = p.colorStart.z + (p.colorEnd.z - p.colorStart.z) * t;
            col.w = p.colorStart.w + (p.colorEnd.w - p.colorStart.w) * t;
            float sz = p.size + (p.sizeEnd - p.size) * t;
            renderer->DrawRect(p.pos, Vec2(sz, sz), col, p.rotation);
        }
    }
    
    void Clear() { m_particles.clear(); }
    int Count() const { return (int)m_particles.size(); }
    
private:
    std::vector<Particle> m_particles;
    float RandF(float mn, float mx) {
        return mn + (float)rand() / RAND_MAX * (mx - mn);
    }
};

} // namespace MiniEngine
