# 第六章：精灵动画系统

---

## 📚 本章概述

本章将学习2D游戏中的精灵动画系统：
1. 帧动画的原理
2. 精灵图集与UV计算
3. Animator动画控制器
4. 动画状态机与状态切换

---

## 🎯 学习目标

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           本章核心知识点                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. 帧动画原理                                                               │
│                                                                             │
│     帧动画 = 快速切换图片，产生动画效果（类似翻页动画）                        │
│                                                                             │
│     时间轴：─────●─────●─────●─────●─────●─────→                           │
│     帧号：     0     1     2     3     0 (循环)                            │
│                                                                             │
│     帧率(FPS)：每秒显示多少帧                                                │
│     帧时间 = 1.0 / FPS                                                      │
│                                                                             │
│  2. 精灵图集 (Sprite Sheet)                                                 │
│                                                                             │
│     将所有动画帧放在一张图中，通过UV坐标选择显示哪一帧                         │
│                                                                             │
│     ┌───┬───┬───┬───┐                                                      │
│     │ 0 │ 1 │ 2 │ 3 │  行0：待机动画 (Idle)                                │
│     ├───┼───┼───┼───┤                                                      │
│     │ 4 │ 5 │ 6 │ 7 │  行1：跑步动画 (Run)                                 │
│     ├───┼───┼───┼───┤                                                      │
│     │ 8 │ 9 │10 │11 │  行2：跳跃动画 (Jump)                                │
│     ├───┼───┼───┼───┤                                                      │
│     │12 │13 │14 │15 │  行3：下落动画 (Fall)                                │
│     └───┴───┴───┴───┘                                                      │
│                                                                             │
│  3. 动画控制器 (Animator)                                                    │
│                                                                             │
│     管理多个动画片段(Clip)，控制播放、暂停、切换                              │
│                                                                             │
│     ┌─────────────────────────────────────────┐                            │
│     │              Animator                   │                            │
│     │  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐   │                            │
│     │  │Idle │  │ Run │  │Jump │  │Fall │   │                            │
│     │  │Clip │  │Clip │  │Clip │  │Clip │   │                            │
│     │  └─────┘  └─────┘  └─────┘  └─────┘   │                            │
│     │            ↑                           │                            │
│     │         当前播放                        │                            │
│     └─────────────────────────────────────────┘                            │
│                                                                             │
│  4. 动画状态机                                                               │
│                                                                             │
│            ┌──────────┐                                                    │
│            │   Idle   │←──────────────────────┐                           │
│            └────┬─────┘                       │                           │
│          按A/D↓  ↑松开按键                    │                           │
│            ┌────┴─────┐                       │                           │
│            │   Run    │                       │                           │
│            └────┬─────┘                       │                           │
│          按空格↓                              │落地                        │
│            ┌────┴─────┐                       │                           │
│            │   Jump   │───────────┐          │                           │
│            └──────────┘           │Y速度<0   │                           │
│                                   ↓          │                           │
│                              ┌────┴─────┐    │                           │
│                              │   Fall   │────┘                           │
│                              └──────────┘                                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 💻 代码结构

### 文件清单

```
06_animation/
└── main.cpp          ← 示例代码（约546行）
```

### 核心代码解析

#### 1. 创建动画精灵

```cpp
// 设置精灵使用精灵图集
g_player.SetTexture(g_spriteSheet);
g_player.SetSpriteSheet(4, 4);  // 4列4行

// 设置位置和尺寸
g_player.SetPosition(640, g_groundY);
g_player.SetSize(128, 128);
g_player.SetPivotCenter();
```

#### 2. 添加动画片段

```cpp
// 添加动画：名称、起始帧、结束帧、帧率、是否循环
g_player.AddAnimation("idle", 0, 3, 8.0f, true);    // 待机：帧0-3，8FPS，循环
g_player.AddAnimation("run", 4, 7, 12.0f, true);    // 跑步：帧4-7，12FPS，循环
g_player.AddAnimation("jump", 8, 11, 10.0f, false); // 跳跃：帧8-11，10FPS，不循环
g_player.AddAnimation("fall", 12, 15, 8.0f, true);  // 下落：帧12-15，8FPS，循环

// 播放默认动画
g_player.Play("idle");
```

#### 3. 设置动画完成回调

```cpp
// 当动画播放完成时触发（仅对不循环动画有效）
g_player.GetAnimator().SetOnComplete([](const std::string& name) {
    std::cout << "[Animation] '" << name << "' completed!" << std::endl;
    
    // 跳跃动画完成后自动切换到下落
    if (name == "jump") {
        g_player.Play("fall");
        g_playerState = PlayerState::Fall;
    }
});
```

