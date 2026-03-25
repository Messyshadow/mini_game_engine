/**
 * AnimatedSprite.cpp - 带动画的精灵实现
 */

#include "AnimatedSprite.h"

namespace MiniEngine {

// ============================================================================
// 构造函数
// ============================================================================

AnimatedSprite::AnimatedSprite()
    : m_position(0.0f, 0.0f)
    , m_size(64.0f, 64.0f)
    , m_scale(1.0f, 1.0f)
    , m_rotation(0.0f)
    , m_pivot(0.5f, 0.5f)  // 默认中心锚点
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_flipX(false)
    , m_flipY(false)
    , m_visible(true)
{
}

// ============================================================================
// 纹理和图集
// ============================================================================

void AnimatedSprite::SetTexture(std::shared_ptr<Texture> texture) {
    m_texture = texture;
}

void AnimatedSprite::SetSpriteSheet(int columns, int rows) {
    m_animator.SetSpriteSheet(columns, rows);
}

// ============================================================================
// 动画管理
// ============================================================================

void AnimatedSprite::AddAnimation(const std::string& name, int startFrame, int endFrame,
                                   float frameRate, bool loop) {
    m_animator.AddClip(name, startFrame, endFrame, frameRate, loop);
}

void AnimatedSprite::Play(const std::string& name, bool restart) {
    m_animator.Play(name, restart);
}

void AnimatedSprite::Stop() {
    m_animator.Stop();
}

void AnimatedSprite::Pause() {
    m_animator.Pause();
}

void AnimatedSprite::Resume() {
    m_animator.Resume();
}

// ============================================================================
// 更新
// ============================================================================

void AnimatedSprite::Update(float deltaTime) {
    m_animator.Update(deltaTime);
}

// ============================================================================
// 渲染数据
// ============================================================================

void AnimatedSprite::GetCurrentUV(Math::Vec2& uvMin, Math::Vec2& uvMax) const {
    m_animator.GetCurrentUV(uvMin, uvMax);
    
    // 处理翻转
    if (m_flipX) {
        std::swap(uvMin.x, uvMax.x);
    }
    if (m_flipY) {
        std::swap(uvMin.y, uvMax.y);
    }
}

Math::Mat4 AnimatedSprite::GetModelMatrix() const {
    /**
     * 模型矩阵的构建顺序（从右往左读）：
     * 
     * M = Translate(position) × Rotate(rotation) × Scale(scale) × Translate(-pivot*size)
     * 
     * 变换顺序（对顶点从左往右应用）：
     * 1. 先将锚点移到原点
     * 2. 缩放
     * 3. 旋转
     * 4. 移动到最终位置
     */
    
    // 计算最终尺寸
    Math::Vec2 finalSize = GetRenderSize();
    
    // 锚点偏移
    Math::Vec2 pivotOffset(-m_pivot.x * finalSize.x, -m_pivot.y * finalSize.y);
    
    // 构建矩阵
    Math::Mat4 model = Math::Mat4::Identity();
    
    // 1. 平移到最终位置
    model = Math::Mat4::Translate(m_position.x, m_position.y, 0.0f);
    
    // 2. 旋转
    model = model * Math::Mat4::RotateZ(m_rotation);
    
    // 3. 缩放（包含在finalSize中，这里额外处理翻转）
    float scaleX = m_flipX ? -1.0f : 1.0f;
    float scaleY = m_flipY ? -1.0f : 1.0f;
    if (scaleX != 1.0f || scaleY != 1.0f) {
        model = model * Math::Mat4::Scale(scaleX, scaleY, 1.0f);
    }
    
    // 4. 锚点偏移
    model = model * Math::Mat4::Translate(pivotOffset.x, pivotOffset.y, 0.0f);
    
    // 5. 缩放到最终尺寸
    model = model * Math::Mat4::Scale(finalSize.x, finalSize.y, 1.0f);
    
    return model;
}

} // namespace MiniEngine
