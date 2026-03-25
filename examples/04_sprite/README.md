# 第四章：纹理与精灵

---

## 📚 本章概述

本章将学习2D游戏中最核心的概念——纹理和精灵：
1. 纹理(Texture)的加载和使用
2. 精灵(Sprite)的概念和属性
3. UV坐标与纹理映射
4. 精灵图集(Sprite Sheet)与帧动画

---

## 🎯 学习目标

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           本章核心知识点                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. 纹理 (Texture)                                                          │
│                                                                             │
│     纹理是存储在GPU显存中的图像数据                                           │
│                                                                             │
│     ┌─────────────────────┐                                                │
│     │ 纹理图片 (PNG/JPG)  │                                                │
│     │                     │                                                │
│     │  ┌───┬───┬───┬───┐ │    UV坐标：                                    │
│     │  │   │   │   │   │ │    (0,0) = 左下角                              │
│     │  ├───┼───┼───┼───┤ │    (1,1) = 右上角                              │
│     │  │   │   │   │   │ │                                                │
│     │  ├───┼───┼───┼───┤ │    注意：很多图片格式Y轴是从上到下              │
│     │  │   │   │   │   │ │    加载时需要翻转！                             │
│     │  └───┴───┴───┴───┘ │                                                │
│     └─────────────────────┘                                                │
│                                                                             │
│  2. 精灵 (Sprite)                                                           │
│                                                                             │
│     精灵 = 纹理 + 变换 + 渲染属性                                            │
│                                                                             │
│     属性：                                                                   │
│     • 位置 (Position) - 在世界坐标中的位置                                   │
│     • 大小 (Size) - 显示尺寸                                                │
│     • 旋转 (Rotation) - 旋转角度                                            │
│     • 缩放 (Scale) - 缩放倍数                                               │
│     • 锚点 (Pivot) - 旋转和定位的参考点                                      │
│     • 颜色 (Color) - 混合颜色                                               │
│     • UV (uvMin, uvMax) - 纹理的哪个区域                                    │
│                                                                             │
│  3. 精灵图集 (Sprite Sheet)                                                 │
│                                                                             │
│     将多个小图合并成一张大图，通过UV选择显示哪一帧                            │
│                                                                             │
│     ┌───┬───┬───┬───┐                                                      │
│     │ 0 │ 1 │ 2 │ 3 │  ← 第0行                                            │
│     ├───┼───┼───┼───┤                                                      │
│     │ 4 │ 5 │ 6 │ 7 │  ← 第1行                                            │
│     ├───┼───┼───┼───┤                                                      │
│     │ 8 │ 9 │10 │11 │  ← 第2行                                            │
│     ├───┼───┼───┼───┤                                                      │
│     │12 │13 │14 │15 │  ← 第3行                                            │
│     └───┴───┴───┴───┘                                                      │
│       ↑   ↑   ↑   ↑                                                        │
│      列0 列1 列2 列3                                                        │
│                                                                             │
│     帧索引 → UV坐标的计算：                                                  │
│     col = frameIndex % columns                                              │
│     row = frameIndex / columns                                              │
│     uvMin = (col/cols, row/rows)                                           │
│     uvMax = ((col+1)/cols, (row+1)/rows)                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 💻 代码结构

### 文件清单

```
04_sprite/
└── main.cpp          ← 示例代码（约434行）
```

### 核心代码解析

#### 1. 程序生成棋盘格纹理

```cpp
std::shared_ptr<Texture> CreateCheckerTexture(int width, int height, int gridSize) {
    std::vector<unsigned char> pixels(width * height * 4);  // RGBA格式
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // 计算棋盘格：根据格子位置决定颜色
            bool isWhite = ((x / gridSize) + (y / gridSize)) % 2 == 0;
            
            int index = (y * width + x) * 4;  // 每像素4字节
            if (isWhite) {
                pixels[index + 0] = 255;  // R
                pixels[index + 1] = 255;  // G
                pixels[index + 2] = 255;  // B
            } else {
                pixels[index + 0] = 128;  // R
                pixels[index + 1] = 128;  // G
                pixels[index + 2] = 128;  // B
            }
            pixels[index + 3] = 255;  // A (完全不透明)
        }
    }
    
    auto texture = std::make_shared<Texture>();
    texture->LoadFromMemory(pixels.data(), width, height, 4);
    return texture;
}
```

#### 2. 设置精灵属性

