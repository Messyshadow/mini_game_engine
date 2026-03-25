/**
 * AudioSystem.h - 音频系统
 * 
 * 使用 miniaudio 库实现音频播放
 * 
 * 功能：
 * - 播放背景音乐（BGM）- 循环播放
 * - 播放音效（SFX）- 单次播放
 * - 音量控制
 * - 暂停/恢复
 */

#ifndef MINI_ENGINE_AUDIO_SYSTEM_H
#define MINI_ENGINE_AUDIO_SYSTEM_H

#include <string>
#include <unordered_map>
#include <memory>

namespace MiniEngine {

/**
 * 音频系统类
 * 
 * 使用示例：
 *   AudioSystem audio;
 *   audio.Initialize();
 *   audio.LoadSound("jump", "data/audio/sfx/jump.wav");
 *   audio.LoadMusic("bgm", "data/audio/bgm/music.mp3");
 *   audio.PlayMusic("bgm");
 *   audio.PlaySound("jump");
 */
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();
    
    // 禁止拷贝
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    
    /**
     * 初始化音频系统
     * @return 成功返回true
     */
    bool Initialize();
    
    /**
     * 关闭音频系统
     */
    void Shutdown();
    
    /**
     * 是否已初始化
     */
    bool IsInitialized() const { return m_initialized; }
    
    // ========================================================================
    // 音效（SFX）- 短音效，可同时播放多个
    // ========================================================================
    
    /**
     * 加载音效
     * @param name 音效名称（用于后续播放）
     * @param filepath 文件路径（支持wav, mp3, flac等）
     * @return 成功返回true
     */
    bool LoadSound(const std::string& name, const std::string& filepath);
    
    /**
     * 播放音效
     * @param name 音效名称
     * @param volume 音量 0.0-1.0
     */
    void PlaySFX(const std::string& name, float volume = 1.0f);
    
    // ========================================================================
    // 背景音乐（BGM）- 同时只能播放一首
    // ========================================================================
    
    /**
     * 加载背景音乐
     * @param name 音乐名称
     * @param filepath 文件路径
     * @return 成功返回true
     */
    bool LoadMusic(const std::string& name, const std::string& filepath);
    
    /**
     * 播放背景音乐（循环）
     * @param name 音乐名称
     * @param volume 音量 0.0-1.0
     */
    void PlayMusic(const std::string& name, float volume = 0.5f);
    
    /**
     * 停止背景音乐
     */
    void StopMusic();
    
    /**
     * 暂停/恢复背景音乐
     */
    void PauseMusic();
    void ResumeMusic();
    
    /**
     * 背景音乐是否正在播放
     */
    bool IsMusicPlaying() const;
    
    // ========================================================================
    // 音量控制
    // ========================================================================
    
    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return m_masterVolume; }
    
    void SetMusicVolume(float volume);
    float GetMusicVolume() const { return m_musicVolume; }
    
    void SetSFXVolume(float volume);
    float GetSFXVolume() const { return m_sfxVolume; }
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    
    bool m_initialized = false;
    float m_masterVolume = 1.0f;
    float m_musicVolume = 0.5f;
    float m_sfxVolume = 1.0f;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_AUDIO_SYSTEM_H
