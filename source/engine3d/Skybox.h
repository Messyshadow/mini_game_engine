#pragma once

#include "engine/Shader.h"
#include "math/Math.h"
#include <glad/gl.h>
#include <memory>

namespace MiniEngine {

using Math::Vec3;
using Math::Mat4;

class Skybox {
public:
    Vec3 topColor    = Vec3(0.2f, 0.4f, 0.8f);
    Vec3 bottomColor = Vec3(0.8f, 0.85f, 0.9f);
    Vec3 horizonColor = Vec3(0.9f, 0.9f, 0.95f);

    bool Init() {
        m_shader = std::make_unique<Shader>();
        const char* vPaths[] = {"data/shader/skybox.vs","../data/shader/skybox.vs","../../data/shader/skybox.vs"};
        const char* fPaths[] = {"data/shader/skybox.fs","../data/shader/skybox.fs","../../data/shader/skybox.fs"};
        bool ok = false;
        for (int i = 0; i < 3; i++) { if (m_shader->LoadFromFile(vPaths[i], fPaths[i])) { ok = true; break; } }
        if (!ok) return false;

        float verts[] = {
            -1, 1,-1, -1,-1,-1, -1,-1, 1, -1,-1, 1, -1, 1, 1, -1, 1,-1,
             1, 1, 1,  1,-1, 1,  1,-1,-1,  1,-1,-1,  1, 1,-1,  1, 1, 1,
            -1,-1, 1, -1,-1,-1,  1,-1,-1,  1,-1,-1,  1,-1, 1, -1,-1, 1,
            -1, 1,-1, -1, 1, 1,  1, 1, 1,  1, 1, 1,  1, 1,-1, -1, 1,-1,
            -1,-1, 1,  1,-1, 1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1,-1, 1,
             1,-1,-1, -1,-1,-1, -1, 1,-1, -1, 1,-1,  1, 1,-1,  1,-1,-1
        };
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
        return true;
    }

    void Render(const Mat4& view, const Mat4& proj) {
        if (!m_shader || m_VAO == 0) return;
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        m_shader->Bind();
        m_shader->SetMat4("uView", view.m);
        m_shader->SetMat4("uProjection", proj.m);
        m_shader->SetVec3("uTopColor", topColor);
        m_shader->SetVec3("uBottomColor", bottomColor);
        m_shader->SetVec3("uHorizonColor", horizonColor);
        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

    void SetDayPreset() {
        topColor = Vec3(0.2f, 0.4f, 0.8f);
        bottomColor = Vec3(0.8f, 0.85f, 0.9f);
        horizonColor = Vec3(0.9f, 0.9f, 0.95f);
    }

    void SetDuskPreset() {
        topColor = Vec3(0.1f, 0.1f, 0.3f);
        bottomColor = Vec3(0.9f, 0.4f, 0.2f);
        horizonColor = Vec3(1.0f, 0.6f, 0.3f);
    }

    void SetNightPreset() {
        topColor = Vec3(0.02f, 0.02f, 0.08f);
        bottomColor = Vec3(0.05f, 0.05f, 0.15f);
        horizonColor = Vec3(0.08f, 0.08f, 0.2f);
    }

    void Destroy() {
        if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
        if (m_VBO) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
        m_shader.reset();
    }

private:
    unsigned int m_VAO = 0, m_VBO = 0;
    std::unique_ptr<Shader> m_shader;
};

} // namespace MiniEngine
