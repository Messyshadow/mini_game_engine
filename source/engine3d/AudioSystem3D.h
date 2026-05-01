#pragma once
#include "fmod.h"
#include "math/Math.h"
#include <string>
#include <unordered_map>
#include <iostream>

namespace MiniEngine {

using Math::Vec3;

class AudioSystem3D {
public:
    bool Init() {
        FMOD_RESULT result = FMOD_System_Create(&m_system);
        if (result != FMOD_OK) { std::cerr << "FMOD create failed" << std::endl; return false; }
        result = FMOD_System_Init(m_system, 512, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) { std::cerr << "FMOD init failed" << std::endl; return false; }
        std::cout << "[FMOD] Initialized" << std::endl;
        return true;
    }

    void Shutdown() {
        for (auto& pair : m_sounds) { FMOD_Sound_Release(pair.second); }
        m_sounds.clear();
        if (m_system) { FMOD_System_Release(m_system); m_system = nullptr; }
    }

    void Update() {
        if (m_system) FMOD_System_Update(m_system);
    }

    bool LoadSound(const std::string& name, const std::string& path, bool is3D = false, bool loop = false) {
        FMOD_MODE mode = FMOD_DEFAULT;
        if (is3D) mode |= FMOD_3D;
        if (loop) mode |= FMOD_LOOP_NORMAL;

        FMOD_SOUND* sound = nullptr;
        FMOD_RESULT result = FMOD_System_CreateSound(m_system, path.c_str(), mode, nullptr, &sound);
        if (result != FMOD_OK) {
            std::string altPaths[] = {"../" + path, "../../" + path};
            for (auto& p : altPaths) {
                result = FMOD_System_CreateSound(m_system, p.c_str(), mode, nullptr, &sound);
                if (result == FMOD_OK) break;
            }
        }
        if (result != FMOD_OK) { std::cerr << "[FMOD] Failed to load: " << path << std::endl; return false; }
        m_sounds[name] = sound;
        std::cout << "[FMOD] Loaded: " << name << " (" << path << ")" << std::endl;
        return true;
    }

    void PlaySound(const std::string& name, float volume = 1.0f) {
        auto it = m_sounds.find(name);
        if (it == m_sounds.end()) return;
        FMOD_CHANNEL* channel = nullptr;
        FMOD_System_PlaySound(m_system, it->second, nullptr, false, &channel);
        if (channel) FMOD_Channel_SetVolume(channel, volume * m_sfxVolume * m_masterVolume);
    }

    void PlaySound3D(const std::string& name, Vec3 pos, float volume = 1.0f) {
        auto it = m_sounds.find(name);
        if (it == m_sounds.end()) return;
        FMOD_CHANNEL* channel = nullptr;
        FMOD_System_PlaySound(m_system, it->second, nullptr, false, &channel);
        if (channel) {
            FMOD_VECTOR fpos = {pos.x, pos.y, pos.z};
            FMOD_VECTOR fvel = {0, 0, 0};
            FMOD_Channel_Set3DAttributes(channel, &fpos, &fvel);
            FMOD_Channel_SetVolume(channel, volume * m_sfxVolume * m_masterVolume);
        }
    }

    void PlayBGM(const std::string& name, float volume = 0.5f) {
        StopBGM();
        auto it = m_sounds.find(name);
        if (it == m_sounds.end()) return;
        FMOD_System_PlaySound(m_system, it->second, nullptr, false, &m_bgmChannel);
        if (m_bgmChannel) FMOD_Channel_SetVolume(m_bgmChannel, volume * m_bgmVolume * m_masterVolume);
        m_bgmBaseVol = volume;
    }

    void StopBGM() {
        if (m_bgmChannel) { FMOD_Channel_Stop(m_bgmChannel); m_bgmChannel = nullptr; }
    }

    void SetBGMVolume(float vol) {
        m_bgmVolume = vol;
        if (m_bgmChannel) FMOD_Channel_SetVolume(m_bgmChannel, m_bgmBaseVol * m_bgmVolume * m_masterVolume);
    }

    void SetSFXVolume(float vol) { m_sfxVolume = vol; }
    void SetMasterVolume(float vol) {
        m_masterVolume = vol;
        if (m_bgmChannel) FMOD_Channel_SetVolume(m_bgmChannel, m_bgmBaseVol * m_bgmVolume * m_masterVolume);
    }

    float GetMasterVolume() const { return m_masterVolume; }
    float GetBGMVolume() const { return m_bgmVolume; }
    float GetSFXVolume() const { return m_sfxVolume; }

    void SetListenerPosition(Vec3 pos, Vec3 forward, Vec3 up) {
        FMOD_VECTOR fpos = {pos.x, pos.y, pos.z};
        FMOD_VECTOR fvel = {0, 0, 0};
        FMOD_VECTOR ffwd = {forward.x, forward.y, forward.z};
        FMOD_VECTOR fup = {up.x, up.y, up.z};
        FMOD_System_Set3DListenerAttributes(m_system, 0, &fpos, &fvel, &ffwd, &fup);
    }

    void Set3DSettings(float dopplerScale, float distanceFactor, float rolloffScale) {
        if (m_system) FMOD_System_Set3DSettings(m_system, dopplerScale, distanceFactor, rolloffScale);
    }

private:
    FMOD_SYSTEM* m_system = nullptr;
    FMOD_CHANNEL* m_bgmChannel = nullptr;
    std::unordered_map<std::string, FMOD_SOUND*> m_sounds;
    float m_masterVolume = 1.0f;
    float m_bgmVolume = 1.0f;
    float m_sfxVolume = 1.0f;
    float m_bgmBaseVol = 0.5f;
};

} // namespace MiniEngine
