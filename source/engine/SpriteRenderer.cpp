/**
 * SpriteRenderer.cpp - 精灵渲染器实现
 * 
 * Shader文件：
 *   - data/shader/sprite.vs (顶点着色器)
 *   - data/shader/sprite.fs (片段着色器)
 */

#include "SpriteRenderer.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace MiniEngine {

// ============================================================================
// 构造和析构
// ============================================================================

SpriteRenderer::SpriteRenderer() = default;

SpriteRenderer::~SpriteRenderer() {
    Shutdown();
}

// ============================================================================
// 初始化
// ============================================================================

bool SpriteRenderer::Initialize() {
    std::cout << "[SpriteRenderer] Initializing..." << std::endl;
    
    // 创建着色器
    if (!CreateShader()) {
        std::cerr << "[SpriteRenderer] Failed to create shader!" << std::endl;
        return false;
    }
    
    // 设置四边形顶点缓冲
    SetupQuad();
    
    // 创建默认白色纹理（1x1像素）
    m_whiteTexture = std::make_unique<Texture>();
    m_whiteTexture->CreateSolidColor(Math::Vec4::White());
    
    // 设置默认投影矩阵（单位矩阵）
    m_projection = Math::Mat4::Identity();
    
    std::cout << "[SpriteRenderer] Initialized successfully." << std::endl;
    return true;
}

void SpriteRenderer::Shutdown() {
    // 检查OpenGL上下文
    if (glfwGetCurrentContext() == nullptr) {
        m_VAO = 0;
        m_VBO = 0;
        m_shader.reset();
        m_whiteTexture.reset();
        return;
    }
    
    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO != 0) {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
    
    m_shader.reset();
    m_whiteTexture.reset();
}

bool SpriteRenderer::CreateShader() {
    m_shader = std::make_unique<Shader>();
    
    if (!m_shader->LoadFromFile("sprite.vs", "sprite.fs")) {
        std::cerr << "[SpriteRenderer] ERROR: Failed to load shader files!" << std::endl;
        std::cerr << "  Please ensure these files exist:" << std::endl;
        std::cerr << "    - data/shader/sprite.vs" << std::endl;
        std::cerr << "    - data/shader/sprite.fs" << std::endl;
        return false;
    }
    
    std::cout << "[SpriteRenderer] Shader loaded successfully" << std::endl;
    return true;
}

