/**
 * Renderer2D.cpp - 2D渲染器实现
 * 
 * Shader文件：
 *   - data/shader/basic2d.vs (顶点着色器)
 *   - data/shader/basic2d.fs (片段着色器)
 */

#include "Renderer2D.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>

namespace MiniEngine {

Renderer2D::Renderer2D() = default;
Renderer2D::~Renderer2D() { Shutdown(); }

bool Renderer2D::Initialize() {
    std::cout << "Initializing 2D Renderer..." << std::endl;
    
    if (!CreateDefaultShader()) {
        std::cerr << "Failed to create default shader!" << std::endl;
        return false;
    }
    
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    
    glBindVertexArray(m_VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex2D), nullptr, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);
    
    SetupVertexAttributes();
    glBindVertexArray(0);
    
    std::cout << "2D Renderer initialized successfully!" << std::endl;
    return true;
}

void Renderer2D::Shutdown() {
    // 检查OpenGL上下文是否还有效（glfwGetCurrentContext）
    // 如果GLFW已经终止，就不要调用OpenGL函数
    if (glfwGetCurrentContext() == nullptr) {
        // OpenGL上下文已失效，直接置零不调用gl函数
        m_VAO = 0;
        m_VBO = 0;
        m_EBO = 0;
        m_shader.reset();
        return;
    }
    
    if (m_VAO != 0) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
    if (m_VBO != 0) { glDeleteBuffers(1, &m_VBO); m_VBO = 0; }
    if (m_EBO != 0) { glDeleteBuffers(1, &m_EBO); m_EBO = 0; }
    m_shader.reset();
}

void Renderer2D::DrawTriangle(const Vertex2D& v1, const Vertex2D& v2, const Vertex2D& v3) {
    Vertex2D vertices[3] = { v1, v2, v3 };
    
    m_shader->Bind();
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Renderer2D::DrawTriangle(const Math::Vec2& p1, const Math::Vec2& p2, 
                              const Math::Vec2& p3, const Math::Vec4& color) {
    DrawTriangle(Vertex2D(p1, color), Vertex2D(p2, color), Vertex2D(p3, color));
}

void Renderer2D::DrawRect(const Math::Vec2& position, const Math::Vec2& size, const Math::Vec4& color) {
    float x = position.x, y = position.y;
    float w = size.x, h = size.y;
    
    Vertex2D vertices[4] = {
        Vertex2D(x,     y,     color.x, color.y, color.z, color.w),
        Vertex2D(x + w, y,     color.x, color.y, color.z, color.w),
        Vertex2D(x + w, y + h, color.x, color.y, color.z, color.w),
        Vertex2D(x,     y + h, color.x, color.y, color.z, color.w),
    };
    
    unsigned int indices[6] = { 0, 1, 2, 0, 2, 3 };
    
    m_shader->Bind();
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer2D::DrawLine(const Math::Vec2& start, const Math::Vec2& end,
                          const Math::Vec4& color, float thickness) {
    Vertex2D vertices[2] = { Vertex2D(start, color), Vertex2D(end, color) };
    
    glLineWidth(thickness);
    m_shader->Bind();
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void Renderer2D::DrawCircle(const Math::Vec2& center, float radius,
                            const Math::Vec4& color, int segments) {
    std::vector<Vertex2D> vertices;
    vertices.reserve(segments + 2);
    vertices.push_back(Vertex2D(center, color));
    
    for (int i = 0; i <= segments; i++) {
        float angle = (static_cast<float>(i) / segments) * Math::TWO_PI;
        float x = center.x + radius * std::cos(angle);
        float y = center.y + radius * std::sin(angle);
        vertices.push_back(Vertex2D(Math::Vec2(x, y), color));
    }
    
    m_shader->Bind();
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex2D), vertices.data());
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<int>(vertices.size()));
    glBindVertexArray(0);
}

void Renderer2D::DrawPoint(const Math::Vec2& position, const Math::Vec4& color, float size) {
    Vertex2D vertex(position, color);
    
    glPointSize(size);
    m_shader->Bind();
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex), &vertex);
    glDrawArrays(GL_POINTS, 0, 1);
    glBindVertexArray(0);
    glPointSize(1.0f);
}

void Renderer2D::DrawVector(const Math::Vec2& origin, const Math::Vec2& direction,
                            const Math::Vec4& color, float scale) {
    Math::Vec2 end = origin + direction * scale;
    DrawLine(origin, end, color, 2.0f);
    
    float arrowSize = 0.05f * scale;
    Math::Vec2 dir = direction.Normalized();
    Math::Vec2 perp = dir.Perpendicular();
    
    Math::Vec2 arrowPoint1 = end - dir * arrowSize + perp * (arrowSize * 0.5f);
    Math::Vec2 arrowPoint2 = end - dir * arrowSize - perp * (arrowSize * 0.5f);
    
    DrawLine(end, arrowPoint1, color, 2.0f);
    DrawLine(end, arrowPoint2, color, 2.0f);
}

void Renderer2D::DrawAxes(float size) {
    DrawVector(Math::Vec2::Zero(), Math::Vec2::Right(), Math::Vec4::Red(), size);
    DrawVector(Math::Vec2::Zero(), Math::Vec2::Up(), Math::Vec4::Green(), size);
}

void Renderer2D::DrawGrid(float spacing, const Math::Vec4& color) {
    for (float x = -1.0f; x <= 1.0f; x += spacing) {
        DrawLine(Math::Vec2(x, -1.0f), Math::Vec2(x, 1.0f), color, 1.0f);
    }
    for (float y = -1.0f; y <= 1.0f; y += spacing) {
        DrawLine(Math::Vec2(-1.0f, y), Math::Vec2(1.0f, y), color, 1.0f);
    }
}

bool Renderer2D::CreateDefaultShader() {
    m_shader = std::make_unique<Shader>();
    
    if (!m_shader->LoadFromFile("basic2d.vs", "basic2d.fs")) {
        std::cerr << "[Renderer2D] ERROR: Failed to load shader files!" << std::endl;
        std::cerr << "  Please ensure these files exist:" << std::endl;
        std::cerr << "    - data/shader/basic2d.vs" << std::endl;
        std::cerr << "    - data/shader/basic2d.fs" << std::endl;
        return false;
    }
    
    std::cout << "[Renderer2D] Shader loaded successfully" << std::endl;
    return true;
}

void Renderer2D::SetupVertexAttributes() {
    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, position));
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, color));
    glEnableVertexAttribArray(1);
    // TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, texCoord));
    glEnableVertexAttribArray(2);
}

} // namespace MiniEngine
