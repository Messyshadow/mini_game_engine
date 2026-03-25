/**
 * Animator.h - 动画控制器
 * 
 * ============================================================================
 * 什么是动画控制器？
 * ============================================================================
 * 
 * 动画控制器(Animator)负责：
 * 1. 管理多个动画片段
 * 2. 处理动画之间的切换
 * 3. 更新动画播放进度
 * 4. 计算当前应该显示哪一帧
 * 
 * ============================================================================
 * 使用示例
 * ============================================================================
 * 
 *   // 创建动画器
 *   Animator animator;
 *   
 *   // 添加动画片段
 *   animator.AddClip(AnimationClip("idle", 0, 3, 12.0f, true));
 *   animator.AddClip(AnimationClip("run", 4, 11, 12.0f, true));
 *   animator.AddClip(AnimationClip("jump", 12, 15, 10.0f, false));
 *   
 *   // 设置图集信息
 *   animator.SetSpriteSheet(8, 4);  // 8列4行
 *   
 *   // 播放动画
 *   animator.Play("idle");
 *   
 *   // 每帧更新
 *   animator.Update(deltaTime);
 *   
 *   // 获取当前UV坐标
 *   Vec2 uvMin, uvMax;
 *   animator.GetCurrentUV(uvMin, uvMax);
 */

#ifndef MINI_ENGINE_ANIMATOR_H
#define MINI_ENGINE_ANIMATOR_H

#include "AnimationClip.h"
#include "math/Math.h"
#include <unordered_map>
#include <string>
#include <functional>

namespace MiniEngine {

/**
 * 动画播放状态
 */
enum class AnimationState {
    Stopped,    // 停止
    Playing,    // 播放中
    Paused      // 暂停
};

/**
 * 动画控制器
 */
class Animator {
public:
    // 回调函数类型
    using AnimationCallback = std::function<void(const std::string&)>;
    
    // ========================================================================
    // 构造和析构
    // ========================================================================
    
    Animator();
    ~Animator() = default;
    
    // ========================================================================
    // 动画片段管理
    // ========================================================================
    
    /**
     * 添加动画片段
     * 
     * @param clip 动画片段
     */
    void AddClip(const AnimationClip& clip);
    
    /**
     * 添加动画片段（简化版本）
     * 
     * @param name 动画名称
     * @param startFrame 起始帧
     * @param endFrame 结束帧
     * @param frameRate 帧率
     * @param loop 是否循环
     */
    void AddClip(const std::string& name, int startFrame, int endFrame,
                 float frameRate = 12.0f, bool loop = true);
    
    /**
     * 获取动画片段
     * 
     * @param name 动画名称
     * @return 动画片段指针，不存在返回nullptr
     */
    AnimationClip* GetClip(const std::string& name);
    const AnimationClip* GetClip(const std::string& name) const;
    
    /**
     * 检查是否有指定动画
     */
    bool HasClip(const std::string& name) const;
    
    // ========================================================================
    // 精灵图集设置
    // ========================================================================
    
    /**
     * 设置精灵图集参数
     * 
     * @param columns 列数（水平方向的帧数）
     * @param rows 行数（垂直方向的帧数）
     * 
     * 例如：一个512x256的图集，每帧64x64
     * columns = 512/64 = 8
     * rows = 256/64 = 4
     */
    void SetSpriteSheet(int columns, int rows);
    
    /**
     * 获取图集列数
     */
    int GetColumns() const { return m_columns; }
    
    /**
     * 获取图集行数
     */
    int GetRows() const { return m_rows; }
    
    // ========================================================================
    // 播放控制
    // ========================================================================
    
    /**
     * 播放指定动画
     * 
     * @param name 动画名称
     * @param restart 如果已在播放同一动画，是否重新开始
     */
    void Play(const std::string& name, bool restart = false);
    
    /**
     * 停止播放
     */
    void Stop();
    
    /**
     * 暂停播放
     */
    void Pause();
    
    /**
     * 继续播放
     */
    void Resume();
    
    /**
     * 更新动画
     * 
     * @param deltaTime 帧间隔时间（秒）
     */
    void Update(float deltaTime);
    
    // ========================================================================
    // 状态查询
    // ========================================================================
    
    /**
     * 获取当前播放状态
     */
    AnimationState GetState() const { return m_state; }
    
    /**
     * 是否正在播放
     */
    bool IsPlaying() const { return m_state == AnimationState::Playing; }
    
    /**
     * 获取当前动画名称
     */
    const std::string& GetCurrentClipName() const { return m_currentClipName; }
    
    /**
     * 获取当前帧索引（全局，在图集中的位置）
     */
    int GetCurrentFrame() const { return m_currentFrame; }
    
    /**
     * 获取当前播放时间
     */
    float GetCurrentTime() const { return m_currentTime; }
    
    /**
     * 获取当前动画进度 (0.0 - 1.0)
     */
    float GetProgress() const;
    
    /**
     * 当前动画是否播放完成（仅对非循环动画有意义）
     */
    bool IsFinished() const { return m_finished; }
    
    // ========================================================================
    // UV坐标计算
    // ========================================================================
    
    /**
     * 获取当前帧的UV坐标
     * 
     * @param uvMin 输出：UV左下角
     * @param uvMax 输出：UV右上角
     * 
     * 这个函数计算当前帧在精灵图集中的位置，
     * 然后转换为UV坐标供渲染使用。
     */
    void GetCurrentUV(Math::Vec2& uvMin, Math::Vec2& uvMax) const;
    
    /**
     * 根据帧索引获取UV坐标
     * 
     * @param frameIndex 帧索引（全局）
     * @param uvMin 输出：UV左下角
     * @param uvMax 输出：UV右上角
     */
    void GetFrameUV(int frameIndex, Math::Vec2& uvMin, Math::Vec2& uvMax) const;
    
    // ========================================================================
    // 回调设置
    // ========================================================================
    
    /**
     * 设置动画完成回调
     * 
     * 当非循环动画播放完成时调用
     * 
     * @param callback 回调函数，参数为动画名称
     */
    void SetOnComplete(AnimationCallback callback) {
        m_onComplete = callback;
    }
    
    /**
     * 设置动画切换回调
     * 
     * 当切换到新动画时调用
     * 
     * @param callback 回调函数，参数为新动画名称
     */
    void SetOnAnimationChange(AnimationCallback callback) {
        m_onAnimationChange = callback;
    }
    
    // ========================================================================
    // 播放速度
    // ========================================================================
    
    /**
     * 设置播放速度倍数
     * 
     * @param speed 速度倍数（1.0=正常，2.0=两倍速，0.5=半速）
     */
    void SetSpeed(float speed) { m_speed = speed; }
    
    /**
     * 获取播放速度
     */
    float GetSpeed() const { return m_speed; }
    
private:
    // 动画片段集合
    std::unordered_map<std::string, AnimationClip> m_clips;
    
    // 精灵图集参数
    int m_columns = 1;      // 列数
    int m_rows = 1;         // 行数
    
    // 当前播放状态
    AnimationState m_state = AnimationState::Stopped;
    std::string m_currentClipName;  // 当前动画名称
    int m_currentFrame = 0;         // 当前帧（全局索引）
    float m_currentTime = 0.0f;     // 当前播放时间
    bool m_finished = false;        // 是否播放完成
    
    // 播放速度
    float m_speed = 1.0f;
    
    // 回调
    AnimationCallback m_onComplete;
    AnimationCallback m_onAnimationChange;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_ANIMATOR_H
