/**
 * AnimationClip.h - 动画片段
 * 
 * ============================================================================
 * 什么是动画片段？
 * ============================================================================
 * 
 * 动画片段(AnimationClip)代表一个完整的动画序列，比如：
 * - "idle" (待机动画)
 * - "run" (跑步动画)  
 * - "jump" (跳跃动画)
 * - "attack" (攻击动画)
 * 
 * 每个片段包含：
 * - 起始帧和结束帧
 * - 播放速度（帧率）
 * - 是否循环
 * - 播放完成后的回调
 * 
 * ============================================================================
 * 使用示例
 * ============================================================================
 * 
 *   // 创建一个4帧的待机动画，12FPS，循环播放
 *   AnimationClip idle("idle", 0, 3, 12.0f, true);
 *   
 *   // 创建一个6帧的攻击动画，不循环
 *   AnimationClip attack("attack", 12, 17, 15.0f, false);
 */

#ifndef MINI_ENGINE_ANIMATION_CLIP_H
#define MINI_ENGINE_ANIMATION_CLIP_H

#include <string>
#include <functional>

namespace MiniEngine {

/**
 * 动画片段类
 * 
 * 定义一个动画序列的属性
 */
class AnimationClip {
public:
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    /**
     * 默认构造
     */
    AnimationClip()
        : m_name("unnamed")
        , m_startFrame(0)
        , m_endFrame(0)
        , m_frameRate(12.0f)
        , m_loop(true)
    {}
    
    /**
     * 完整构造
     * 
     * @param name 动画名称（用于查找）
     * @param startFrame 起始帧索引（在精灵图集中的位置）
     * @param endFrame 结束帧索引（包含）
     * @param frameRate 播放帧率（帧/秒）
     * @param loop 是否循环播放
     */
    AnimationClip(const std::string& name, int startFrame, int endFrame, 
                  float frameRate = 12.0f, bool loop = true)
        : m_name(name)
        , m_startFrame(startFrame)
        , m_endFrame(endFrame)
        , m_frameRate(frameRate)
        , m_loop(loop)
    {}
    
    // ========================================================================
    // 属性访问
    // ========================================================================
    
    /** 获取动画名称 */
    const std::string& GetName() const { return m_name; }
    
    /** 获取起始帧 */
    int GetStartFrame() const { return m_startFrame; }
    
    /** 获取结束帧 */
    int GetEndFrame() const { return m_endFrame; }
    
    /** 获取总帧数 */
    int GetFrameCount() const { return m_endFrame - m_startFrame + 1; }
    
    /** 获取帧率 */
    float GetFrameRate() const { return m_frameRate; }
    
    /** 获取每帧持续时间（秒） */
    float GetFrameDuration() const { 
        return m_frameRate > 0.0f ? 1.0f / m_frameRate : 0.0f; 
    }
    
    /** 获取动画总时长（秒） */
    float GetDuration() const {
        return GetFrameCount() * GetFrameDuration();
    }
    
    /** 是否循环播放 */
    bool IsLooping() const { return m_loop; }
    
    // ========================================================================
    // 属性设置
    // ========================================================================
    
    /** 设置动画名称 */
    void SetName(const std::string& name) { m_name = name; }
    
    /** 设置帧范围 */
    void SetFrameRange(int start, int end) {
        m_startFrame = start;
        m_endFrame = end;
    }
    
    /** 设置帧率 */
    void SetFrameRate(float fps) { m_frameRate = fps; }
    
    /** 设置是否循环 */
    void SetLooping(bool loop) { m_loop = loop; }
    
    // ========================================================================
    // 帧计算
    // ========================================================================
    
    /**
     * 根据本地帧索引获取全局帧索引
     * 
     * 本地帧索引：0, 1, 2, ... (相对于动画片段)
     * 全局帧索引：startFrame, startFrame+1, ... (在精灵图集中的位置)
     * 
     * @param localFrame 本地帧索引 (0 到 frameCount-1)
     * @return 全局帧索引
     */
    int GetGlobalFrame(int localFrame) const {
        // 确保在有效范围内
        int frameCount = GetFrameCount();
        if (frameCount <= 0) return m_startFrame;
        
        // 处理循环
        if (m_loop) {
            localFrame = localFrame % frameCount;
            if (localFrame < 0) localFrame += frameCount;
        } else {
            // 不循环时，钳制到范围内
            if (localFrame < 0) localFrame = 0;
            if (localFrame >= frameCount) localFrame = frameCount - 1;
        }
        
        return m_startFrame + localFrame;
    }
    
    /**
     * 根据时间获取当前帧
     * 
     * @param time 从动画开始经过的时间（秒）
     * @return 全局帧索引
     */
    int GetFrameAtTime(float time) const {
        if (m_frameRate <= 0.0f) return m_startFrame;
        
        // 计算本地帧
        int localFrame = static_cast<int>(time * m_frameRate);
        
        return GetGlobalFrame(localFrame);
    }

private:
    std::string m_name;     // 动画名称
    int m_startFrame;       // 起始帧（在图集中的索引）
    int m_endFrame;         // 结束帧（包含）
    float m_frameRate;      // 帧率（帧/秒）
    bool m_loop;            // 是否循环
};

} // namespace MiniEngine

#endif // MINI_ENGINE_ANIMATION_CLIP_H
