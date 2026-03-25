# 第一章：基础窗口与向量数学

---

## 📚 本章概述

本章是游戏引擎开发的起点，你将学习：
1. 如何创建和管理游戏窗口
2. 游戏主循环的基本结构
3. 2D向量数学的基础操作
4. ImGui调试界面的使用

---

## 🎯 学习目标

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           本章核心知识点                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. 引擎初始化流程                                                           │
│     ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐          │
│     │ 创建窗口  │───→│ 初始化GL │───→│ 初始化GUI│───→│ 主循环    │          │
│     └──────────┘    └──────────┘    └──────────┘    └──────────┘          │
│                                                                             │
│  2. 向量运算                                                                 │
│     • 加法：A + B = (Ax+Bx, Ay+By)                                         │
│     • 点积：A · B = Ax×Bx + Ay×By  （判断方向关系）                         │
│     • 叉积：A × B = Ax×By - Ay×Bx  （判断左右关系）                         │
│     • 长度：|A| = √(Ax² + Ay²)                                             │
│     • 角度：θ = arccos(A·B / |A||B|)                                       │
│                                                                             │
│  3. 游戏主循环                                                               │
│     ┌─────────────────────────────────────────┐                            │
│     │  while (running) {                      │                            │
│     │      ProcessInput();   // 处理输入      │                            │
│     │      Update(dt);       // 更新逻辑      │                            │
│     │      Render();         // 渲染画面      │                            │
│     │  }                                      │                            │
│     └─────────────────────────────────────────┘                            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 💻 代码结构

### 文件清单

```
01_window/
└── main.cpp          ← 示例代码（约75行）
```

### 核心代码解析

#### 1. 引擎初始化

```cpp
// 获取引擎单例
Engine& engine = Engine::Instance();

// 配置窗口参数
EngineConfig config;
config.windowTitle = "Mini Engine - Vector Math";

// 初始化引擎（创建窗口、初始化OpenGL）
if (!engine.Initialize(config)) return -1;
```

#### 2. 回调函数设置

```cpp
// 设置三个核心回调函数
engine.SetUpdateCallback(Update);      // 逻辑更新（每帧调用）
engine.SetRenderCallback(Render);      // 图形渲染（每帧调用）
engine.SetImGuiCallback(RenderImGui);  // GUI渲染（每帧调用）

// 启动主循环
engine.Run();
```

#### 3. 向量运算演示

```cpp
Vec2 vectorA(3.0f, 2.0f);
Vec2 vectorB(1.0f, 4.0f);

// 向量加法
Vec2 sum = vectorA + vectorB;  // = (4, 6)

// 点积（内积）
float dot = vectorA.Dot(vectorB);  // = 3×1 + 2×4 = 11

// 叉积（2D中返回标量）
float cross = vectorA.Cross(vectorB);  // = 3×4 - 2×1 = 10

// 向量长度
float length = vectorA.Length();  // = √(9+4) = √13 ≈ 3.61

// 两向量夹角
float angle = Vec2::AngleBetween(vectorA, vectorB);
```

---

## 🎮 操作说明

| 操作 | 功能 |
|------|------|
| 拖动滑块 | 修改向量A和B的值 |
| Quit按钮 | 退出程序 |
| ESC | 退出程序 |

---

## 📊 界面说明

程序运行后会显示两个ImGui窗口：

```
┌─────────────────────────┐  ┌─────────────────────────┐
│ Engine Info             │  │ Vector Math Demo        │
├─────────────────────────┤  ├─────────────────────────┤
│ FPS: 60.0               │  │ Vector A: [3.0] [2.0]   │
│ Frame Time: 16.7 ms     │  │ Vector B: [1.0] [4.0]   │
│                         │  │ ─────────────────────── │
│ [Quit]                  │  │ Operations:             │
│                         │  │ A + B = (4.00, 6.00)    │
└─────────────────────────┘  │ A · B = 11.000          │
                             │ A × B = 10.000          │
                             │ |A| = 3.606             │
                             │ Angle = 29.7 degrees    │
                             └─────────────────────────┘
```

---

## 📐 向量数学详解

### 点积 (Dot Product) 的几何意义

```
    点积的结果告诉我们两个向量的方向关系：
    
    A · B > 0  →  夹角 < 90°  →  大致同向
    A · B = 0  →  夹角 = 90°  →  垂直
    A · B < 0  →  夹角 > 90°  →  大致反向
    
        B                    B                     B
        ↑                    ↑                      ↖
        │                    │                        ╲
        │ θ<90°              │ θ=90°                   ╲ θ>90°
    ────●────→ A         ────●────→ A             ────●────→ A
      点积>0               点积=0                   点积<0
```

### 叉积 (Cross Product) 的几何意义

```
    2D叉积的结果是一个标量，表示有向面积：
    
    A × B > 0  →  B在A的逆时针方向（左边）
    A × B = 0  →  A和B平行
    A × B < 0  →  B在A的顺时针方向（右边）
    
         B                              
        ↗                              A
       ╱ 面积=A×B                      ↗
      ╱                               ╱
     ●────→ A                    ────●
      叉积>0                          ╲
     (B在A左边)                        ↘ B
                                     叉积<0
                                    (B在A右边)
```

---

## 🔧 关键类说明

### Vec2 - 2D向量类

```cpp
struct Vec2 {
    float x, y;
    
    // 构造
    Vec2();                    // 默认(0,0)
    Vec2(float x, float y);    // 指定值
    
    // 运算符
    Vec2 operator+(const Vec2& v) const;  // 加法
    Vec2 operator-(const Vec2& v) const;  // 减法
    Vec2 operator*(float s) const;        // 标量乘法
    
    // 方法
    float Length() const;      // 长度
    float Dot(const Vec2& v) const;       // 点积
    float Cross(const Vec2& v) const;     // 叉积
    Vec2 Normalized() const;   // 单位化
    
    // 静态方法
    static Vec2 Zero();        // (0, 0)
    static float AngleBetween(const Vec2& a, const Vec2& b);
};
```

### Engine - 引擎单例类

```cpp
class Engine {
public:
    static Engine& Instance();  // 获取单例
    
    bool Initialize(const EngineConfig& config);
    void Shutdown();
    void Run();  // 主循环
    void Quit(); // 退出
    
    // 回调设置
    void SetUpdateCallback(std::function<void(float)> callback);
    void SetRenderCallback(std::function<void()> callback);
    void SetImGuiCallback(std::function<void()> callback);
    
    // 属性获取
    float GetFPS() const;
    float GetDeltaTime() const;
    int GetWindowWidth() const;
    int GetWindowHeight() const;
    GLFWwindow* GetWindow() const;
};
```

---

## 🎯 练习建议

1. **修改向量值**：尝试拖动滑块，观察各种运算结果的变化
2. **理解点积**：让两个向量垂直，观察点积为0
3. **理解叉积**：观察叉积正负与向量相对位置的关系
4. **尝试修改代码**：添加更多向量运算（如归一化、投影等）

---

## 📁 本章文件

```
examples/01_window/
├── README.md           ← 本文档
└── main.cpp            ← 示例代码
```

---

## 🎯 下一章预告

**第二章：三角形渲染与向量可视化** 将学习：
- OpenGL基础渲染
- 在屏幕上绘制图形
- 向量运算的可视化表示
