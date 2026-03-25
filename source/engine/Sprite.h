/**
 * Sprite.h - 精灵类
 * 
 * ============================================================================
 * 什么是精灵（Sprite）？
 * ============================================================================
 * 
 * 精灵是2D游戏中最基本的可视化元素。
 * 
 * 一个精灵包含：
 * - 纹理（外观）
 * - 变换（位置、旋转、缩放）
 * - 颜色/透明度
 * - 可选：动画帧、锚点等
 * 
 * ============================================================================
 * 精灵的锚点（Pivot/Anchor）
 * ============================================================================
 * 
 * 锚点决定了精灵的"中心"在哪里。
 * 
 * 常见的锚点设置：
 * 
 *   (0,1)────(0.5,1)────(1,1)
 *     │                    │
 *   (0,0.5)  (0.5,0.5)  (1,0.5)    (0.5, 0.5) = 中心（默认）
 *     │                    │       (0.5, 0)   = 底部中心（适合角色）
 *   (0,0)────(0.5,0)────(1,0)      (0, 0)     = 左下角
 * 
 * 锚点影响：
 * - 旋转时绕哪个点旋转
 * - 位置坐标表示精灵的哪个点
 * 
 * ============================================================================
 * UV坐标与精灵图集（Sprite Sheet）
 * ============================================================================
 * 
 * 精灵图集是将多个小图合并成一张大图，以减少纹理切换。
 * 
 *   ┌────┬────┬────┬────┐
 *   │ 0  │ 1  │ 2  │ 3  │   每个格子是一帧动画
 *   ├────┼────┼────┼────┤   通过设置UV坐标来选择显示哪一帧
 *   │ 4  │ 5  │ 6  │ 7  │
 *   └────┴────┴────┴────┘
 * 
 * 例如显示第2帧（索引2）：
 *   UV = (0.5, 0.5) 到 (0.75, 1.0)
 */

#ifndef MINI_ENGINE_SPRITE_H
#define MINI_ENGINE_SPRITE_H

#include "Texture.h"
#include "math/Math.h"
#include <memory>

namespace MiniEngine {

/**
 * 精灵类
 * 
 * 表示一个2D可渲染的图像对象
 */
class Sprite {
public:
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    /**
     * 默认构造
     * 创建一个没有纹理的空精灵
     */
    Sprite();
    
    /**
     * 从纹理创建精灵
     * 
     * @param texture 共享的纹理指针
     */
    explicit Sprite(std::shared_ptr<Texture> texture);
    
    /**
     * 析构函数
     */
    ~Sprite() = default;
    
    // ========================================================================
    // 纹理设置
    // ========================================================================
    
    /**
     * 设置纹理
     */
    void SetTexture(std::shared_ptr<Texture> texture);
    
    /**
     * 获取纹理
     */
    std::shared_ptr<Texture> GetTexture() const { return m_texture; }
    
    /**
     * 检查是否有纹理
     */
    bool HasTexture() const { return m_texture != nullptr && m_texture->IsValid(); }
    
    // ========================================================================
    // 变换属性
    // ========================================================================
    
    // --- 位置 ---
    
    void SetPosition(const Math::Vec2& position) { m_position = position; }
    void SetPosition(float x, float y) { m_position = Math::Vec2(x, y); }
    Math::Vec2 GetPosition() const { return m_position; }
    
    void Move(const Math::Vec2& delta) { m_position += delta; }
    void Move(float dx, float dy) { m_position.x += dx; m_position.y += dy; }
    
    // --- 旋转 ---
    
    /** 设置旋转角度（弧度） */
    void SetRotation(float radians) { m_rotation = radians; }
    
    /** 设置旋转角度（度） */
    void SetRotationDegrees(float degrees) { m_rotation = Math::ToRadians(degrees); }
    
    /** 获取旋转角度（弧度） */
    float GetRotation() const { return m_rotation; }
    
    /** 获取旋转角度（度） */
    float GetRotationDegrees() const { return Math::ToDegrees(m_rotation); }
    
    /** 旋转（增量） */
    void Rotate(float deltaRadians) { m_rotation += deltaRadians; }
    
    // --- 缩放 ---
    
    void SetScale(const Math::Vec2& scale) { m_scale = scale; }
    void SetScale(float sx, float sy) { m_scale = Math::Vec2(sx, sy); }
    void SetScale(float uniformScale) { m_scale = Math::Vec2(uniformScale, uniformScale); }
    Math::Vec2 GetScale() const { return m_scale; }
    
