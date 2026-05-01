# 第29章开发规范：UI与音频 + FMOD

## 必读：先读 PROJECT_CONTEXT.md、ROADMAP_CH26_30.md、LOCAL_ASSETS.md

## 第29章三大核心

### A. FMOD音频系统（替换miniaudio）
### B. 3D空间音效
### C. 游戏UI系统（HUD + 伤害数字 + 血条）

---

## A. FMOD音频系统

### FMOD库位置（已在项目中）
- 头文件：depends/fmod/core/inc/fmod.h
- 库文件：depends/fmod/core/lib/fmod_vc.lib
- DLL：depends/fmod/core/lib/fmod.dll

### CMake配置

```cmake
# FMOD头文件
include_directories("depends/fmod/core/inc")

# 链接（给29章target）
target_link_libraries(29_ui_audio
    "${CMAKE_CURRENT_SOURCE_DIR}/depends/fmod/core/lib/fmod_vc.lib")

# DLL拷贝
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/depends/fmod/core/lib/fmod.dll"
     DESTINATION "${CMAKE_BINARY_DIR}/Debug")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/depends/fmod/core/lib/fmod.dll"
     DESTINATION "${CMAKE_BINARY_DIR}/Release")
```

### 需要创建的文件
- `source/engine3d/AudioSystem3D.h` — FMOD音频封装

### AudioSystem3D 设计

```cpp
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
        FMOD_RESULT result = FMOD_System_Create(&m_system, FMOD_VERSION);
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
    
    // 加载音效
    bool LoadSound(const std::string& name, const std::string& path, bool is3D = false, bool loop = false) {
        FMOD_MODE mode = FMOD_DEFAULT;
        if (is3D) mode |= FMOD_3D;
        if (loop) mode |= FMOD_LOOP_NORMAL;
        
        FMOD_SOUND* sound = nullptr;
        FMOD_RESULT result = FMOD_System_CreateSound(m_system, path.c_str(), mode, nullptr, &sound);
        if (result != FMOD_OK) {
            // 尝试备用路径
            std::string altPaths[] = {"../"+path, "../../"+path};
            for (auto& p : altPaths) {
                result = FMOD_System_CreateSound(m_system, p.c_str(), mode, nullptr, &sound);
                if (result == FMOD_OK) break;
            }
        }
        if (result != FMOD_OK) { std::cerr << "[FMOD] Failed to load: " << path << std::endl; return false; }
        m_sounds[name] = sound;
        return true;
    }
    
    // 播放2D音效
    void PlaySound(const std::string& name, float volume = 1.0f) {
        auto it = m_sounds.find(name);
        if (it == m_sounds.end()) return;
        FMOD_CHANNEL* channel = nullptr;
        FMOD_System_PlaySound(m_system, it->second, nullptr, false, &channel);
        if (channel) FMOD_Channel_SetVolume(channel, volume);
    }
    
    // 播放3D音效
    void PlaySound3D(const std::string& name, Vec3 pos, float volume = 1.0f) {
        auto it = m_sounds.find(name);
        if (it == m_sounds.end()) return;
        FMOD_CHANNEL* channel = nullptr;
        FMOD_System_PlaySound(m_system, it->second, nullptr, false, &channel);
        if (channel) {
            FMOD_VECTOR fpos = {pos.x, pos.y, pos.z};
            FMOD_VECTOR fvel = {0, 0, 0};
            FMOD_Channel_Set3DAttributes(channel, &fpos, &fvel);
            FMOD_Channel_SetVolume(channel, volume);
        }
    }
    
    // 播放BGM（循环）
    void PlayBGM(const std::string& name, float volume = 0.5f) {
        StopBGM();
        auto it = m_sounds.find(name);
        if (it == m_sounds.end()) return;
        FMOD_System_PlaySound(m_system, it->second, nullptr, false, &m_bgmChannel);
        if (m_bgmChannel) FMOD_Channel_SetVolume(m_bgmChannel, volume);
    }
    
    void StopBGM() {
        if (m_bgmChannel) { FMOD_Channel_Stop(m_bgmChannel); m_bgmChannel = nullptr; }
    }
    
    void SetBGMVolume(float vol) {
        if (m_bgmChannel) FMOD_Channel_SetVolume(m_bgmChannel, vol);
    }
    
    // 设置3D听者位置（每帧调用，传入摄像机位置和朝向）
    void SetListenerPosition(Vec3 pos, Vec3 forward, Vec3 up) {
        FMOD_VECTOR fpos = {pos.x, pos.y, pos.z};
        FMOD_VECTOR fvel = {0, 0, 0};
        FMOD_VECTOR ffwd = {forward.x, forward.y, forward.z};
        FMOD_VECTOR fup = {up.x, up.y, up.z};
        FMOD_System_Set3DListenerAttributes(m_system, 0, &fpos, &fvel, &ffwd, &fup);
    }

private:
    FMOD_SYSTEM* m_system = nullptr;
    FMOD_CHANNEL* m_bgmChannel = nullptr;
    std::unordered_map<std::string, FMOD_SOUND*> m_sounds;
};

} // namespace MiniEngine
```

