#pragma once

#include "engine/Shader.h"
#include "math/Math.h"
#include <glad/gl.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <memory>

namespace MiniEngine {

using Math::Vec3;
using Math::Vec4;
using Math::Mat4;

struct Particle3D {
    Vec3 position;
    Vec3 velocity;
    Vec4 color;
    float size;
    float life, maxLife;
    bool active = false;
};

class ParticleEmitter3D {
public:
    Vec3 position;
    Vec3 direction = Vec3(0, 1, 0);
    float spread = 0.5f;
    float emitRate = 20;
    float particleLife = 1.5f;
    float particleSize = 0.2f;
    Vec4 startColor = Vec4(1, 0.8f, 0.3f, 1);
    Vec4 endColor = Vec4(1, 0.2f, 0, 0);
    float startSpeed = 5.0f;
    float gravity = -2.0f;
    int maxParticles = 200;
    bool enabled = true;
    bool continuous = true;

    void Init() {
        m_particles.resize(maxParticles);
        float quad[] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
             0.5f,  0.5f,
            -0.5f, -0.5f,
             0.5f,  0.5f,
            -0.5f,  0.5f
        };
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    void Update(float dt) {
        for (auto& p : m_particles) {
            if (!p.active) continue;
            p.velocity.y += gravity * dt;
            p.position.x += p.velocity.x * dt;
            p.position.y += p.velocity.y * dt;
            p.position.z += p.velocity.z * dt;
            p.life -= dt;
            if (p.life <= 0) p.active = false;
            else {
                float t = 1.0f - (p.life / p.maxLife);
                p.color = LerpColor(startColor, endColor, t);
                p.size = particleSize * (1.0f - t * 0.5f);
            }
        }
        if (enabled && continuous) {
            m_emitAccum += emitRate * dt;
            while (m_emitAccum >= 1.0f) { EmitOne(position); m_emitAccum -= 1.0f; }
        }
    }

    void Render(Shader* shader, const Mat4& view, const Mat4& proj) {
        if (!shader || m_VAO == 0) return;
        shader->Bind();
        shader->SetMat4("uView", view.m);
        shader->SetMat4("uProjection", proj.m);
        glBindVertexArray(m_VAO);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        for (auto& p : m_particles) {
            if (!p.active) continue;
            shader->SetVec3("uParticlePos", p.position);
            shader->SetFloat("uSize", p.size);
            shader->SetVec4("uColor", p.color);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glBindVertexArray(0);
    }

    void Burst(Vec3 pos, int count) {
        for (int i = 0; i < count; i++) EmitOne(pos);
    }

    int ActiveCount() const {
        int c = 0;
        for (auto& p : m_particles) if (p.active) c++;
        return c;
    }

    void Destroy() {
        if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
        if (m_VBO) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
        m_particles.clear();
    }

private:
    std::vector<Particle3D> m_particles;
    float m_emitAccum = 0;
    unsigned int m_VAO = 0, m_VBO = 0;

    static float RandF() { return (float)rand() / (float)RAND_MAX; }
    static float RandF(float lo, float hi) { return lo + RandF() * (hi - lo); }

    static Vec4 LerpColor(const Vec4& a, const Vec4& b, float t) {
        return Vec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                    a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
    }

    void EmitOne(Vec3 pos) {
        for (auto& p : m_particles) {
            if (p.active) continue;
            p.active = true;
            p.position = pos;
            float theta = RandF(0, 6.2832f);
            float phi = RandF(0, spread);
            float sp = std::sin(phi), cp = std::cos(phi);
            Vec3 rndDir(sp * std::cos(theta), cp, sp * std::sin(theta));
            Vec3 up(0, 1, 0);
            float dLen = std::sqrt(direction.x*direction.x + direction.y*direction.y + direction.z*direction.z);
            if (dLen > 0.001f) {
                Vec3 d = direction * (1.0f / dLen);
                Vec3 right(d.z, 0, -d.x);
                float rLen = std::sqrt(right.x*right.x + right.z*right.z);
                if (rLen > 0.001f) { right = right * (1.0f / rLen); }
                else { right = Vec3(1, 0, 0); }
                up = Vec3(d.y * right.z - d.z * right.y, d.z * right.x - d.x * right.z, d.x * right.y - d.y * right.x);
                rndDir = d * cp + right * (sp * std::cos(theta)) + up * (sp * std::sin(theta));
            }
            float spd = startSpeed * RandF(0.7f, 1.3f);
            p.velocity = rndDir * spd;
            p.life = p.maxLife = particleLife * RandF(0.8f, 1.2f);
            p.size = particleSize * RandF(0.8f, 1.2f);
            p.color = startColor;
            return;
        }
    }
};

} // namespace MiniEngine
