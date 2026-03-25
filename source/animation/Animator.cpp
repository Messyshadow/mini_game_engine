/**
 * Animator.cpp - 动画控制器实现
 */

#include "Animator.h"
#include <iostream>
#include <algorithm>

namespace MiniEngine {

// ============================================================================
// 构造函数
// ============================================================================

Animator::Animator()
    : m_columns(1)
    , m_rows(1)
    , m_state(AnimationState::Stopped)
    , m_currentFrame(0)
    , m_currentTime(0.0f)
    , m_finished(false)
    , m_speed(1.0f)
{
}

// ============================================================================
// 动画片段管理
// ============================================================================

void Animator::AddClip(const AnimationClip& clip) {
    m_clips[clip.GetName()] = clip;
}

void Animator::AddClip(const std::string& name, int startFrame, int endFrame,
                       float frameRate, bool loop) {
    m_clips[name] = AnimationClip(name, startFrame, endFrame, frameRate, loop);
}

AnimationClip* Animator::GetClip(const std::string& name) {
    auto it = m_clips.find(name);
    if (it != m_clips.end()) {
        return &it->second;
    }
    return nullptr;
}

const AnimationClip* Animator::GetClip(const std::string& name) const {
    auto it = m_clips.find(name);
    if (it != m_clips.end()) {
        return &it->second;
    }
    return nullptr;
}

bool Animator::HasClip(const std::string& name) const {
    return m_clips.find(name) != m_clips.end();
}

// ============================================================================
// 精灵图集设置
// ============================================================================

void Animator::SetSpriteSheet(int columns, int rows) {
    m_columns = std::max(1, columns);
    m_rows = std::max(1, rows);
}

// ============================================================================
// 播放控制
// ============================================================================

void Animator::Play(const std::string& name, bool restart) {
    // 检查动画是否存在
    if (!HasClip(name)) {
        std::cerr << "[Animator] Animation not found: " << name << std::endl;
        return;
    }
    
    // 如果已经在播放同一动画且不需要重启
    if (m_currentClipName == name && m_state == AnimationState::Playing && !restart) {
        return;
    }
    
    // 切换到新动画
    m_currentClipName = name;
    m_currentTime = 0.0f;
    m_finished = false;
    m_state = AnimationState::Playing;
    
    // 更新当前帧
    const AnimationClip* clip = GetClip(name);
    if (clip) {
        m_currentFrame = clip->GetStartFrame();
    }
    
    // 触发回调
    if (m_onAnimationChange) {
        m_onAnimationChange(name);
    }
}

void Animator::Stop() {
    m_state = AnimationState::Stopped;
    m_currentTime = 0.0f;
    m_finished = false;
    
    // 重置到第一帧
    const AnimationClip* clip = GetClip(m_currentClipName);
    if (clip) {
        m_currentFrame = clip->GetStartFrame();
    }
}

void Animator::Pause() {
    if (m_state == AnimationState::Playing) {
        m_state = AnimationState::Paused;
    }
}

void Animator::Resume() {
    if (m_state == AnimationState::Paused) {
        m_state = AnimationState::Playing;
    }
}

// ============================================================================
// 更新
// ============================================================================

void Animator::Update(float deltaTime) {
    // 只有在播放状态才更新
    if (m_state != AnimationState::Playing) {
        return;
    }
    
    // 获取当前动画片段
    const AnimationClip* clip = GetClip(m_currentClipName);
    if (!clip) {
        return;
    }
    
    // 如果已完成且不循环，不再更新
    if (m_finished && !clip->IsLooping()) {
        return;
    }
    
    // 更新时间（应用速度倍数）
    m_currentTime += deltaTime * m_speed;
    
    // 计算当前帧
    float frameDuration = clip->GetFrameDuration();
    if (frameDuration <= 0.0f) {
        return;
    }
    
    // 计算本地帧索引
    int localFrame = static_cast<int>(m_currentTime / frameDuration);
    int frameCount = clip->GetFrameCount();
    
    if (clip->IsLooping()) {
        // 循环动画
        localFrame = localFrame % frameCount;
        m_currentFrame = clip->GetStartFrame() + localFrame;
        
        // 保持时间在合理范围内，防止浮点数溢出
        float totalDuration = clip->GetDuration();
        if (m_currentTime >= totalDuration) {
            m_currentTime = std::fmod(m_currentTime, totalDuration);
        }
    } else {
        // 非循环动画
        if (localFrame >= frameCount) {
            // 播放完成
            localFrame = frameCount - 1;
            m_currentFrame = clip->GetStartFrame() + localFrame;
            
            if (!m_finished) {
                m_finished = true;
                
                // 触发完成回调
                if (m_onComplete) {
                    m_onComplete(m_currentClipName);
                }
            }
        } else {
            m_currentFrame = clip->GetStartFrame() + localFrame;
        }
    }
}

// ============================================================================
// 状态查询
// ============================================================================

float Animator::GetProgress() const {
    const AnimationClip* clip = GetClip(m_currentClipName);
    if (!clip) {
        return 0.0f;
    }
    
    float duration = clip->GetDuration();
    if (duration <= 0.0f) {
        return 0.0f;
    }
    
    if (clip->IsLooping()) {
        return std::fmod(m_currentTime, duration) / duration;
    } else {
        return std::min(m_currentTime / duration, 1.0f);
    }
}

// ============================================================================
// UV坐标计算
// ============================================================================

void Animator::GetCurrentUV(Math::Vec2& uvMin, Math::Vec2& uvMax) const {
    GetFrameUV(m_currentFrame, uvMin, uvMax);
}

void Animator::GetFrameUV(int frameIndex, Math::Vec2& uvMin, Math::Vec2& uvMax) const {
    /**
     * UV坐标计算说明：
     * 
     * 精灵图集布局（假设8列4行）：
     * 
     *   帧0  帧1  帧2  帧3  帧4  帧5  帧6  帧7     ← 行0 (Y=最上方)
     *   帧8  帧9  帧10 帧11 帧12 帧13 帧14 帧15    ← 行1
     *   帧16 帧17 帧18 帧19 帧20 帧21 帧22 帧23    ← 行2
     *   帧24 帧25 帧26 帧27 帧28 帧29 帧30 帧31    ← 行3 (Y=最下方)
     * 
     * 但UV坐标的Y轴是从下往上的：
     *   V=1.0 ─────────────────── 顶部
     *   
     *   V=0.0 ─────────────────── 底部
     * 
     * 所以需要翻转Y坐标！
     */
    
    // 防止除零
    if (m_columns <= 0 || m_rows <= 0) {
        uvMin = Math::Vec2(0.0f, 0.0f);
        uvMax = Math::Vec2(1.0f, 1.0f);
        return;
    }
    
    // 计算帧在图集中的位置
    int col = frameIndex % m_columns;           // 列 (0 到 columns-1)
    int row = frameIndex / m_columns;           // 行 (0 到 rows-1)
    
    // 计算每个帧的UV尺寸
    float frameWidth = 1.0f / static_cast<float>(m_columns);
    float frameHeight = 1.0f / static_cast<float>(m_rows);
    
    // 计算UV坐标
    // 注意：图片的行0在顶部，但UV的V=0在底部
    // 所以需要翻转：UV_row = (rows - 1) - row
    
    float u0 = col * frameWidth;
    float u1 = (col + 1) * frameWidth;
    
    // Y轴翻转
    int flippedRow = (m_rows - 1) - row;
    float v0 = flippedRow * frameHeight;
    float v1 = (flippedRow + 1) * frameHeight;
    
    uvMin = Math::Vec2(u0, v0);
    uvMax = Math::Vec2(u1, v1);
}

} // namespace MiniEngine