```cpp
// 设置纹理
g_mainSprite.SetTexture(g_colorTexture);

// 设置位置（屏幕中心）
g_mainSprite.SetPosition(640, 360);

// 设置显示尺寸
g_mainSprite.SetSize(200, 200);

// 设置锚点为中心（旋转和缩放以中心为基准）
g_mainSprite.SetPivotCenter();

// 设置颜色（会与纹理颜色相乘）
g_mainSprite.SetColor(Vec4(1, 1, 1, 1));  // 白色=原色
```

#### 3. 精灵图集帧切换

```cpp
// 设置精灵使用精灵图集的某一帧
// 参数：帧索引、列数、行数
g_mainSprite.SetFrameFromSheet(g_currentFrame, 4, 4);

// 内部计算UV坐标：
// col = frameIndex % columns = 5 % 4 = 1
// row = frameIndex / columns = 5 / 4 = 1
// uvMin = (1/4, 1/4) = (0.25, 0.25)
// uvMax = (2/4, 2/4) = (0.50, 0.50)
```

#### 4. 使用SpriteRenderer绘制

```cpp
// 开始批次渲染
g_renderer->Begin();

// 设置投影矩阵
g_renderer->SetProjection(g_projection);

// 绘制多个精灵
for (auto& sprite : g_backgroundSprites) {
    g_renderer->DrawSprite(sprite);
}
g_renderer->DrawSprite(g_mainSprite);

// 绘制纯色矩形（也可以用SpriteRenderer）
g_renderer->DrawRect(Vec2(50, 50), Vec2(80, 80), Vec4(1, 0, 0, 0.5f));

// 结束批次
g_renderer->End();
```

---

## 🎮 操作说明

| 按键 | 功能 |
|------|------|
| W/A/S/D | 移动精灵 |
| Q/E | 旋转精灵 |
| Z/X | 放大/缩小精灵 |
| 空格 | 切换到下一帧 |
| R | 重置变换 |
| ESC | 退出程序 |

---

## 📊 界面说明

```
┌─────────────────────────────────────────────────────────────────┐
│                        渲染窗口                                  │
│  ┌────────────────────────────────────────────────────────┐    │
│  │ 背景：半透明棋盘格精灵                                   │    │
│  │  ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐           │    │
│  │  │▓▓▓│ │▓▓▓│ │▓▓▓│ │▓▓▓│ │▓▓▓│ │▓▓▓│ │▓▓▓│           │    │
│  │  └───┘ └───┘ └───┘ └───┘ └───┘ └───┘ └───┘           │    │
│  │  ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐           │    │
│  │  │▓▓▓│ │▓▓▓│ │▓▓▓│ │███│ │▓▓▓│ │▓▓▓│ │▓▓▓│           │    │
│  │  └───┘ └───┘ └───┘ │   │ └───┘ └───┘ └───┘           │    │
│  │                    │主 │                               │    │
│  │  纯色矩形示例：      │精 │                               │    │
│  │  ■ ◆ ■              │灵 │                               │    │
│  │ 红 绿 蓝            └───┘                               │    │
│  │ (旋转45°)                                               │    │
│  └────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────┐  ┌─────────────────────────┐
│ Sprite Demo             │  │ Texture Info            │
├─────────────────────────┤  ├─────────────────────────┤
│ FPS: 60.0               │  │ Color Grid Texture:     │
│ ─────────────────────── │  │   Size: 64x64           │
│ Controls:               │  │   Channels: 4           │
│ • WASD - Move           │  │   ID: 2                 │
│ • Q/E  - Rotate         │  │ ─────────────────────── │
│ • Z/X  - Scale          │  │ Checker Texture:        │
│ • Space - Next frame    │  │   Size: 128x128         │
│ • R    - Reset          │  │   ID: 1                 │
│ ─────────────────────── │  │ ─────────────────────── │
│ Sprite Properties:      │  │ Tip: Put your images in│
│ Position: [640] [360]   │  │ 'template/data/texture/'│
│ Rotation: [0.0]         │  └─────────────────────────┘
│ Size: [200] [200]       │
│ Color: [████████████]   │
│ ─────────────────────── │
│ Animation:              │
│ Frame: [====5====]      │
│ [✓] Auto Animate        │
│ Speed: [===0.2===]      │
│ ─────────────────────── │
│ [✓] Flip X  [ ] Flip Y  │
└─────────────────────────┘
```

---

## 📐 纹理详解

### 纹理过滤模式