    // --- 尺寸 ---
    
    /**
     * 设置显示尺寸（像素）
     * 这会自动计算相应的缩放值
     */
    void SetSize(const Math::Vec2& size);
    void SetSize(float width, float height);
    
    /**
     * 获取显示尺寸（像素）
     * = 纹理尺寸 × 缩放
     */
    Math::Vec2 GetSize() const;
    
    // ========================================================================
    // 锚点
    // ========================================================================
    
    /**
     * 设置锚点
     * 
     * @param pivot 锚点位置，(0,0)=左下角，(1,1)=右上角，(0.5,0.5)=中心
     */
    void SetPivot(const Math::Vec2& pivot) { m_pivot = pivot; }
    void SetPivot(float px, float py) { m_pivot = Math::Vec2(px, py); }
    Math::Vec2 GetPivot() const { return m_pivot; }
    
    /** 设置锚点为中心 */
    void SetPivotCenter() { m_pivot = Math::Vec2(0.5f, 0.5f); }
    
    /** 设置锚点为底部中心（适合角色） */
    void SetPivotBottom() { m_pivot = Math::Vec2(0.5f, 0.0f); }
    
    // ========================================================================
    // 颜色和透明度
    // ========================================================================
    
    /**
     * 设置着色颜色
     * 纹理颜色会与这个颜色相乘
     * 白色(1,1,1,1)表示使用原始颜色
     */
    void SetColor(const Math::Vec4& color) { m_color = color; }
    void SetColor(float r, float g, float b, float a = 1.0f) { m_color = Math::Vec4(r, g, b, a); }
    Math::Vec4 GetColor() const { return m_color; }
    
    /** 设置透明度（0=完全透明，1=不透明） */
    void SetAlpha(float alpha) { m_color.w = alpha; }
    float GetAlpha() const { return m_color.w; }
    
    // ========================================================================
    // UV坐标（用于精灵图集）
    // ========================================================================
    
    /**
     * 设置UV坐标范围
     * 
     * @param uvMin 左下角UV (默认 0,0)
     * @param uvMax 右上角UV (默认 1,1)
     */
    void SetUV(const Math::Vec2& uvMin, const Math::Vec2& uvMax) {
        m_uvMin = uvMin;
        m_uvMax = uvMax;
    }
    
    /**
     * 从精灵图集设置UV
     * 
     * @param frameIndex 帧索引
     * @param columns 图集列数
     * @param rows 图集行数
     */
    void SetFrameFromSheet(int frameIndex, int columns, int rows);
    
    Math::Vec2 GetUVMin() const { return m_uvMin; }
    Math::Vec2 GetUVMax() const { return m_uvMax; }
    
    // ========================================================================
    // 变换矩阵
    // ========================================================================
    
    /**
     * 获取模型变换矩阵
     * 
     * 包含：平移、旋转、缩放，并考虑锚点偏移
     */
    Math::Mat4 GetModelMatrix() const;
    
    // ========================================================================
    // 可见性
    // ========================================================================
    
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    
    // ========================================================================
    // 翻转
    // ========================================================================
    
    void SetFlipX(bool flip) { m_flipX = flip; }
    void SetFlipY(bool flip) { m_flipY = flip; }
    bool GetFlipX() const { return m_flipX; }
    bool GetFlipY() const { return m_flipY; }
    
private:
    // 纹理（使用智能指针共享）
    std::shared_ptr<Texture> m_texture;
    
    // 变换
    Math::Vec2 m_position = Math::Vec2::Zero();  // 位置（像素）
    float m_rotation = 0.0f;                      // 旋转（弧度）
    Math::Vec2 m_scale = Math::Vec2::One();       // 缩放
    
    // 锚点（0-1范围）
    Math::Vec2 m_pivot = Math::Vec2(0.5f, 0.5f);  // 默认中心
    
    // 颜色调制
    Math::Vec4 m_color = Math::Vec4::White();
    
    // UV坐标
    Math::Vec2 m_uvMin = Math::Vec2(0.0f, 0.0f);
    Math::Vec2 m_uvMax = Math::Vec2(1.0f, 1.0f);
    
    // 状态
    bool m_visible = true;
    bool m_flipX = false;
    bool m_flipY = false;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_SPRITE_H
