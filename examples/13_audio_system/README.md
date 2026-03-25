# 第13章：音频系统

## 学习目标
1. 使用miniaudio库播放音频
2. 背景音乐（BGM）循环播放
3. 音效（SFX）触发播放
4. 音量控制

## 依赖
本章需要 **miniaudio** 库（单头文件）。

### 安装步骤
1. 下载 miniaudio.h：
   ```
   https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h
   ```
2. 放到 `depends/miniaudio/miniaudio.h`

## 操作说明

| 按键 | 功能 |
|------|------|
| M | 播放/停止背景音乐 |
| 1 | 播放斧头音效 |
| 2 | 播放弓箭音效 |
| 3 | 播放爆炸音效 |
| ↑/↓ | 调节主音量 |
| ←/→ | 调节音乐音量 |

## AudioSystem API

```cpp
// 初始化
AudioSystem audio;
audio.Initialize();

// 加载音频
audio.LoadSound("jump", "data/audio/sfx/jump.wav");
audio.LoadMusic("bgm", "data/audio/bgm/music.mp3");

// 播放
audio.PlaySound("jump");       // 播放音效（单次）
audio.PlayMusic("bgm");        // 播放音乐（循环）

// 控制
audio.StopMusic();
audio.PauseMusic();
audio.ResumeMusic();

// 音量 (0.0 - 1.0)
audio.SetMasterVolume(0.8f);
audio.SetMusicVolume(0.5f);
audio.SetSFXVolume(1.0f);
```

## 支持的格式
- WAV
- MP3
- FLAC
- OGG