void SpriteRenderer::SetupQuad() {
    /**
     * 四边形顶点数据
     * 
     * 每个顶点包含：
     *   - 位置 (x, y)：2个float
     *   - 纹理坐标 (u, v)：2个float
     * 
     * 四边形布局（以中心为原点）：
     * 
     *   3 ─────── 2
     *   │ ╲     │
     *   │   ╲   │   顶点顺序：0-1-2, 0-2-3
     *   │     ╲ │   （两个三角形）
     *   0 ─────── 1
     * 
     * 注意：我们使用单位尺寸（0-1），
     *      实际尺寸通过Model矩阵缩放
     */
    
    float vertices[] = {
        // 位置(x,y)    // UV(u,v)
        0.0f, 0.0f,    0.0f, 0.0f,   // 左下 (0)
        1.0f, 0.0f,    1.0f, 0.0f,   // 右下 (1)
        1.0f, 1.0f,    1.0f, 1.0f,   // 右上 (2)
        
        0.0f, 0.0f,    0.0f, 0.0f,   // 左下 (0)
        1.0f, 1.0f,    1.0f, 1.0f,   // 右上 (2)
        0.0f, 1.0f,    0.0f, 1.0f    // 左上 (3)
    };
    
    // 创建VAO
    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
    
    // 创建VBO
    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // 设置顶点属性
    // 属性0：位置
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 属性1：纹理坐标
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// ============================================================================
// 渲染
// ============================================================================

void SpriteRenderer::Begin() {
    // 启用混合（透明度）
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 禁用深度测试（2D渲染按绘制顺序）
    glDisable(GL_DEPTH_TEST);
    
    // 绑定着色器
    m_shader->Bind();
    
    // 设置投影矩阵
    m_shader->SetMat4("uProjection", m_projection.Data());
    
    // 设置纹理采样器
    m_shader->SetInt("uTexture", 0);
}

void SpriteRenderer::End() {
    // 解绑着色器
    m_shader->Unbind();
    
    // 恢复深度测试（如果需要的话）
    glEnable(GL_DEPTH_TEST);
}

void SpriteRenderer::SetProjection(const Math::Mat4& projection) {
    m_projection = projection;
}

void SpriteRenderer::DrawSprite(const Sprite& sprite) {
    // 检查可见性
    if (!sprite.IsVisible()) return;
    
    // 获取纹理（如果没有，使用白色纹理）
    const Texture* texture = nullptr;
    if (sprite.HasTexture()) {
        texture = sprite.GetTexture().get();
    } else {
        texture = m_whiteTexture.get();
    }
    
    // 获取模型矩阵
    Math::Mat4 model = sprite.GetModelMatrix();
    
    // 获取尺寸并应用到模型矩阵
    Math::Vec2 size = sprite.GetSize();
    model = model * Math::Mat4::Scale(size.x, size.y, 1.0f);
    
    // 绘制
    DrawQuadInternal(model, texture, sprite.GetColor(), 
                     sprite.GetUVMin(), sprite.GetUVMax());
}

void SpriteRenderer::DrawTexture(
    const Texture& texture,
    const Math::Vec2& position,
    const Math::Vec2& size,
    float rotation,
    const Math::Vec4& color)
{
    // 计算实际尺寸
    Math::Vec2 actualSize = size;
    if (size.x <= 0 || size.y <= 0) {
        actualSize = texture.GetSize();
    }
    
    // 构建模型矩阵
    // 以中心为锚点
    Math::Mat4 model = Math::Mat4::Translate(position);
    model = model * Math::Mat4::RotateZ(rotation);
    model = model * Math::Mat4::Translate(-actualSize.x * 0.5f, -actualSize.y * 0.5f, 0.0f);
    model = model * Math::Mat4::Scale(actualSize.x, actualSize.y, 1.0f);
    
    DrawQuadInternal(model, &texture, color, 
                     Math::Vec2(0, 0), Math::Vec2(1, 1));
}

void SpriteRenderer::DrawRect(
    const Math::Vec2& position,
    const Math::Vec2& size,
    const Math::Vec4& color,
    float rotation)
{
    // 构建模型矩阵
    Math::Mat4 model = Math::Mat4::Translate(position);
    model = model * Math::Mat4::RotateZ(rotation);
    model = model * Math::Mat4::Translate(-size.x * 0.5f, -size.y * 0.5f, 0.0f);
    model = model * Math::Mat4::Scale(size.x, size.y, 1.0f);
    
    DrawQuadInternal(model, m_whiteTexture.get(), color,
                     Math::Vec2(0, 0), Math::Vec2(1, 1));
}

void SpriteRenderer::DrawQuadInternal(
    const Math::Mat4& model,
    const Texture* texture,
    const Math::Vec4& color,
    const Math::Vec2& uvMin,
    const Math::Vec2& uvMax)
{
    // 设置模型矩阵
    m_shader->SetMat4("uModel", model.Data());
    
    // 设置颜色
    m_shader->SetVec4("uColor", color);
    
    // 设置UV范围（用于spritesheet动画）
    // 在着色器中：finalUV = uvMin + vertexUV * (uvMax - uvMin)
    m_shader->SetVec2("uUVMin", uvMin);
    m_shader->SetVec2("uUVMax", uvMax);
    
    // 绑定纹理
    if (texture) {
        texture->Bind(0);
    }
    
    // 绘制四边形
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace MiniEngine
