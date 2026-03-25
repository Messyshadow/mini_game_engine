# 第11章：精灵动画系统

## 📚 本章概述

本章学习如何从Spritesheet（精灵图集）加载角色动画，实现帧动画播放和动画状态机。

---

## 🎯 学习目标

1. 理解Spritesheet的结构和UV计算
2. 实现帧动画播放系统
3. 构建动画状态机
4. 根据角色状态自动切换动画

---

## 🔑 核心概念

### 1. Spritesheet（精灵图集）

Spritesheet是将角色所有动画帧排列在一张图片中：

```
player.png (256×192像素)

┌────┬────┬────┬────┐
│ 0  │ 1  │ 2  │ 3  │  行0: 待机动画 (Idle)
├────┼────┼────┼────┤
│ 4  │ 5  │ 6  │ 7  │  行1: 跑步动画 (Run)
├────┼────┼────┼────┤
│ 8  │ 9  │ 10 │ 11 │  行2: 跳跃动画 (Jump)
└────┴────┴────┴────┘
     每帧 64×64 像素
```

### 2. 帧动画原理

帧动画就是快速切换图片，产生运动的错觉：

```
时间线：  0ms    100ms   200ms   300ms   400ms ...
显示帧：  [0]  →  [1]  →  [2]  →  [3]  →  [0]  → 循环

关键参数：
- 帧率(FPS)：每秒播放多少帧
- 帧时长：每帧显示的时间 = 1.0 / FPS
```

### 3. 动画状态机

根据角色状态自动切换动画：

```
        ┌─────────┐
   ┌────│  Idle   │←───┐
   │    └────┬────┘    │
   │         │ 移动    │ 停止
   │         ↓         │
   │    ┌─────────┐    │
   │    │   Run   │────┘
   │    └────┬────┘
   │         │ 跳跃
   │         ↓
   │    ┌─────────┐
   └────│  Jump   │
  落地  └─────────┘
```

---

## 💻 关键代码

### 动画数据结构

```cpp
struct Animation {
    std::string name;       // 动画名称
    int startFrame;         // 起始帧索引
    int frameCount;         // 帧数量
    float frameTime;        // 每帧时长（秒）
    bool loop;              // 是否循环
};

// 配置动画
animator.AddAnimation(AnimState::Idle, Animation("Idle", 0, 4, 0.15f, true));
animator.AddAnimation(AnimState::Run,  Animation("Run",  4, 4, 0.1f,  true));
animator.AddAnimation(AnimState::Jump, Animation("Jump", 8, 4, 0.15f, false));
```

### UV坐标计算

```cpp
void CalculateFrameUV(int frameIndex, int cols, int rows, 
                      Vec2& uvMin, Vec2& uvMax) {
    int col = frameIndex % cols;
    int row = frameIndex / cols;
    
    float tileU = 1.0f / cols;
    float tileV = 1.0f / rows;
    
    // Y轴翻转
    uvMin.x = col * tileU;
    uvMin.y = 1.0f - (row + 1) * tileV;
    uvMax.x = (col + 1) * tileU;
    uvMax.y = 1.0f - row * tileV;
}
```

### 动画更新

```cpp
void AnimationController::Update(float deltaTime) {
    if (m_paused) return;
    
    const Animation& anim = m_animations[m_currentState];
    
    m_timer += deltaTime * m_speed;
    
    if (m_timer >= anim.frameTime) {
        m_timer -= anim.frameTime;
        m_currentFrame++;
        
        if (m_currentFrame >= anim.frameCount) {
            if (anim.loop) {
                m_currentFrame = 0;  // 循环
            } else {
                m_currentFrame = anim.frameCount - 1;  // 停在最后一帧
            }
        }
    }
}
```

### 状态机逻辑

```cpp
// 每帧更新动画状态
if (!g_isGrounded) {
    // 空中 → 跳跃动画
    g_animator.Play(AnimState::Jump);
} else if (moving) {
    // 地面移动 → 跑步动画
    g_animator.Play(AnimState::Run);
} else {
    // 站立 → 待机动画
    g_animator.Play(AnimState::Idle);
}
```

### 角色朝向翻转

```cpp
// 如果角色朝左，水平翻转UV
if (!g_facingRight) {
    std::swap(uvMin.x, uvMax.x);
}
```

---

## 📁 文件依赖

```
data/texture/
├── player.png     ← 角色动画（256×192, 4×3帧, 64×64/帧）
└── tileset.png    ← 地面纹理
```

### player.png 规格

| 属性 | 值 |
|------|-----|
| **尺寸** | 256 × 192 像素 |
| **布局** | 4列 × 3行 |
| **每帧** | 64 × 64 像素 |
| **总帧数** | 12帧 |

---

## 🎮 操作说明

| 按键 | 功能 |
|------|------|
| A/D | 左右移动（自动播放跑步动画） |
| 空格 | 跳跃（播放跳跃动画） |
| 1 | 手动播放待机动画 |
| 2 | 手动播放跑步动画 |
| 3 | 手动播放跳跃动画 |
| P | 暂停/继续动画 |
| +/- | 加速/减速动画 |
| R | 重置位置 |

---

## 🔧 动画参数调整

### 动画速度

```cpp
// 每帧时长越短，动画越快
Animation("Run", 4, 4, 0.08f, true);  // 更快的跑步
Animation("Run", 4, 4, 0.15f, true);  // 更慢的跑步

// 或者调整全局速度
g_animator.SetSpeed(1.5f);  // 1.5倍速
```

### 添加新动画

```cpp
// 假设spritesheet第4行是攻击动画
enum class AnimState { Idle, Run, Jump, Attack };
g_animator.AddAnimation(AnimState::Attack, Animation("Attack", 12, 4, 0.08f, false));
```

---

## 📊 动画系统架构

```
┌─────────────────────────────────────────────────┐
│                AnimationController              │
├─────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐              │
│  │ Animation 1 │  │ Animation 2 │  ...          │
│  │  - Idle     │  │  - Run      │              │
│  │  - frames   │  │  - frames   │              │
│  │  - timing   │  │  - timing   │              │
│  └─────────────┘  └─────────────┘              │
│                                                 │
│  currentState: AnimState                        │
│  currentFrame: int                              │
│  timer: float                                   │
│                                                 │
│  +Play(state)     → 切换动画                    │
│  +Update(dt)      → 更新帧                      │
│  +GetFrameIndex() → 获取当前帧                  │
└─────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────┐
│              SpriteRenderer                     │
│                                                 │
│  根据frameIndex计算UV                           │
│  渲染当前帧                                     │
└─────────────────────────────────────────────────┘
```

---

## 📝 思考题

1. **如何实现动画混合？**
   - 提示：在两个动画之间插值过渡

2. **如何支持不同速率的动画？**
   - 提示：每个Animation可以有独立的frameTime

3. **如何实现动画事件？**
   - 提示：在特定帧触发回调函数（如攻击帧播放音效）

4. **如何优化大量角色的动画？**
   - 提示：使用GPU实例化渲染

---

## 🔗 相关章节

- **第10章**：纹理瓦片地图（UV计算基础）
- **第12章**：摄像机系统（下一章）
- **第4章**：纹理与精灵（Sprite基础）

---

## ✅ 本章检查清单

- [ ] 理解Spritesheet的结构
- [ ] 掌握帧动画的UV计算
- [ ] 实现动画控制器
- [ ] 实现动画状态机
- [ ] 能够根据角色状态自动切换动画
- [ ] 实现角色朝向翻转
- [ ] 能够调整动画速度
