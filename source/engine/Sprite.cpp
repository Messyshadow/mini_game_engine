/**
 * Sprite.cpp - 精灵类实现
 */

#include "Sprite.h"

namespace MiniEngine {

// ============================================================================
// 构造函数
// ============================================================================

Sprite::Sprite() 
    : m_texture(nullptr)
    , m_position(Math::Vec2::Zero())
    , m_rotation(0.0f)
    , m_scale(Math::Vec2::One())
    , m_pivot(0.5f, 0.5f)
    , m_color(Math::Vec4::White())
    , m_uvMin(0.0f, 0.0f)
    , m_uvMax(1.0f, 1.0f)
    , m_visible(true)
    , m_flipX(false)
    , m_flipY(false)
{
}

Sprite::Sprite(std::shared_ptr<Texture> texture)
    : Sprite()  // 委托给默认构造函数
{
    m_texture = texture;
}

// ============================================================================
// 纹理
// ============================================================================

void Sprite::SetTexture(std::shared_ptr<Texture> texture) {
    m_texture = texture;
}

// ============================================================================
// 尺寸
// ============================================================================

void Sprite::SetSize(const Math::Vec2& size) {
    SetSize(size.x, size.y);
}

void Sprite::SetSize(float width, float height) {
    /**
     * 根据目标尺寸计算缩放值
     * 
     * 目标尺寸 = 纹理尺寸 × 缩放
     * 所以：缩放 = 目标尺寸 / 纹理尺寸
     */
    
    if (m_texture && m_texture->IsValid()) {
        float texWidth = static_cast<float>(m_texture->GetWidth());
        float texHeight = static_cast<float>(m_texture->GetHeight());
        
        // 避免除以零
        if (texWidth > 0 && texHeight > 0) {
            m_scale.x = width / texWidth;
            m_scale.y = height / texHeight;
        }
    } else {
        // 没有纹理时，直接设置缩放（假设基础尺寸为1x1）
        m_scale.x = width;
        m_scale.y = height;
    }
}

Math::Vec2 Sprite::GetSize() const {
    /**
     * 计算实际显示尺寸
     * = 纹理尺寸 × 缩放
     */
    
    if (m_texture && m_texture->IsValid()) {
        return Math::Vec2(
            m_texture->GetWidth() * m_scale.x,
            m_texture->GetHeight() * m_scale.y
        );
    }
    
    // 没有纹理时返回缩放值（假设1x1）
    return m_scale;
}

// ============================================================================
// 精灵图集
// ============================================================================

void Sprite::SetFrameFromSheet(int frameIndex, int columns, int rows) {
    /**
     * 从精灵图集中选择一帧
     * 
     * 精灵图集布局示例（4列2行）：
     * 
     *   ┌────┬────┬────┬────┐
     *   │ 0  │ 1  │ 2  │ 3  │  第0行（V: 0.5-1.0）
     *   ├────┼────┼────┼────┤
     *   │ 4  │ 5  │ 6  │ 7  │  第1行（V: 0.0-0.5）
     *   └────┴────┴────┴────┘
     * 
     * 计算方法：
     * - 每格宽度 = 1.0 / columns
     * - 每格高度 = 1.0 / rows
     * - 列号 = frameIndex % columns
     * - 行号 = frameIndex / columns
     * - UV需要翻转Y轴（OpenGL原点在左下）
     */
    
    if (columns <= 0 || rows <= 0) return;
    
    // 每格的UV尺寸
    float frameWidth = 1.0f / columns;
    float frameHeight = 1.0f / rows;
    
    // 计算帧在图集中的位置
    int col = frameIndex % columns;
    int row = frameIndex / columns;
    
    // 翻转行号（图片通常从上到下，但UV从下到上）
    row = rows - 1 - row;
    
    // 计算UV坐标
    m_uvMin.x = col * frameWidth;
    m_uvMin.y = row * frameHeight;
    m_uvMax.x = (col + 1) * frameWidth;
    m_uvMax.y = (row + 1) * frameHeight;
}

// ============================================================================
// 变换矩阵
// ============================================================================

Math::Mat4 Sprite::GetModelMatrix() const {
    /**
     * 构建模型矩阵
     * 
     * 变换顺序（从右到左读）：
     * 1. 根据锚点偏移（使锚点成为旋转中心）
     * 2. 缩放
     * 3. 旋转
     * 4. 平移到目标位置
     * 
     * M = Translate(position) * Rotate * Scale * Translate(-pivot * size)
     */
    
    // 获取精灵尺寸
    Math::Vec2 size = GetSize();
    
    // 计算锚点偏移（将锚点移到原点）
    // 例如：锚点在中心(0.5,0.5)，尺寸100x100
    // 偏移 = -0.5 * 100 = -50，将精灵中心移到原点
    float pivotOffsetX = -m_pivot.x * size.x;
    float pivotOffsetY = -m_pivot.y * size.y;
    
    // 处理翻转
    float scaleX = m_flipX ? -1.0f : 1.0f;
    float scaleY = m_flipY ? -1.0f : 1.0f;
    
    // 构建变换矩阵
    // 注意矩阵乘法顺序：先写的后执行
    Math::Mat4 model = Math::Mat4::Identity();
    
    // 4. 最后平移到目标位置
    model = Math::Mat4::Translate(m_position) * model;
    
    // 3. 旋转
    model = model * Math::Mat4::RotateZ(m_rotation);
    
    // 2. 缩放（包含翻转）
    model = model * Math::Mat4::Scale(scaleX, scaleY, 1.0f);
    
    // 1. 先偏移锚点
    model = model * Math::Mat4::Translate(pivotOffsetX, pivotOffsetY, 0.0f);
    
    return model;
}

} // namespace MiniEngine
