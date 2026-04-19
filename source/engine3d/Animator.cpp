/**
 * Animator.cpp - SkinnedMesh的OpenGL实现
 */

#include "engine3d/Animator.h"
#include <glad/gl.h>

namespace MiniEngine {

void SkinnedMesh::Create(const std::vector<SkinnedVertex>& verts,
                         const std::vector<uint32_t>& idxs)
{
    Destroy();
    m_vertexCount = (int)verts.size();
    m_indexCount  = (int)idxs.size();
    
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    
    glBindVertexArray(m_VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(SkinnedVertex), verts.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxs.size() * sizeof(uint32_t), idxs.data(), GL_STATIC_DRAW);
    
    int stride = sizeof(SkinnedVertex);
    
    // location 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertex, position));
    
    // location 1: normal (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertex, normal));
    
    // location 2: color (vec3)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertex, color));
    
    // location 3: texCoord (vec2)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertex, texCoord));
    
    // location 4: boneIDs (ivec4) — 必须用glVertexAttribIPointer（整数属性）
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_INT, stride, (void*)offsetof(SkinnedVertex, boneIDs));
    
    // location 5: boneWeights (vec4)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertex, boneWeights));
    
    glBindVertexArray(0);
}

void SkinnedMesh::Draw() const {
    if (m_VAO == 0) return;
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void SkinnedMesh::Destroy() {
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
        m_VAO = m_VBO = m_EBO = 0;
    }
}

} // namespace MiniEngine
