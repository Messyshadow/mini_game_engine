#pragma once

#include "engine/Shader.h"
#include "math/Math.h"
#include <glad/gl.h>
#include <memory>
#include <string>
#include <iostream>

#ifndef STB_IMAGE_INCLUDE_GUARD
#define STB_IMAGE_INCLUDE_GUARD
extern "C" {
    unsigned char* stbi_load(const char*, int*, int*, int*, int);
    void stbi_image_free(void*);
    void stbi_set_flip_vertically_on_load(int);
}
#endif

namespace MiniEngine {

using Math::Vec3;
using Math::Mat4;

class Skybox {
public:
    Vec3 topColor    = Vec3(0.2f, 0.4f, 0.8f);
    Vec3 bottomColor = Vec3(0.8f, 0.85f, 0.9f);
    Vec3 horizonColor = Vec3(0.9f, 0.9f, 0.95f);
    bool useTexture = false;

    bool Init() {
        m_procShader = std::make_unique<Shader>();
        const char* vPaths[] = {"data/shader/skybox.vs","../data/shader/skybox.vs","../../data/shader/skybox.vs"};
        const char* fPaths[] = {"data/shader/skybox.fs","../data/shader/skybox.fs","../../data/shader/skybox.fs"};
        bool ok = false;
        for (int i = 0; i < 3; i++) { if (m_procShader->LoadFromFile(vPaths[i], fPaths[i])) { ok = true; break; } }
        if (!ok) return false;

        m_texShader = std::make_unique<Shader>();
        const char* tfPaths[] = {"data/shader/skybox_tex.fs","../data/shader/skybox_tex.fs","../../data/shader/skybox_tex.fs"};
        ok = false;
        for (int i = 0; i < 3; i++) { if (m_texShader->LoadFromFile(vPaths[i], tfPaths[i])) { ok = true; break; } }
        if (ok) {
            m_texShader->Bind();
            m_texShader->SetInt("uSkybox", 0);
        }

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

    bool LoadCubeMap(const std::string& directory) {
        const char* faces[] = {"right.jpg","left.jpg","top.jpg","bottom.jpg","front.jpg","back.jpg"};
        const char* altFaces[] = {"right.tga","left.tga","top.tga","bottom.tga","front.tga","back.tga"};

        glGenTextures(1, &m_cubeMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMap);

        stbi_set_flip_vertically_on_load(0);

        int loaded = 0;
        for (int i = 0; i < 6; i++) {
            int w, h, ch;
            std::string paths[] = {
                directory + "/" + faces[i],
                "../" + directory + "/" + faces[i],
                "../../" + directory + "/" + faces[i],
                directory + "/" + altFaces[i],
                "../" + directory + "/" + altFaces[i],
                "../../" + directory + "/" + altFaces[i]
            };
            unsigned char* data = nullptr;
            for (auto& p : paths) {
                data = stbi_load(p.c_str(), &w, &h, &ch, 0);
                if (data) break;
            }
            if (data) {
                GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
                loaded++;
            } else {
                std::cerr << "Skybox: failed to load face " << faces[i] << std::endl;
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        if (loaded == 6) {
            useTexture = true;
            std::cout << "Skybox cubemap loaded from " << directory << std::endl;
            return true;
        }
        glDeleteTextures(1, &m_cubeMap); m_cubeMap = 0;
        return false;
    }

    void Render(const Mat4& view, const Mat4& proj) {
        if (m_VAO == 0) return;
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        if (useTexture && m_cubeMap && m_texShader) {
            m_texShader->Bind();
            m_texShader->SetMat4("uView", view.m);
            m_texShader->SetMat4("uProjection", proj.m);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMap);
        } else if (m_procShader) {
            m_procShader->Bind();
            m_procShader->SetMat4("uView", view.m);
            m_procShader->SetMat4("uProjection", proj.m);
            m_procShader->SetVec3("uTopColor", topColor);
            m_procShader->SetVec3("uBottomColor", bottomColor);
            m_procShader->SetVec3("uHorizonColor", horizonColor);
        }

        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

    void SetDayPreset()   { topColor=Vec3(0.2f,0.4f,0.8f); bottomColor=Vec3(0.8f,0.85f,0.9f); horizonColor=Vec3(0.9f,0.9f,0.95f); }
    void SetDuskPreset()  { topColor=Vec3(0.1f,0.1f,0.3f); bottomColor=Vec3(0.9f,0.4f,0.2f); horizonColor=Vec3(1.0f,0.6f,0.3f); }
    void SetNightPreset() { topColor=Vec3(0.02f,0.02f,0.08f); bottomColor=Vec3(0.05f,0.05f,0.15f); horizonColor=Vec3(0.08f,0.08f,0.2f); }

    void Destroy() {
        if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
        if (m_VBO) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
        if (m_cubeMap) { glDeleteTextures(1, &m_cubeMap); m_cubeMap = 0; }
        m_procShader.reset();
        m_texShader.reset();
    }

private:
    unsigned int m_VAO = 0, m_VBO = 0;
    unsigned int m_cubeMap = 0;
    std::unique_ptr<Shader> m_procShader;
    std::unique_ptr<Shader> m_texShader;
};

} // namespace MiniEngine
