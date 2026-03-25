/**
 * AudioSystem.cpp - 音频系统实现
 * 
 * 使用 miniaudio 库
 */

// 防止Windows min/max宏与std::min/max冲突
#ifdef _WIN32
#define NOMINMAX
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "AudioSystem.h"
#include <iostream>
#include <vector>
#include <algorithm>

namespace MiniEngine {

// 最大同时播放的音效数量
constexpr int MAX_SOUNDS = 16;

struct AudioSystem::Impl {
    ma_engine engine;
    bool engineInitialized = false;
    
    // 音效数据
    std::unordered_map<std::string, std::string> soundPaths;
    std::unordered_map<std::string, std::string> musicPaths;
    
    // 当前播放的音乐
    ma_sound* currentMusic = nullptr;
    std::string currentMusicName;
    
    // 音效池（用于同时播放多个音效）
    std::vector<ma_sound*> activeSounds;
};

AudioSystem::AudioSystem() : m_impl(std::make_unique<Impl>()) {
}

AudioSystem::~AudioSystem() {
    Shutdown();
}

bool AudioSystem::Initialize() {
    if (m_initialized) return true;
    
    ma_engine_config config = ma_engine_config_init();
    
    ma_result result = ma_engine_init(&config, &m_impl->engine);
    if (result != MA_SUCCESS) {
        std::cerr << "[AudioSystem] Failed to initialize audio engine: " << result << std::endl;
        return false;
    }
    
    m_impl->engineInitialized = true;
    m_initialized = true;
    
    std::cout << "[AudioSystem] Initialized successfully" << std::endl;
    return true;
}

void AudioSystem::Shutdown() {
    if (!m_initialized) return;
    
    // 停止并释放当前音乐
    if (m_impl->currentMusic) {
        ma_sound_uninit(m_impl->currentMusic);
        delete m_impl->currentMusic;
        m_impl->currentMusic = nullptr;
    }
    
    // 释放所有活动音效
    for (auto* sound : m_impl->activeSounds) {
        if (sound) {
            ma_sound_uninit(sound);
            delete sound;
        }
    }
    m_impl->activeSounds.clear();
    
    // 关闭引擎
    if (m_impl->engineInitialized) {
        ma_engine_uninit(&m_impl->engine);
        m_impl->engineInitialized = false;
    }
    
    m_initialized = false;
    std::cout << "[AudioSystem] Shutdown complete" << std::endl;
}

bool AudioSystem::LoadSound(const std::string& name, const std::string& filepath) {
    if (!m_initialized) {
        std::cerr << "[AudioSystem] Not initialized" << std::endl;
        return false;
    }
    
    // 验证文件是否存在（尝试加载一次）
    ma_sound testSound;
    ma_result result = ma_sound_init_from_file(&m_impl->engine, filepath.c_str(), 
                                                MA_SOUND_FLAG_DECODE, nullptr, nullptr, &testSound);
    if (result != MA_SUCCESS) {
        std::cerr << "[AudioSystem] Failed to load sound: " << filepath << " (error: " << result << ")" << std::endl;
        return false;
    }
    ma_sound_uninit(&testSound);
    
    // 保存路径
    m_impl->soundPaths[name] = filepath;
    std::cout << "[AudioSystem] Loaded sound: " << name << " -> " << filepath << std::endl;
    return true;
}

void AudioSystem::PlaySFX(const std::string& name, float volume) {
    if (!m_initialized) return;
    
    auto it = m_impl->soundPaths.find(name);
    if (it == m_impl->soundPaths.end()) {
        std::cerr << "[AudioSystem] Sound not found: " << name << std::endl;
        return;
    }
    
    // 清理已完成的音效
    auto& sounds = m_impl->activeSounds;
    sounds.erase(std::remove_if(sounds.begin(), sounds.end(), [](ma_sound* s) {
        if (s && !ma_sound_is_playing(s)) {
            ma_sound_uninit(s);
            delete s;
            return true;
        }
        return false;
    }), sounds.end());
    
    // 限制同时播放数量
    if (sounds.size() >= MAX_SOUNDS) {
        return;
    }
    
    // 创建新音效
    ma_sound* sound = new ma_sound();
    ma_result result = ma_sound_init_from_file(&m_impl->engine, it->second.c_str(),
                                                MA_SOUND_FLAG_DECODE, nullptr, nullptr, sound);
    if (result != MA_SUCCESS) {
        delete sound;
        return;
    }
    
    ma_sound_set_volume(sound, volume * m_sfxVolume * m_masterVolume);
    ma_sound_start(sound);
    
    sounds.push_back(sound);
}

bool AudioSystem::LoadMusic(const std::string& name, const std::string& filepath) {
    if (!m_initialized) {
        std::cerr << "[AudioSystem] Not initialized" << std::endl;
        return false;
    }
    
    // 验证文件
    ma_sound testSound;
    ma_result result = ma_sound_init_from_file(&m_impl->engine, filepath.c_str(),
                                                MA_SOUND_FLAG_STREAM, nullptr, nullptr, &testSound);
    if (result != MA_SUCCESS) {
        std::cerr << "[AudioSystem] Failed to load music: " << filepath << " (error: " << result << ")" << std::endl;
        return false;
    }
    ma_sound_uninit(&testSound);
    
    m_impl->musicPaths[name] = filepath;
    std::cout << "[AudioSystem] Loaded music: " << name << " -> " << filepath << std::endl;
    return true;
}

void AudioSystem::PlayMusic(const std::string& name, float volume) {
    if (!m_initialized) return;
    
    auto it = m_impl->musicPaths.find(name);
    if (it == m_impl->musicPaths.end()) {
        std::cerr << "[AudioSystem] Music not found: " << name << std::endl;
        return;
    }
    
    // 停止当前音乐
    StopMusic();
    
    // 创建新音乐
    m_impl->currentMusic = new ma_sound();
    ma_result result = ma_sound_init_from_file(&m_impl->engine, it->second.c_str(),
                                                MA_SOUND_FLAG_STREAM, nullptr, nullptr, m_impl->currentMusic);
    if (result != MA_SUCCESS) {
        delete m_impl->currentMusic;
        m_impl->currentMusic = nullptr;
        std::cerr << "[AudioSystem] Failed to play music: " << name << std::endl;
        return;
    }
    
    ma_sound_set_volume(m_impl->currentMusic, volume * m_musicVolume * m_masterVolume);
    ma_sound_set_looping(m_impl->currentMusic, MA_TRUE);
    ma_sound_start(m_impl->currentMusic);
    
    m_impl->currentMusicName = name;
    std::cout << "[AudioSystem] Playing music: " << name << std::endl;
}

void AudioSystem::StopMusic() {
    if (!m_initialized || !m_impl->currentMusic) return;
    
    ma_sound_stop(m_impl->currentMusic);
    ma_sound_uninit(m_impl->currentMusic);
    delete m_impl->currentMusic;
    m_impl->currentMusic = nullptr;
    m_impl->currentMusicName.clear();
}

void AudioSystem::PauseMusic() {
    if (!m_initialized || !m_impl->currentMusic) return;
    ma_sound_stop(m_impl->currentMusic);
}

void AudioSystem::ResumeMusic() {
    if (!m_initialized || !m_impl->currentMusic) return;
    ma_sound_start(m_impl->currentMusic);
}

bool AudioSystem::IsMusicPlaying() const {
    if (!m_initialized || !m_impl->currentMusic) return false;
    return ma_sound_is_playing(m_impl->currentMusic);
}

void AudioSystem::SetMasterVolume(float volume) {
    m_masterVolume = std::max(0.0f, std::min(1.0f, volume));
    ma_engine_set_volume(&m_impl->engine, m_masterVolume);
}

void AudioSystem::SetMusicVolume(float volume) {
    m_musicVolume = std::max(0.0f, std::min(1.0f, volume));
    if (m_impl->currentMusic) {
        ma_sound_set_volume(m_impl->currentMusic, m_musicVolume * m_masterVolume);
    }
}

void AudioSystem::SetSFXVolume(float volume) {
    m_sfxVolume = std::max(0.0f, std::min(1.0f, volume));
}

} // namespace MiniEngine
