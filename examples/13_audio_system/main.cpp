/**
 * 示例13：音频系统 (Audio System)
 * 
 * 学习目标：
 * - 使用miniaudio播放音频
 * - 背景音乐（BGM）循环播放
 * - 音效（SFX）触发播放
 * - 音量控制
 * 
 * 操作：
 * - M: 播放/停止背景音乐
 * - 1: 播放斧头音效
 * - 2: 播放弓箭音效
 * - 3: 播放爆炸音效
 * - 上/下方向键: 调节主音量
 * - 左/右方向键: 调节音乐音量
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "audio/AudioSystem.h"
#include "math/Math.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// 全局变量
std::unique_ptr<Renderer2D> g_renderer;
std::unique_ptr<AudioSystem> g_audio;

float g_masterVolume = 1.0f;
float g_musicVolume = 0.5f;
float g_sfxVolume = 1.0f;
bool g_musicPlaying = false;

bool Initialize() {
    // 初始化渲染器
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    // 初始化音频系统
    g_audio = std::make_unique<AudioSystem>();
    if (!g_audio->Initialize()) {
        std::cerr << "Failed to initialize audio system" << std::endl;
        return false;
    }
    
    // 加载背景音乐
    const char* bgmPaths[] = {
        "data/audio/bgm/windaswarriors.mp3",
        "../data/audio/bgm/windaswarriors.mp3"
    };
    for (auto path : bgmPaths) {
        if (g_audio->LoadMusic("bgm", path)) break;
    }
    
    // 加载音效
    const char* sfxPaths[][2] = {
        {"axe", "data/audio/sfx/Sound_Axe.wav"},
        {"axe", "../data/audio/sfx/Sound_Axe.wav"},
        {"bow", "data/audio/sfx/Sound_Bow.wav"},
        {"bow", "../data/audio/sfx/Sound_Bow.wav"},
        {"bomb", "data/audio/sfx/bigBomb.wav"},
        {"bomb", "../data/audio/sfx/bigBomb.wav"},
    };
    
    for (int i = 0; i < 6; i += 2) {
        if (!g_audio->LoadSound(sfxPaths[i][0], sfxPaths[i][1])) {
            g_audio->LoadSound(sfxPaths[i+1][0], sfxPaths[i+1][1]);
        }
    }
    
    std::cout << "\n=== Audio System Ready ===" << std::endl;
    std::cout << "M: Toggle BGM" << std::endl;
    std::cout << "1: Axe sound" << std::endl;
    std::cout << "2: Bow sound" << std::endl;
    std::cout << "3: Bomb sound" << std::endl;
    std::cout << "Up/Down: Master volume" << std::endl;
    std::cout << "Left/Right: Music volume" << std::endl;
    
    return true;
}

void Update(float dt) {
    Engine& engine = Engine::Instance();
    GLFWwindow* w = engine.GetWindow();
    
    // 按键状态（防止重复触发）
    static bool mLast = false, k1Last = false, k2Last = false, k3Last = false;
    static bool upLast = false, downLast = false, leftLast = false, rightLast = false;
    
    bool mKey = glfwGetKey(w, GLFW_KEY_M) == GLFW_PRESS;
    bool k1 = glfwGetKey(w, GLFW_KEY_1) == GLFW_PRESS;
    bool k2 = glfwGetKey(w, GLFW_KEY_2) == GLFW_PRESS;
    bool k3 = glfwGetKey(w, GLFW_KEY_3) == GLFW_PRESS;
    bool up = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS;
    bool down = glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS;
    bool left = glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS;
    bool right = glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS;
    
    // M键：切换背景音乐
    if (mKey && !mLast) {
        if (g_musicPlaying) {
            g_audio->StopMusic();
            g_musicPlaying = false;
            std::cout << "[Audio] BGM Stopped" << std::endl;
        } else {
            g_audio->PlayMusic("bgm", g_musicVolume);
            g_musicPlaying = true;
            std::cout << "[Audio] BGM Playing" << std::endl;
        }
    }
    
    // 1/2/3键：播放音效
    if (k1 && !k1Last) {
        g_audio->PlaySFX("axe", g_sfxVolume);
        std::cout << "[Audio] Axe!" << std::endl;
    }
    if (k2 && !k2Last) {
        g_audio->PlaySFX("bow", g_sfxVolume);
        std::cout << "[Audio] Bow!" << std::endl;
    }
    if (k3 && !k3Last) {
        g_audio->PlaySFX("bomb", g_sfxVolume);
        std::cout << "[Audio] Bomb!" << std::endl;
    }
    
    // 方向键：调节音量
    if (up && !upLast) {
        g_masterVolume = std::min(1.0f, g_masterVolume + 0.1f);
        g_audio->SetMasterVolume(g_masterVolume);
        std::cout << "[Audio] Master Volume: " << (int)(g_masterVolume * 100) << "%" << std::endl;
    }
    if (down && !downLast) {
        g_masterVolume = std::max(0.0f, g_masterVolume - 0.1f);
        g_audio->SetMasterVolume(g_masterVolume);
        std::cout << "[Audio] Master Volume: " << (int)(g_masterVolume * 100) << "%" << std::endl;
    }
    if (right && !rightLast) {
        g_musicVolume = std::min(1.0f, g_musicVolume + 0.1f);
        g_audio->SetMusicVolume(g_musicVolume);
        std::cout << "[Audio] Music Volume: " << (int)(g_musicVolume * 100) << "%" << std::endl;
    }
    if (left && !leftLast) {
        g_musicVolume = std::max(0.0f, g_musicVolume - 0.1f);
        g_audio->SetMusicVolume(g_musicVolume);
        std::cout << "[Audio] Music Volume: " << (int)(g_musicVolume * 100) << "%" << std::endl;
    }
    
    // 更新按键状态
    mLast = mKey;
    k1Last = k1; k2Last = k2; k3Last = k3;
    upLast = up; downLast = down; leftLast = left; rightLast = right;
}

void Render() {
    // 简单的渐变背景
    Engine& engine = Engine::Instance();
    int sw = engine.GetWindowWidth();
    int sh = engine.GetWindowHeight();
    
    // 根据音乐播放状态改变背景颜色
    if (g_musicPlaying) {
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);  // 深蓝色
    } else {
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);  // 灰色
    }
    glClear(GL_COLOR_BUFFER_BIT);
}

void RenderImGui() {
    ImGui::Begin("Audio System", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("=== Controls ===");
    ImGui::BulletText("M - Toggle BGM");
    ImGui::BulletText("1 - Axe sound");
    ImGui::BulletText("2 - Bow sound");
    ImGui::BulletText("3 - Bomb sound");
    ImGui::BulletText("Up/Down - Master volume");
    ImGui::BulletText("Left/Right - Music volume");
    
    ImGui::Separator();
    ImGui::Text("=== Status ===");
    ImGui::Text("BGM: %s", g_musicPlaying ? "Playing" : "Stopped");
    ImGui::Text("Audio Initialized: %s", g_audio->IsInitialized() ? "Yes" : "No");
    
    ImGui::Separator();
    ImGui::Text("=== Volume ===");
    
    // 主音量滑块
    if (ImGui::SliderFloat("Master", &g_masterVolume, 0.0f, 1.0f, "%.0f%%")) {
        g_audio->SetMasterVolume(g_masterVolume);
    }
    
    // 音乐音量滑块
    if (ImGui::SliderFloat("Music", &g_musicVolume, 0.0f, 1.0f, "%.0f%%")) {
        g_audio->SetMusicVolume(g_musicVolume);
    }
    
    // 音效音量滑块
    if (ImGui::SliderFloat("SFX", &g_sfxVolume, 0.0f, 1.0f, "%.0f%%")) {
        g_audio->SetSFXVolume(g_sfxVolume);
    }
    
    ImGui::Separator();
    
    // 音效按钮
    if (ImGui::Button("Play Axe", ImVec2(100, 30))) {
        g_audio->PlaySFX("axe", g_sfxVolume);
    }
    ImGui::SameLine();
    if (ImGui::Button("Play Bow", ImVec2(100, 30))) {
        g_audio->PlaySFX("bow", g_sfxVolume);
    }
    ImGui::SameLine();
    if (ImGui::Button("Play Bomb", ImVec2(100, 30))) {
        g_audio->PlaySFX("bomb", g_sfxVolume);
    }
    
    // BGM按钮
    if (g_musicPlaying) {
        if (ImGui::Button("Stop BGM", ImVec2(310, 30))) {
            g_audio->StopMusic();
            g_musicPlaying = false;
        }
    } else {
        if (ImGui::Button("Play BGM", ImVec2(310, 30))) {
            g_audio->PlayMusic("bgm", g_musicVolume);
            g_musicPlaying = true;
        }
    }
    
    ImGui::End();
}

void Cleanup() {
    if (g_audio) {
        g_audio->Shutdown();
        g_audio.reset();
    }
    g_renderer.reset();
}

int main() {
    std::cout << "=== Example 13: Audio System ===" << std::endl;
    
    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth = 800;
    cfg.windowHeight = 600;
    cfg.windowTitle = "Mini Engine - Audio System";
    
    if (!engine.Initialize(cfg)) {
        return -1;
    }
    
    if (!Initialize()) {
        engine.Shutdown();
        return -1;
    }
    
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    
    engine.Run();
    
    Cleanup();
    engine.Shutdown();
    
    return 0;
}
