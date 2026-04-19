/**
 * Camera3D.h - 3D摄像机系统
 *
 * 三种模式：
 * 1. Orbit（轨道）— 围绕目标点旋转，适合模型查看
 * 2. FPS（第一人称）— WASD移动 + 鼠标自由看，适合探索
 * 3. TPS（第三人称）— 跟随目标角色，保持一定距离和角度
 *
 * ============================================================================
 * 欧拉角摄像机数学
 * ============================================================================
 *
 * 用Yaw（偏航角，水平旋转）和Pitch（俯仰角，垂直旋转）定义方向：
 *
 *   forward.x = cos(pitch) * sin(yaw)
 *   forward.y = sin(pitch)
 *   forward.z = cos(pitch) * cos(yaw)
 *
 *   right = normalize(cross(forward, worldUp))
 *   up    = normalize(cross(right, forward))
 *
 * Pitch限制在[-89°, 89°]防止万向锁。
 */

#pragma once

#include "math/Math.h"
#include <cmath>
#include <algorithm>

namespace MiniEngine {

using Math::Vec3;
using Math::Mat4;

enum class CameraMode { Orbit, FPS, TPS };

class Camera3D {
public:
    CameraMode mode = CameraMode::Orbit;
    
    // 共用参数
    float yaw = -30.0f;        // 偏航角（度）
    float pitch = 25.0f;       // 俯仰角（度）
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 200.0f;
    
    // Orbit模式
    float orbitDistance = 12.0f;
    Vec3 orbitTarget = Vec3(0, 1, 0);
    
    // FPS模式
    Vec3 fpsPosition = Vec3(0, 2, 8);
    float moveSpeed = 8.0f;
    float sprintMultiplier = 2.5f;
    float mouseSensitivity = 0.15f;
    
    // TPS模式
    Vec3 tpsTarget = Vec3(0, 1, 0);    // 跟随的角色位置
    float tpsDistance = 6.0f;
    float tpsHeight = 2.5f;            // 摄像机高于角色的高度
    float tpsSmooth = 5.0f;            // 跟随平滑速度
    
    // 平滑插值
    bool enableSmoothing = true;
    float smoothSpeed = 8.0f;
    
    // ========================================================================
    // 获取矩阵
    // ========================================================================
    
    Vec3 GetPosition() const {
        switch (mode) {
            case CameraMode::Orbit: return GetOrbitEye();
            case CameraMode::FPS:   return fpsPosition;
            case CameraMode::TPS:   return m_currentPos;
            default: return Vec3(0, 5, 10);
        }
    }
    
    Vec3 GetForward() const {
        float yr = yaw * 0.0174533f, pr = pitch * 0.0174533f;
        return Vec3(std::cos(pr) * std::sin(yr),
                    std::sin(pr),
                    std::cos(pr) * std::cos(yr));
    }
    
    Mat4 GetViewMatrix() const {
        Vec3 eye = GetPosition();
        Vec3 target;
        switch (mode) {
            case CameraMode::Orbit: target = orbitTarget; break;
            case CameraMode::FPS:   target = eye + GetForward(); break;
            case CameraMode::TPS:   target = tpsTarget + Vec3(0, 1.2f, 0); break;
            default: target = Vec3(0, 0, 0);
        }
        return Mat4::LookAt(eye, target, Vec3(0, 1, 0));
    }
    
    Mat4 GetProjectionMatrix(float aspect) const {
        return Mat4::Perspective(fov * 0.0174533f, aspect, nearPlane, farPlane);
    }
    
    // ========================================================================
    // 更新
    // ========================================================================
    
    /** FPS模式移动（每帧调用） */
    void ProcessFPSMovement(float forward, float right, float up, float dt, bool sprint = false) {
        if (mode != CameraMode::FPS) return;
        float speed = moveSpeed * (sprint ? sprintMultiplier : 1.0f) * dt;
        Vec3 fwd = GetForward();
        Vec3 flatFwd = Vec3(fwd.x, 0, fwd.z);
        float len = std::sqrt(flatFwd.x*flatFwd.x + flatFwd.z*flatFwd.z);
        if (len > 0.001f) flatFwd = flatFwd * (1.0f / len);
        Vec3 rt = Vec3(flatFwd.z, 0, -flatFwd.x); // cross(flatFwd, up)
        
        fpsPosition = fpsPosition + flatFwd * (forward * speed) + rt * (right * speed) + Vec3(0, up * speed, 0);
    }
    
    /** 鼠标移动（Yaw/Pitch） */
    void ProcessMouseMove(float dx, float dy) {
        yaw += dx * mouseSensitivity;
        pitch -= dy * mouseSensitivity;
        // 限制Pitch防止万向锁
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }
    
    /** 滚轮缩放 */
    void ProcessScroll(float delta) {
        if (mode == CameraMode::Orbit) {
            orbitDistance -= delta * 0.8f;
            orbitDistance = std::max(1.0f, std::min(50.0f, orbitDistance));
        } else if (mode == CameraMode::TPS) {
            tpsDistance -= delta * 0.5f;
            tpsDistance = std::max(2.0f, std::min(20.0f, tpsDistance));
        } else {
            fov -= delta * 2.0f;
            fov = std::max(10.0f, std::min(120.0f, fov));
        }
    }
    
    /** TPS模式更新跟随（每帧调用） */
    void UpdateTPS(float dt) {
        if (mode != CameraMode::TPS) return;
        Vec3 target = GetTPSIdealPosition();
        if (enableSmoothing) {
            float t = 1.0f - std::exp(-tpsSmooth * dt);
            m_currentPos = m_currentPos + (target - m_currentPos) * t;
        } else {
            m_currentPos = target;
        }
    }
    
    /** 切换模式时初始化位置 */
    void SwitchMode(CameraMode newMode) {
        if (newMode == mode) return;
        if (newMode == CameraMode::FPS) {
            fpsPosition = GetPosition();
        } else if (newMode == CameraMode::TPS) {
            m_currentPos = GetTPSIdealPosition();
        }
        mode = newMode;
    }
    
private:
    Vec3 m_currentPos = Vec3(0, 4, 8); // TPS平滑后的当前位置
    
    Vec3 GetOrbitEye() const {
        float yr = yaw * 0.0174533f, pr = pitch * 0.0174533f;
        return orbitTarget + Vec3(
            orbitDistance * std::cos(pr) * std::sin(yr),
            orbitDistance * std::sin(pr),
            orbitDistance * std::cos(pr) * std::cos(yr));
    }
    
    Vec3 GetTPSIdealPosition() const {
        float yr = yaw * 0.0174533f, pr = pitch * 0.0174533f;
        return tpsTarget + Vec3(
            -tpsDistance * std::cos(pr) * std::sin(yr),
            tpsHeight + tpsDistance * std::sin(pr) * 0.3f,
            -tpsDistance * std::cos(pr) * std::cos(yr));
    }
};

} // namespace MiniEngine
