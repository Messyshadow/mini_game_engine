/**
 * Camera2D.h - 2D摄像机系统
 * 
 * ============================================================================
 * 概述
 * ============================================================================
 * 
 * 2D摄像机控制游戏视角，主要功能：
 * 1. 跟随目标（玩家）
 * 2. 平滑移动（不会突然跳跃）
 * 3. 边界限制（不超出地图范围）
 * 4. 震动效果
 * 
 * ============================================================================
 * 摄像机工作原理
 * ============================================================================
 * 
 *   世界坐标系：
 *   ┌────────────────────────────────────────┐
 *   │                                        │
 *   │    ┌──────────────┐                    │
 *   │    │   屏幕视口   │ ← 摄像机看到的区域 │
 *   │    │      🧙      │                    │
 *   │    └──────────────┘                    │
 *   │                                        │
 *   └────────────────────────────────────────┘
 *   
 *   屏幕坐标 = 世界坐标 - 摄像机位置
 * 
 * ============================================================================
 * 平滑跟随算法（Lerp插值）
 * ============================================================================
 * 
 *   newPos = currentPos + (targetPos - currentPos) * smoothSpeed * deltaTime
 *   
 *   smoothSpeed越大，跟随越快
 *   smoothSpeed越小，跟随越慢（更平滑）
 */

#ifndef MINI_ENGINE_CAMERA2D_H
#define MINI_ENGINE_CAMERA2D_H

#include "math/Math.h"

namespace MiniEngine {

/**
 * 2D摄像机类
 */
class Camera2D {
public:
    Camera2D();
    ~Camera2D() = default;
    
    // ========================================================================
    // 更新
    // ========================================================================
    
    /**
     * 每帧更新摄像机
     */
    void Update(float deltaTime);
    
    // ========================================================================
    // 目标跟随
    // ========================================================================
    
    void SetTarget(const Math::Vec2& target) { m_target = target; }
    Math::Vec2 GetTarget() const { return m_target; }
    
    /**
     * 设置平滑速度
     * @param speed 0.1=很慢, 5.0=中等, 10+=快速
     */
    void SetSmoothSpeed(float speed) { m_smoothSpeed = speed; }
    float GetSmoothSpeed() const { return m_smoothSpeed; }
    
    /** 立即移动到目标位置（无平滑） */
    void SnapToTarget();
    
    // ========================================================================
    // 位置和视口
    // ========================================================================
    
    Math::Vec2 GetPosition() const { return m_position; }
    void SetPosition(const Math::Vec2& pos) { m_position = pos; }
    
    void SetViewportSize(float width, float height);
    void SetViewportSize(const Math::Vec2& size);
    Math::Vec2 GetViewportSize() const { return m_viewportSize; }
    
    // ========================================================================
    // 边界限制
    // ========================================================================
    
    void SetWorldBounds(float minX, float minY, float maxX, float maxY);
    void SetWorldBounds(const Math::Vec2& min, const Math::Vec2& max);
    void ClearWorldBounds() { m_hasBounds = false; }
    bool HasWorldBounds() const { return m_hasBounds; }
    
    // ========================================================================
    // 缩放
    // ========================================================================
    
    void SetZoom(float zoom) { m_zoom = zoom; }
    float GetZoom() const { return m_zoom; }
    
    // ========================================================================
    // 震动效果
    // ========================================================================
    
    void Shake(float intensity, float duration);
    bool IsShaking() const { return m_shakeTimer > 0; }
    
    // ========================================================================
    // 坐标转换
    // ========================================================================
    
    Math::Vec2 WorldToScreen(const Math::Vec2& worldPos) const;
    Math::Vec2 ScreenToWorld(const Math::Vec2& screenPos) const;
    Math::Mat4 GetViewMatrix() const;
    
private:
    Math::Vec2 m_position;
    Math::Vec2 m_target;
    Math::Vec2 m_viewportSize;
    
    float m_smoothSpeed = 5.0f;
    float m_zoom = 1.0f;
    
    bool m_hasBounds = false;
    Math::Vec2 m_boundsMin;
    Math::Vec2 m_boundsMax;
    
    float m_shakeIntensity = 0;
    float m_shakeTimer = 0;
    Math::Vec2 m_shakeOffset;
    
    void ApplyBounds();
    void UpdateShake(float deltaTime);
    Math::Vec2 CalculateTargetPosition() const;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_CAMERA2D_H
