/**
 * Mesh3D.cpp - 3D网格实现
 */

#include <glad/gl.h>  // 必须在其他GL相关头文件之前
#include "Mesh3D.h"
#include <cmath>

namespace MiniEngine {

Mesh3D::~Mesh3D() { Destroy(); }

Mesh3D::Mesh3D(Mesh3D&& o) noexcept 
    : m_VAO(o.m_VAO), m_VBO(o.m_VBO), m_EBO(o.m_EBO),
      m_vertexCount(o.m_vertexCount), m_indexCount(o.m_indexCount) {
    o.m_VAO = o.m_VBO = o.m_EBO = 0;
    o.m_vertexCount = o.m_indexCount = 0;
}

Mesh3D& Mesh3D::operator=(Mesh3D&& o) noexcept {
    if (this != &o) {
        Destroy();
        m_VAO = o.m_VAO; m_VBO = o.m_VBO; m_EBO = o.m_EBO;
        m_vertexCount = o.m_vertexCount; m_indexCount = o.m_indexCount;
        o.m_VAO = o.m_VBO = o.m_EBO = 0;
        o.m_vertexCount = o.m_indexCount = 0;
    }
    return *this;
}

void Mesh3D::Create(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices) {
    Destroy(); // 清理旧数据
    
    m_vertexCount = (int)vertices.size();
    m_indexCount = (int)indices.size();
    
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    
    glBindVertexArray(m_VAO);
    
    // 上传顶点数据
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D), vertices.data(), GL_STATIC_DRAW);
    
    // 上传索引数据
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
    
    // 设置顶点属性指针
    // location 0: position (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, position));
    glEnableVertexAttribArray(0);
    
    // location 1: normal (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, normal));
    glEnableVertexAttribArray(1);
    
    // location 2: color (vec3)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, color));
    glEnableVertexAttribArray(2);
    
    // location 3: texCoord (vec2)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)offsetof(Vertex3D, texCoord));
    glEnableVertexAttribArray(3);
    
    glBindVertexArray(0);
}