#### 4. 动画状态切换逻辑

```cpp
void Update(float deltaTime) {
    // 确定新状态
    PlayerState newState = g_playerState;
    
    if (!g_onGround) {
        // 在空中
        if (g_velocity.y > 0) {
            newState = PlayerState::Jump;   // 上升中
        } else {
            newState = PlayerState::Fall;   // 下落中
        }
    } else {
        // 在地面
        if (g_velocity.x != 0) {
            newState = PlayerState::Run;    // 移动中
        } else {
            newState = PlayerState::Idle;   // 静止
        }
    }
    
    // 状态改变时切换动画
    if (newState != g_playerState) {
        g_playerState = newState;
        
        switch (g_playerState) {
            case PlayerState::Idle:  g_player.Play("idle");  break;
            case PlayerState::Run:   g_player.Play("run");   break;
            case PlayerState::Jump:  g_player.Play("jump");  break;
            case PlayerState::Fall:  g_player.Play("fall");  break;
        }
    }
    
    // 更新动画
    g_player.Update(deltaTime);
}
```

#### 5. 动画更新内部逻辑

```cpp
void Animator::Update(float deltaTime) {
    if (!m_currentClip || m_paused) return;
    
    // 累加时间
    m_timer += deltaTime * m_speed;
    
    // 计算当前帧
    float frameDuration = 1.0f / m_currentClip->fps;
    
    if (m_timer >= frameDuration) {
        m_timer -= frameDuration;
        m_currentFrame++;
        
        // 检查是否播放完毕
        if (m_currentFrame > m_currentClip->endFrame) {
            if (m_currentClip->loop) {
                m_currentFrame = m_currentClip->startFrame;  // 循环
            } else {
                m_currentFrame = m_currentClip->endFrame;    // 停在最后一帧
                if (m_onComplete) {
                    m_onComplete(m_currentClip->name);       // 触发回调
                }
            }
        }
    }
}
```

---

## 🎮 操作说明

| 按键 | 功能 |
|------|------|
| A/D | 左右移动（播放跑步动画） |
| 空格 | 跳跃（播放跳跃动画） |
| 1/2/3/4 | 直接切换到 待机/跑步/跳跃/下落 动画 |
| +/- | 加速/减速动画播放 |
| ESC | 退出程序 |

---

## 📊 界面说明

```
┌─────────────────────────────────────────────────────────────────┐
│                        渲染窗口                                  │
│  ┌────────────────────────────────────────────────────────┐    │
│  │                                                        │    │
│  │                                                        │    │
│  │                                                        │    │
│  │                     ┌─────────┐                        │    │
│  │                     │ ██████  │  ← 玩家精灵            │    │
│  │                     │ ██  ██  │    颜色根据动画变化：   │    │
│  │                     │ ██████  │    蓝=待机 绿=跑步     │    │
│  │                     │ ██  ██  │    红=跳跃 紫=下落     │    │
│  │                     └─────────┘                        │    │
│  │ ████████████████████████████████████████████████████  │    │
│  │                     棕色地面                           │    │
│  └────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────┐  ┌─────────────────────────┐
│ Animation Demo          │  │ Animation Clips         │
├─────────────────────────┤  ├─────────────────────────┤
│ FPS: 60.0               │  │ Sprite Sheet: 4x4       │
│ ─────────────────────── │  │ ─────────────────────── │
│ Controls:               │  │ Idle (frames 0-3):      │
│ • A/D   - Move          │  │   [0] [1] [2] [3]       │
│ • Space - Jump          │  │ ─────────────────────── │
│ • 1-4   - Switch anim   │  │ Run (frames 4-7):       │
│ • +/-   - Speed         │  │   [4] [5] [6] [7]       │
│ ─────────────────────── │  │ ─────────────────────── │
│ Animation Info:         │  │ Jump (frames 8-11):     │
│   Current: run          │  │   [8] [9] [10] [11]     │
│   Frame: 5              │  │ ─────────────────────── │
│   Progress: 50.0%       │  │ Fall (frames 12-15):    │
│   Speed: 1.00x          │  │   [12] [13] [14] [15]   │
│ ─────────────────────── │  │ ─────────────────────── │
│ Player State:           │  │ Note: This demo uses    │
│   State: Run            │  │ procedural textures.    │
│   Position: (640, 150)  │  │ In real games, load     │
│   Velocity: (300, 0)    │  │ sprite sheet images.    │
│   On Ground: Yes        │  └─────────────────────────┘
│   Flip X: No            │
│ ─────────────────────── │
│ UV Coordinates:         │
│   Min: (0.250, 0.250)   │
│   Max: (0.500, 0.500)   │
└─────────────────────────┘
```