```
    当纹理被放大或缩小显示时，如何采样像素？
    
    GL_NEAREST（最近邻）：          GL_LINEAR（线性插值）：
    
    选择最近的一个像素              对周围4个像素加权平均
    
    ┌───┬───┐                      ┌───┬───┐
    │ A │ B │  采样点在中间        │ A │ B │  结果 = (A+B+C+D)/4
    ├───┼───┤  结果 = A            ├───┼───┤  (实际是加权)
    │ C │ D │                      │ C │ D │
    └───┴───┘                      └───┴───┘
    
    效果：像素清晰，有锯齿          效果：平滑，但可能模糊
    适合：像素艺术游戏              适合：写实风格游戏
```

### UV坐标系统

```
    UV坐标范围是 [0, 1]，表示纹理的归一化坐标
    
    OpenGL约定（Y轴向上）：          图片文件约定（Y轴向下）：
    
    (0,1)────────(1,1)              (0,0)────────(1,0)
      │            │                 │            │
      │   纹理     │        VS       │   图片     │
      │            │                 │            │
    (0,0)────────(1,0)              (0,1)────────(1,1)
    
    解决方案：加载纹理时设置 flipY = true
```

### 精灵图集UV计算

```cpp
// 假设4x4的精灵图集，获取第5帧的UV
int frameIndex = 5;
int cols = 4, rows = 4;

int col = frameIndex % cols;  // = 5 % 4 = 1
int row = frameIndex / cols;  // = 5 / 4 = 1

float uvWidth = 1.0f / cols;   // = 0.25
float uvHeight = 1.0f / rows;  // = 0.25

Vec2 uvMin(col * uvWidth, row * uvHeight);           // = (0.25, 0.25)
Vec2 uvMax((col + 1) * uvWidth, (row + 1) * uvHeight); // = (0.50, 0.50)
```

---

## 🔧 关键类说明

### Texture - 纹理类

```cpp
class Texture {
public:
    // 从文件加载
    bool Load(const std::string& filepath, bool flipY = true);
    
    // 从内存创建
    bool LoadFromMemory(const unsigned char* data, int width, int height, int channels);
    
    // 创建纯色纹理
    bool CreateSolidColor(const Vec4& color, int width = 1, int height = 1);
    
    // 绑定到纹理单元
    void Bind(unsigned int slot = 0) const;
    void Unbind() const;
    
    // 设置过滤模式
    void SetFilter(TextureFilter min, TextureFilter mag);
    
    // 属性
    int GetWidth() const;
    int GetHeight() const;
    unsigned int GetID() const;
};
```

### Sprite - 精灵类

```cpp
class Sprite {
public:
    // 纹理
    void SetTexture(std::shared_ptr<Texture> texture);
    
    // 变换
    void SetPosition(float x, float y);
    void SetPosition(const Vec2& pos);
    void SetSize(float w, float h);
    void SetSize(const Vec2& size);
    void SetRotation(float radians);
    void SetRotationDegrees(float degrees);
    void SetScale(const Vec2& scale);
    
    // 锚点
    void SetPivot(float x, float y);   // 0~1范围
    void SetPivotCenter();             // = SetPivot(0.5, 0.5)
    
    // 颜色
    void SetColor(const Vec4& color);
    
    // UV坐标
    void SetUV(const Vec2& min, const Vec2& max);
    void SetFrameFromSheet(int frameIndex, int cols, int rows);
    
    // 翻转
    void SetFlipX(bool flip);
    void SetFlipY(bool flip);
};
```

### SpriteRenderer - 精灵渲染器

```cpp
class SpriteRenderer {
public:
    bool Initialize();
    void Shutdown();
    
    void Begin();  // 开始批次
    void End();    // 结束批次
    
    void SetProjection(const Mat4& projection);
    
    void DrawSprite(const Sprite& sprite);
    void DrawTexture(const Texture& texture, const Vec2& position, ...);
    void DrawRect(const Vec2& position, const Vec2& size, const Vec4& color, float rotation = 0);
};
```

---

## 🎯 练习建议

1. **观察帧切换**：启用自动动画，观察精灵图集的帧变化
2. **调整颜色**：修改精灵颜色，观察颜色混合效果
3. **翻转精灵**：勾选Flip X/Y，观察翻转效果
4. **加载真实图片**：尝试加载自己的PNG图片

---

## 📁 本章文件

```
examples/04_sprite/
├── README.md           ← 本文档
└── main.cpp            ← 示例代码
```

---

## 🎯 下一章预告

**第五章：2D碰撞检测** 将学习：
- AABB（轴对齐包围盒）碰撞
- 圆形碰撞
- 碰撞响应（推出、反弹）
- 穿透深度计算