void Mesh3D::Draw() const {
    if (m_VAO == 0) return;
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh3D::Destroy() {
    if (m_EBO) { glDeleteBuffers(1, &m_EBO); m_EBO = 0; }
    if (m_VBO) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
    if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
    m_vertexCount = m_indexCount = 0;
}

// ============================================================================
// 预设形状
// ============================================================================

Mesh3D Mesh3D::CreateColorCube(float s) {
    float h = s * 0.5f;
    
    // 每面4个顶点（因为每面法线不同，不能共享顶点）= 24个顶点
    // 每面的颜色不同
    std::vector<Vertex3D> v = {
        // 前面 (Z+) — 红色
        {{-h,-h, h}, { 0, 0, 1}, {1.0f, 0.3f, 0.3f}, {0,0}},
        {{ h,-h, h}, { 0, 0, 1}, {1.0f, 0.3f, 0.3f}, {1,0}},
        {{ h, h, h}, { 0, 0, 1}, {1.0f, 0.5f, 0.5f}, {1,1}},
        {{-h, h, h}, { 0, 0, 1}, {1.0f, 0.5f, 0.5f}, {0,1}},
        // 后面 (Z-) — 绿色
        {{ h,-h,-h}, { 0, 0,-1}, {0.3f, 1.0f, 0.3f}, {0,0}},
        {{-h,-h,-h}, { 0, 0,-1}, {0.3f, 1.0f, 0.3f}, {1,0}},
        {{-h, h,-h}, { 0, 0,-1}, {0.5f, 1.0f, 0.5f}, {1,1}},
        {{ h, h,-h}, { 0, 0,-1}, {0.5f, 1.0f, 0.5f}, {0,1}},
        // 左面 (X-) — 蓝色
        {{-h,-h,-h}, {-1, 0, 0}, {0.3f, 0.3f, 1.0f}, {0,0}},
        {{-h,-h, h}, {-1, 0, 0}, {0.3f, 0.3f, 1.0f}, {1,0}},
        {{-h, h, h}, {-1, 0, 0}, {0.5f, 0.5f, 1.0f}, {1,1}},
        {{-h, h,-h}, {-1, 0, 0}, {0.5f, 0.5f, 1.0f}, {0,1}},
        // 右面 (X+) — 黄色
        {{ h,-h, h}, { 1, 0, 0}, {1.0f, 1.0f, 0.3f}, {0,0}},
        {{ h,-h,-h}, { 1, 0, 0}, {1.0f, 1.0f, 0.3f}, {1,0}},
        {{ h, h,-h}, { 1, 0, 0}, {1.0f, 1.0f, 0.5f}, {1,1}},
        {{ h, h, h}, { 1, 0, 0}, {1.0f, 1.0f, 0.5f}, {0,1}},
        // 上面 (Y+) — 青色
        {{-h, h, h}, { 0, 1, 0}, {0.3f, 1.0f, 1.0f}, {0,0}},
        {{ h, h, h}, { 0, 1, 0}, {0.3f, 1.0f, 1.0f}, {1,0}},
        {{ h, h,-h}, { 0, 1, 0}, {0.5f, 1.0f, 1.0f}, {1,1}},
        {{-h, h,-h}, { 0, 1, 0}, {0.5f, 1.0f, 1.0f}, {0,1}},
        // 下面 (Y-) — 品红色
        {{-h,-h,-h}, { 0,-1, 0}, {1.0f, 0.3f, 1.0f}, {0,0}},
        {{ h,-h,-h}, { 0,-1, 0}, {1.0f, 0.3f, 1.0f}, {1,0}},
        {{ h,-h, h}, { 0,-1, 0}, {1.0f, 0.5f, 1.0f}, {1,1}},
        {{-h,-h, h}, { 0,-1, 0}, {1.0f, 0.5f, 1.0f}, {0,1}},
    };
    
    // 每面2个三角形，6个索引
    std::vector<uint32_t> idx;
    for (uint32_t face = 0; face < 6; face++) {
        uint32_t base = face * 4;
        idx.push_back(base); idx.push_back(base+1); idx.push_back(base+2);
        idx.push_back(base); idx.push_back(base+2); idx.push_back(base+3);
    }
    
    Mesh3D mesh;
    mesh.Create(v, idx);
    return mesh;
}

Mesh3D Mesh3D::CreateCube(float s) {
    // 纯白色立方体（配合光照使用）
    float h = s * 0.5f;
    Vec3 white(1, 1, 1);
    std::vector<Vertex3D> v = {
        {{-h,-h, h},{0,0,1},white,{0,0}}, {{h,-h,h},{0,0,1},white,{1,0}},
        {{h,h,h},{0,0,1},white,{1,1}}, {{-h,h,h},{0,0,1},white,{0,1}},
        {{h,-h,-h},{0,0,-1},white,{0,0}}, {{-h,-h,-h},{0,0,-1},white,{1,0}},
        {{-h,h,-h},{0,0,-1},white,{1,1}}, {{h,h,-h},{0,0,-1},white,{0,1}},
        {{-h,-h,-h},{-1,0,0},white,{0,0}}, {{-h,-h,h},{-1,0,0},white,{1,0}},
        {{-h,h,h},{-1,0,0},white,{1,1}}, {{-h,h,-h},{-1,0,0},white,{0,1}},
        {{h,-h,h},{1,0,0},white,{0,0}}, {{h,-h,-h},{1,0,0},white,{1,0}},
        {{h,h,-h},{1,0,0},white,{1,1}}, {{h,h,h},{1,0,0},white,{0,1}},
        {{-h,h,h},{0,1,0},white,{0,0}}, {{h,h,h},{0,1,0},white,{1,0}},
        {{h,h,-h},{0,1,0},white,{1,1}}, {{-h,h,-h},{0,1,0},white,{0,1}},
        {{-h,-h,-h},{0,-1,0},white,{0,0}}, {{h,-h,-h},{0,-1,0},white,{1,0}},
        {{h,-h,h},{0,-1,0},white,{1,1}}, {{-h,-h,h},{0,-1,0},white,{0,1}},
    };
    std::vector<uint32_t> idx;
    for (uint32_t f=0;f<6;f++){uint32_t b=f*4;idx.push_back(b);idx.push_back(b+1);idx.push_back(b+2);idx.push_back(b);idx.push_back(b+2);idx.push_back(b+3);}
    Mesh3D mesh; mesh.Create(v, idx); return mesh;
}

Mesh3D Mesh3D::CreatePlane(float w, float d, int sx, int sz) {
    std::vector<Vertex3D> verts;
    std::vector<uint32_t> indices;
    
    float hw = w * 0.5f, hd = d * 0.5f;
    
    for (int z = 0; z <= sz; z++) {
        for (int x = 0; x <= sx; x++) {
            float fx = (float)x / sx, fz = (float)z / sz;
            Vertex3D v;
            v.position = Vec3(-hw + fx * w, 0, -hd + fz * d);
            v.normal = Vec3(0, 1, 0);
            v.color = Vec3(0.4f, 0.5f, 0.4f);
            v.texCoord = Vec2(fx, fz);
            verts.push_back(v);
        }
    }
    
    for (int z = 0; z < sz; z++) {
        for (int x = 0; x < sx; x++) {
            uint32_t tl = z * (sx + 1) + x;
            uint32_t tr = tl + 1;
            uint32_t bl = tl + (sx + 1);
            uint32_t br = bl + 1;
            indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
            indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
        }
    }
    
    Mesh3D mesh; mesh.Create(verts, indices); return mesh;
}

Mesh3D Mesh3D::CreateSphere(float radius, int segments, int rings) {
    std::vector<Vertex3D> verts;
    std::vector<uint32_t> indices;
    const float PI = 3.14159265358979f;
    
    for (int r = 0; r <= rings; r++) {
        float phi = PI * (float)r / rings;
        float y = std::cos(phi) * radius;
        float ringRadius = std::sin(phi) * radius;
        
        for (int s = 0; s <= segments; s++) {
            float theta = 2.0f * PI * (float)s / segments;
            float x = std::cos(theta) * ringRadius;
            float z = std::sin(theta) * ringRadius;
            
            Vertex3D v;
            v.position = Vec3(x, y, z);
            v.normal = Vec3(x, y, z) * (1.0f / radius); // 归一化
            v.color = Vec3(0.8f, 0.8f, 0.9f);
            v.texCoord = Vec2((float)s / segments, (float)r / rings);
            verts.push_back(v);
        }
    }
    
    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < segments; s++) {
            uint32_t cur = r * (segments + 1) + s;
            uint32_t next = cur + segments + 1;
            indices.push_back(cur); indices.push_back(next); indices.push_back(cur + 1);
            indices.push_back(cur + 1); indices.push_back(next); indices.push_back(next + 1);
        }
    }
    
    Mesh3D mesh; mesh.Create(verts, indices); return mesh;
}

} // namespace MiniEngine