---

## 📐 动画系统详解

### UV坐标计算

```
    精灵图集4x4，选择第5帧：
    
    frameIndex = 5
    cols = 4, rows = 4
    
    col = 5 % 4 = 1    (第1列，从0开始)
    row = 5 / 4 = 1    (第1行，从0开始)
    
    uvWidth  = 1.0 / 4 = 0.25
    uvHeight = 1.0 / 4 = 0.25
    
    uvMin = (col × uvWidth, row × uvHeight)
          = (1 × 0.25, 1 × 0.25)
          = (0.25, 0.25)
    
    uvMax = ((col+1) × uvWidth, (row+1) × uvHeight)
          = (2 × 0.25, 2 × 0.25)
          = (0.50, 0.50)
    
    ┌───┬───┬───┬───┐
    │   │   │   │   │ ← row=0, uv.y: 0.00~0.25
    ├───┼───┼───┼───┤
    │   │ ● │   │   │ ← row=1, uv.y: 0.25~0.50  (选中)
    ├───┼───┼───┼───┤
    │   │   │   │   │
    ├───┼───┼───┼───┤
    │   │   │   │   │
    └───┴───┴───┴───┘
        ↑
      col=1
    uv.x: 0.25~0.50
```

### 动画时间控制

```
    帧率 = 12 FPS（每秒12帧）
    帧时间 = 1.0 / 12 ≈ 0.083秒
    
    时间轴：
    0.000s ─────── 0.083s ─────── 0.167s ─────── 0.250s ───→
       ↑             ↑             ↑             ↑
     帧0           帧1           帧2           帧3
    
    累加器逻辑：
    timer += deltaTime
    if (timer >= frameDuration) {
        timer -= frameDuration
        currentFrame++
    }
```

### 动画速度调节

```cpp
// 速度倍数
float speed = 1.0f;  // 正常速度
float speed = 2.0f;  // 2倍速
float speed = 0.5f;  // 半速

// 应用到更新
m_timer += deltaTime * m_speed;

// 效果：
// speed=2.0 → timer增长更快 → 帧切换更快 → 动画加速
// speed=0.5 → timer增长更慢 → 帧切换更慢 → 动画减速
```

---

## 🔧 关键类说明

### AnimationClip - 动画片段

```cpp
struct AnimationClip {
    std::string name;    // 动画名称
    int startFrame;      // 起始帧索引
    int endFrame;        // 结束帧索引
    float fps;           // 帧率
    bool loop;           // 是否循环
    
    int GetFrameCount() const { return endFrame - startFrame + 1; }
    float GetDuration() const { return GetFrameCount() / fps; }
};
```

### Animator - 动画控制器

```cpp
class Animator {
public:
    // 添加动画
    void AddClip(const std::string& name, int startFrame, int endFrame, 
                 float fps, bool loop);
    
    // 播放控制
    void Play(const std::string& name, bool restart = false);
    void Pause();
    void Resume();
    void Stop();
    
    // 属性
    void SetSpeed(float speed);
    float GetSpeed() const;
    
    int GetCurrentFrame() const;
    float GetProgress() const;  // 0.0 ~ 1.0
    const std::string& GetCurrentClipName() const;
    
    // 回调
    void SetOnComplete(std::function<void(const std::string&)> callback);
    void SetOnFrameChange(std::function<void(int)> callback);
    
    // 更新
    void Update(float deltaTime);
};
```

### AnimatedSprite - 动画精灵

```cpp
class AnimatedSprite : public Sprite {
public:
    // 设置精灵图集
    void SetSpriteSheet(int columns, int rows);
    
    // 动画管理
    void AddAnimation(const std::string& name, int startFrame, int endFrame,
                      float fps, bool loop);
    void Play(const std::string& name, bool restart = false);
    
    // 获取动画器
    Animator& GetAnimator();
    
    // 更新（必须每帧调用）
    void Update(float deltaTime);
    
    // 获取当前UV
    void GetCurrentUV(Vec2& uvMin, Vec2& uvMax) const;
};
```

---

## 🎯 练习建议

1. **观察动画切换**：移动和跳跃，观察动画状态变化
2. **调整动画速度**：按+/-键，观察速度变化效果
3. **理解帧索引**：观察UV坐标和帧号的对应关系
4. **修改帧率**：尝试修改各动画的FPS值

---

## 📁 本章文件

```
examples/06_animation/
├── README.md           ← 本文档
└── main.cpp            ← 示例代码
```

---

## 🎯 下一章预告

**第七章：平台跳跃物理** 将学习：
- 重力与速度
- 地面检测
- 跳跃实现
- 单向平台
