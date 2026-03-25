/**
 * Renderer2D.h - 2D渲染器
 */

#ifndef MINI_ENGINE_RENDERER2D_H
#define MINI_ENGINE_RENDERER2D_H

#include "Shader.h"
#include "math/Math.h"
#include <memory>

namespace MiniEngine {

struct Vertex2D {
    Math::Vec2 position;
    Math::Vec4 color;
    Math::Vec2 texCoord;
    
    Vertex2D() = default;
    Vertex2D(const Math::Vec2& pos, const Math::Vec4& col)
        : position(pos), color(col), texCoord(0.0f, 0.0f) {}
    Vertex2D(float x, float y, float r, float g, float b, float a = 1.0f)
        : position(x, y), color(r, g, b, a), texCoord(0.0f, 0.0f) {}
};

class Renderer2D {
public:
    Renderer2D();
    ~Renderer2D();
    
    bool Initialize();
    void Shutdown();
    
    // 绘制函数
    void DrawTriangle(const Vertex2D& v1, const Vertex2D& v2, const Vertex2D& v3);
    void DrawTriangle(const Math::Vec2& p1, const Math::Vec2& p2, 
                      const Math::Vec2& p3, const Math::Vec4& color);
    void DrawRect(const Math::Vec2& position, const Math::Vec2& size, const Math::Vec4& color);
    void DrawLine(const Math::Vec2& start, const Math::Vec2& end,
                  const Math::Vec4& color, float thickness = 1.0f);
    void DrawCircle(const Math::Vec2& center, float radius,
                    const Math::Vec4& color, int segments = 32);
    void DrawPoint(const Math::Vec2& position, const Math::Vec4& color, float size = 5.0f);
    
    // 向量可视化
    void DrawVector(const Math::Vec2& origin, const Math::Vec2& direction,
                    const Math::Vec4& color, float scale = 1.0f);
    void DrawAxes(float size = 0.8f);
    void DrawGrid(float spacing = 0.1f, const Math::Vec4& color = Math::Vec4(0.3f, 0.3f, 0.3f, 1.0f));
    
private:
    bool CreateDefaultShader();
    void SetupVertexAttributes();
    
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_EBO = 0;
    
    std::unique_ptr<Shader> m_shader;
    
    static constexpr int MAX_VERTICES = 10000;
    static constexpr int MAX_INDICES = 30000;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_RENDERER2D_H
