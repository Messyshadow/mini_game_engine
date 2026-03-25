#include "Camera2D.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace MiniEngine {

Camera2D::Camera2D() : m_position(0, 0), m_target(0, 0), m_viewportSize(1280, 720) {}

void Camera2D::Update(float deltaTime) {
    Math::Vec2 desiredPos = CalculateTargetPosition();
    if (m_smoothSpeed > 0) {
        float t = std::min(m_smoothSpeed * deltaTime, 1.0f);
        m_position.x += (desiredPos.x - m_position.x) * t;
        m_position.y += (desiredPos.y - m_position.y) * t;
    } else {
        m_position = desiredPos;
    }
    if (m_hasBounds) ApplyBounds();
    if (m_shakeTimer > 0) UpdateShake(deltaTime);
}

Math::Vec2 Camera2D::CalculateTargetPosition() const {
    return Math::Vec2(m_target.x - m_viewportSize.x * 0.5f / m_zoom,
                      m_target.y - m_viewportSize.y * 0.5f / m_zoom);
}

void Camera2D::SnapToTarget() {
    m_position = CalculateTargetPosition();
    if (m_hasBounds) ApplyBounds();
}

void Camera2D::SetViewportSize(float w, float h) { m_viewportSize = Math::Vec2(w, h); }
void Camera2D::SetViewportSize(const Math::Vec2& s) { m_viewportSize = s; }

void Camera2D::SetWorldBounds(float minX, float minY, float maxX, float maxY) {
    m_boundsMin = Math::Vec2(minX, minY);
    m_boundsMax = Math::Vec2(maxX, maxY);
    m_hasBounds = true;
}

void Camera2D::SetWorldBounds(const Math::Vec2& min, const Math::Vec2& max) {
    m_boundsMin = min; m_boundsMax = max; m_hasBounds = true;
}

void Camera2D::ApplyBounds() {
    float viewW = m_viewportSize.x / m_zoom, viewH = m_viewportSize.y / m_zoom;
    float maxCamX = m_boundsMax.x - viewW, maxCamY = m_boundsMax.y - viewH;
    m_position.x = (maxCamX < m_boundsMin.x) ? (m_boundsMin.x + m_boundsMax.x - viewW) * 0.5f
                   : std::max(m_boundsMin.x, std::min(m_position.x, maxCamX));
    m_position.y = (maxCamY < m_boundsMin.y) ? (m_boundsMin.y + m_boundsMax.y - viewH) * 0.5f
                   : std::max(m_boundsMin.y, std::min(m_position.y, maxCamY));
}

void Camera2D::Shake(float intensity, float duration) {
    m_shakeIntensity = intensity; m_shakeTimer = duration;
}

void Camera2D::UpdateShake(float dt) {
    m_shakeTimer -= dt;
    if (m_shakeTimer > 0) {
        m_shakeOffset = Math::Vec2(((float)rand()/RAND_MAX*2-1)*m_shakeIntensity,
                                   ((float)rand()/RAND_MAX*2-1)*m_shakeIntensity);
        m_shakeIntensity *= 0.9f;
    } else { m_shakeOffset = Math::Vec2::Zero(); m_shakeTimer = 0; }
}

Math::Vec2 Camera2D::WorldToScreen(const Math::Vec2& w) const {
    return Math::Vec2((w.x-m_position.x-m_shakeOffset.x)*m_zoom, (w.y-m_position.y-m_shakeOffset.y)*m_zoom);
}

Math::Vec2 Camera2D::ScreenToWorld(const Math::Vec2& s) const {
    return Math::Vec2(s.x/m_zoom+m_position.x+m_shakeOffset.x, s.y/m_zoom+m_position.y+m_shakeOffset.y);
}

Math::Mat4 Camera2D::GetViewMatrix() const {
    Math::Vec2 offset = m_position + m_shakeOffset;
    return Math::Mat4::Translate(-offset.x, -offset.y, 0);
}

} // namespace MiniEngine
