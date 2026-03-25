/**
 * SpriteRenderer.h - 精灵渲染器
 * 
 * ============================================================================
 * 精灵渲染器的作用
 * ============================================================================
 * 
 * 精灵渲染器负责将2D精灵绘制到屏幕上。
 * 
 * 主要功能：
 * 1. 管理着色器程序
 * 2. 处理顶点数据上传
 * 3. 绑定纹理
 * 4. 应用变换矩阵（投影、模型）
 * 
 * ============================================================================
 * 使用方法
 * ============================================================================
 * 
 *   // 初始化
 *   SpriteRenderer renderer;
 *   renderer.Initialize();
 *   
 *   // 每帧渲染
 *   renderer.Begin();
 *   renderer.SetProjection(Mat4::Ortho2D(0, 1280, 0, 720));
 *   renderer.DrawSprite(playerSprite);
 *   renderer.DrawSprite(enemySprite);
 *   renderer.End();
 */

#ifndef MINI_ENGINE_SPRITE_RENDERER_H
#define MINI_ENGINE_SPRITE_RENDERER_H

#include "Shader.h"
#include "Texture.h"
#include "Sprite.h"
#include "math/Math.h"
#include <memory>

namespace MiniEngine {

class SpriteRenderer {
public:
    SpriteRenderer();
    ~SpriteRenderer();
    
    // 禁止拷贝
    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;
    
    /**
     * 初始化渲染器
     * 创建着色器和顶点缓冲
     */
    bool Initialize();
    
    /**
     * 清理资源
     */
    void Shutdown();
    
    /**
     * 开始渲染批次
     */
    void Begin();
    
    /**
     * 结束渲染批次
     */
    void End();
    
    /**
     * 设置投影矩阵
     * 通常使用正交投影：Mat4::Ortho2D(0, width, 0, height)
     */
    void SetProjection(const Math::Mat4& projection);
    
    /**
     * 绘制精灵
     */
    void DrawSprite(const Sprite& sprite);
    
    /**
     * 绘制纹理（简化接口）
     * 
     * @param texture 纹理
     * @param position 位置（像素）
     * @param size 尺寸，(0,0)表示使用纹理原始尺寸
     * @param rotation 旋转（弧度）
     * @param color 颜色
     */
    void DrawTexture(
        const Texture& texture,
        const Math::Vec2& position,
        const Math::Vec2& size = Math::Vec2::Zero(),
        float rotation = 0.0f,
        const Math::Vec4& color = Math::Vec4::White()
    );
    
    /**
     * 绘制纯色矩形
     */
    void DrawRect(
        const Math::Vec2& position,
        const Math::Vec2& size,
        const Math::Vec4& color,
        float rotation = 0.0f
    );
    
private:
    /**
     * 创建默认着色器
     */
    bool CreateShader();
    
    /**
     * 设置四边形顶点缓冲
     */
    void SetupQuad();
    
    /**
     * 内部绘制四边形
     */
    void DrawQuadInternal(
        const Math::Mat4& model,
        const Texture* texture,
        const Math::Vec4& color,
        const Math::Vec2& uvMin,
        const Math::Vec2& uvMax
    );
    
    // OpenGL对象
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    
    // 着色器
    std::unique_ptr<Shader> m_shader;
    
    // 投影矩阵
    Math::Mat4 m_projection;
    
    // 默认白色纹理（用于纯色绘制）
    std::unique_ptr<Texture> m_whiteTexture;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_SPRITE_RENDERER_H