### 加载的音效列表

```cpp
// 初始化时加载
audio.LoadSound("hit", "data/audio/sfx3d/hit.mp3", true);        // 3D
audio.LoadSound("hurt", "data/audio/sfx3d/hurt.mp3", true);       // 3D
audio.LoadSound("slash", "data/audio/sfx3d/slash.wav", true);     // 3D
audio.LoadSound("sword_swing", "data/audio/sfx3d/sword_swing.mp3", true);
audio.LoadSound("dash", "data/audio/sfx3d/dash.mp3");             // 2D
audio.LoadSound("jump", "data/audio/sfx3d/jump.mp3");             // 2D
audio.LoadSound("land", "data/audio/sfx3d/land.mp3");             // 2D
audio.LoadSound("draw_sword", "data/audio/sfx3d/draw_sword.mp3");
audio.LoadSound("bgm_battle", "data/audio/bgm/boss_battle.mp3", false, true); // 循环

// 游戏中播放
audio.PlaySound("jump");                          // 跳跃时
audio.PlaySound("dash");                          // 冲刺时
audio.PlaySound3D("hit", enemyPos);               // 命中敌人时（3D空间音效）
audio.PlaySound3D("hurt", playerPos);             // 受伤时
audio.PlayBGM("bgm_battle", 0.4f);               // 背景音乐

// 每帧更新听者位置
audio.SetListenerPosition(cameraPos, cameraForward, Vec3(0,1,0));
audio.Update();
```

---

## B. 3D空间音效

FMOD的3D音效特性：
- 远处的敌人攻击声音更小
- 左边的敌人声音从左声道传来
- 需要每帧更新听者位置（摄像机位置）

---

## C. 游戏UI系统

### 需要创建的文件
- `source/engine3d/GameUI3D.h` — 3D游戏UI

### UI内容

1. **玩家HUD（屏幕左上角）**
   - HP血条（绿→黄→红渐变）
   - MP/能量条
   - 当前武器/技能图标区域

2. **敌人血条（头顶）**
   - 在3D世界坐标转换为屏幕坐标
   - 用ImGui在对应屏幕位置绘制血条
   - 世界坐标→屏幕坐标：`screenPos = projection * view * worldPos`，然后透视除法

3. **伤害数字飘字**
   - 命中时在敌人位置生成飘字
   - 数字向上飘动并渐隐
   - 暴击用更大字号和不同颜色

4. **提示文字**
   - 中央提示（"按J攻击"、"进入战斗区域"等）
   - 自动淡出

### 世界坐标→屏幕坐标转换

```cpp
Vec2 WorldToScreen(Vec3 worldPos, Mat4 view, Mat4 proj, int screenW, int screenH) {
    // MVP变换
    float p[4] = {worldPos.x, worldPos.y, worldPos.z, 1.0f};
    // 手动矩阵乘法 view * vec4
    // 然后 proj * result
    // 透视除法 x/=w, y/=w
    // NDC[-1,1] 映射到 屏幕[0,w] [0,h]
    // 注意Y轴反转（屏幕Y向下）
    
    // 简化版：用Mat4乘法
    Mat4 vp = proj; // proj * view 需要自己实现矩阵乘法
    // ... 或者直接用4个float手动算
}
```

---

## 第29章需要创建的文件清单

1. `source/engine3d/AudioSystem3D.h` — FMOD音频封装
2. `source/engine3d/GameUI3D.h` — 游戏UI（HUD+血条+飘字）
3. `examples/29_ui_audio/main.cpp` — 演示程序
4. `examples/29_ui_audio/README.md` — 详细文档
5. 更新 CMakeLists.txt（FMOD链接+DLL拷贝）
6. 更新 README.md

## 场景设计

- 延续第28章的场景（天空盒+粒子+场景物体）
- 玩家有HP/MP血条
- 3个敌人有头顶血条
- 攻击命中：3D空间音效 + 伤害飘字 + 粒子火花
- 冲刺/跳跃/落地都有对应音效
- BGM循环播放
- ImGui编辑面板可调音量

## ImGui编辑面板

- Camera: 摄像机参数
- Audio: 主音量/BGM音量/SFX音量/3D音效距离衰减参数
- UI: 血条大小/位置/飘字速度/字号
- Combat: 攻击参数
- Lighting: 光照参数

## README要求

包含：FMOD初始化流程、2D vs 3D音效区别、3D听者概念、世界坐标→屏幕坐标数学推导、伤害飘字设计、5道思考题、下一章预告（第30章完整Demo）
