/**
 * AnimatedSprite.h - 带动画的精灵
 * 
 * ============================================================================
 * 什么是AnimatedSprite？
 * ============================================================================
 * 
 * AnimatedSprite = Sprite + Animator
 * 
 * 它组合了：
 * - 精灵的渲染功能（位置、旋转、缩放、颜色）
 * - 动画控制器的播放功能（播放、暂停、切换动画）
 * 
 * 这是游戏中角色、敌人、特效等最常用的类。
 * 
 * ============================================================================
 * 使用示例
 * ============================================================================
 * 
 *   // 创建带动画的精灵
 *   AnimatedSprite player;
 *   
 *   // 设置纹理（精灵图集）
 *   player.SetTexture(playerTexture);
 *   
 *   // 设置图集布局
 *   player.SetSpriteSheet(8, 4);  // 8列4行
 *   
 *   // 添加动画
 *   player.AddAnimation("idle", 0, 3, 8.0f, true);
 *   player.AddAnimation("run", 4, 11, 12.0f, true);
 *   player.AddAnimation("jump", 12, 15, 10.0f, false);
 *   
 *   // 设置位置和大小
 *   player.SetPosition(400, 300);
 *   player.SetSize(64, 64);
 *   
 *   // 播放动画
 *   player.Play("idle");
 *   
 *   // 每帧更新
 *   player.Update(deltaTime);
 */

#ifndef MINI_ENGINE_ANIMATED_SPRITE_H
#define MINI_ENGINE_ANIMATED_SPRITE_H

#include "Animator.h"
#include "engine/Sprite.h"
#include "engine/Texture.h"
#include "math/Math.h"
#include <memory>

namespace MiniEngine {

/**
 * 带动画的精灵
 */
class AnimatedSprite {
public:
    // ========================================================================
    // 构造和析构
    // ========================================================================
    
    AnimatedSprite();
    ~AnimatedSprite() = default;
    
    // ========================================================================
    // 纹理和图集
    // ========================================================================
    
    /**
     * 设置纹理（精灵图集）
     */
    void SetTexture(std::shared_ptr<Texture> texture);
    
    /**
     * 获取纹理
     */
    std::shared_ptr<Texture> GetTexture() const { return m_texture; }
    
    /**
     * 设置精灵图集布局
     * 
     * @param columns 列数
     * @param rows 行数
     */
    void SetSpriteSheet(int columns, int rows);
    
    // ========================================================================
    // 动画管理
    // ========================================================================
    
    /**
     * 添加动画
     * 
     * @param name 动画名称
     * @param startFrame 起始帧
     * @param endFrame 结束帧
     * @param frameRate 帧率
     * @param loop 是否循环
     */
    void AddAnimation(const std::string& name, int startFrame, int endFrame,
                      float frameRate = 12.0f, bool loop = true);
    
    /**
     * 播放动画
     * 
     * @param name 动画名称
     * @param restart 是否重新开始
     */
    void Play(const std::string& name, bool restart = false);
    
    /**
     * 停止动画
     */
    void Stop();
    
    /**
     * 暂停动画
     */
    void Pause();
    
    /**
     * 继续播放
     */
    void Resume();
    
    /**
     * 获取动画控制器（用于高级控制）
     */
    Animator& GetAnimator() { return m_animator; }
    const Animator& GetAnimator() const { return m_animator; }
    
    // ========================================================================
    // 更新
    // ========================================================================
    
    /**
     * 更新动画
     * 
     * @param deltaTime 帧间隔时间
     */
    void Update(float deltaTime);
    
    // ========================================================================
    // 变换属性
    // ========================================================================
    
    /** 设置位置 */
    void SetPosition(const Math::Vec2& pos) { m_position = pos; }
    void SetPosition(float x, float y) { m_position = Math::Vec2(x, y); }
    
    /** 获取位置 */
    const Math::Vec2& GetPosition() const { return m_position; }
    
    /** 设置尺寸 */
    void SetSize(const Math::Vec2& size) { m_size = size; }
    void SetSize(float width, float height) { m_size = Math::Vec2(width, height); }
    
    /** 获取尺寸 */
    const Math::Vec2& GetSize() const { return m_size; }
    
    /** 设置缩放 */
    void SetScale(const Math::Vec2& scale) { m_scale = scale; }
    void SetScale(float scale) { m_scale = Math::Vec2(scale, scale); }
    void SetScale(float scaleX, float scaleY) { m_scale = Math::Vec2(scaleX, scaleY); }
    
    /** 获取缩放 */
    const Math::Vec2& GetScale() const { return m_scale; }
    
    /** 设置旋转（弧度） */
    void SetRotation(float radians) { m_rotation = radians; }
    
    /** 设置旋转（角度） */
    void SetRotationDegrees(float degrees) { m_rotation = Math::ToRadians(degrees); }
    
    /** 获取旋转（弧度） */
    float GetRotation() const { return m_rotation; }
    
    /** 设置锚点 (0,0)=左下, (0.5,0.5)=中心, (1,1)=右上 */
    void SetPivot(const Math::Vec2& pivot) { m_pivot = pivot; }
    void SetPivot(float x, float y) { m_pivot = Math::Vec2(x, y); }
    void SetPivotCenter() { m_pivot = Math::Vec2(0.5f, 0.5f); }
    
    /** 获取锚点 */
    const Math::Vec2& GetPivot() const { return m_pivot; }
    
    // ========================================================================
    // 颜色和翻转
    // ========================================================================
    
    /** 设置颜色调制 */
    void SetColor(const Math::Vec4& color) { m_color = color; }
    
    /** 获取颜色 */
    const Math::Vec4& GetColor() const { return m_color; }
    
    /** 设置透明度 */
    void SetAlpha(float alpha) { m_color.w = alpha; }
    
    /** 水平翻转 */
    void SetFlipX(bool flip) { m_flipX = flip; }
    bool GetFlipX() const { return m_flipX; }
    
    /** 垂直翻转 */
    void SetFlipY(bool flip) { m_flipY = flip; }
    bool GetFlipY() const { return m_flipY; }
    
    // ========================================================================
    // 可见性
    // ========================================================================
    
    /** 设置可见性 */
    void SetVisible(bool visible) { m_visible = visible; }
    
    /** 是否可见 */
    bool IsVisible() const { return m_visible; }
    
    // ========================================================================
    // 渲染数据
    // ========================================================================
    
    /**
     * 获取当前帧的UV坐标
     */
    void GetCurrentUV(Math::Vec2& uvMin, Math::Vec2& uvMax) const;
    
    /**
     * 获取模型矩阵
     */
    Math::Mat4 GetModelMatrix() const;
    
    /**
     * 获取最终渲染尺寸（考虑缩放）
     */
    Math::Vec2 GetRenderSize() const {
        return Math::Vec2(m_size.x * m_scale.x, m_size.y * m_scale.y);
    }
    
private:
    // 纹理
    std::shared_ptr<Texture> m_texture;
    
    // 动画控制器
    Animator m_animator;
    
    // 变换
    Math::Vec2 m_position;
    Math::Vec2 m_size;
    Math::Vec2 m_scale;
    float m_rotation;
    Math::Vec2 m_pivot;
    
    // 颜色和翻转
    Math::Vec4 m_color;
    bool m_flipX;
    bool m_flipY;
    
    // 可见性
    bool m_visible;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_ANIMATED_SPRITE_H
