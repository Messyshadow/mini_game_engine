# 第二章：三角形渲染与向量可视化

---

## 📚 本章概述

本章将学习OpenGL基础渲染和向量的可视化表示：
1. 使用Renderer2D绘制基本图形
2. 向量加法、减法的可视化
3. 点积和叉积的几何意义
4. 交互式向量操作

---

## 🎯 学习目标

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           本章核心知识点                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. NDC坐标系 (Normalized Device Coordinates)                               │
│                                                                             │
│         Y轴                                                                 │
│          ↑ (0,1)                                                           │
│          │                                                                  │
│   (-1,0) ├───────────────→ X轴 (1,0)                                       │
│          │      (0,0)                                                       │
│          │                                                                  │
│        (0,-1)                                                               │
│                                                                             │
│     NDC范围：X ∈ [-1, 1], Y ∈ [-1, 1]                                      │
│     这是OpenGL渲染的标准坐标系                                               │
│                                                                             │
│  2. 基本图形绘制                                                             │
│     • 三角形：最基本的图元，所有图形都由三角形组成                            │
│     • 矩形：由两个三角形组成                                                 │
│     • 圆形：由多个三角形扇形组成                                             │
│     • 线段：连接两点的直线                                                   │
│                                                                             │
│  3. 向量可视化                                                               │
│                                                                             │
│     向量加法：              向量减法：              点积投影：               │
│         B                      A-B                    B                    │
│        ↗                      ↗                      ↗                     │
│       ╱                      ╱                      ╱                      │
│      ╱  A+B                 ╱                      ╱ proj                  │
│     ●────→ A               ●────→ A               ●────→ A                 │
│      ╲    ↗                                        └─┘                     │
│       ╲  ╱                                         投影                    │
│        ↘╱                                                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 💻 代码结构

### 文件清单

```
02_triangle/
└── main.cpp          ← 示例代码（约153行）
```

### 演示模式

本示例包含4种演示模式：

| 模式 | 说明 | 演示内容 |
|------|------|----------|
| Basic Shapes | 基础图形 | 三角形、矩形、圆形的绘制 |
| Vector Operations | 向量运算 | 向量加法、减法的可视化 |
| Dot Product | 点积 | 向量投影的可视化 |
| Cross Product | 叉积 | 平行四边形面积的可视化 |

### 核心代码解析

#### 1. 绘制彩色三角形

```cpp
// 使用顶点颜色创建渐变三角形
g_renderer->DrawTriangle(
    Vertex2D(-0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f),  // 左下顶点：红色
    Vertex2D( 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f),  // 右下顶点：绿色
    Vertex2D( 0.0f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f)   // 顶部顶点：蓝色
);
// GPU会自动在三个顶点之间插值颜色，形成渐变效果
```

#### 2. 绘制向量箭头

```cpp
// 从原点绘制向量A（红色箭头）
g_renderer->DrawVector(Vec2::Zero(), g_vectorA, Vec4::Red(), 1.0f);

// 从原点绘制向量B（绿色箭头）  
g_renderer->DrawVector(Vec2::Zero(), g_vectorB, Vec4::Green(), 1.0f);

// 绘制向量和 A+B（黄色箭头）
Vec2 sum = g_vectorA + g_vectorB;
g_renderer->DrawVector(Vec2::Zero(), sum, Vec4::Yellow(), 1.0f);
```

#### 3. 点积投影可视化

```cpp
// 将向量B投影到向量A上
Vec2 aNorm = a.Normalized();                    // A的单位向量
Vec2 projection = aNorm * (b.Dot(aNorm));       // 投影公式：proj = â × (B·â)

// 绘制投影线（灰色虚线）
g_renderer->DrawLine(b, projection, Vec4(0.5f, 0.5f, 0.5f, 0.8f), 1.0f);

// 绘制投影向量（蓝色）
g_renderer->DrawVector(Vec2::Zero(), projection, Vec4::Blue(), 1.0f);
```

#### 4. 叉积面积可视化

```cpp
// 计算叉积（2D中为标量，表示有向面积）
float cross = g_vectorA.Cross(g_vectorB);

// 根据叉积正负选择颜色
// 正值（B在A左边）：蓝色    负值（B在A右边）：橙色
Vec4 parallelogramColor = (cross > 0) 
    ? Vec4(0.0f, 0.5f, 1.0f, 0.3f)   // 蓝色半透明
    : Vec4(1.0f, 0.5f, 0.0f, 0.3f);  // 橙色半透明

// 绘制平行四边形（由两个三角形组成）
g_renderer->DrawTriangle(Vec2::Zero(), g_vectorA, g_vectorA + g_vectorB, parallelogramColor);
g_renderer->DrawTriangle(Vec2::Zero(), g_vectorA + g_vectorB, g_vectorB, parallelogramColor);
```

---

## 🎮 操作说明

| 按键/操作 | 功能 |
|-----------|------|
| 单选按钮 | 切换演示模式 |
| 拖动Vector A/B | 修改向量值 |
| Animate Vector B | 让向量B自动旋转 |
| Speed滑块 | 调整旋转速度 |
| ESC | 退出程序 |

---

## 📊 界面说明

```
┌─────────────────────────────────────────────────────────────────┐
│                        渲染窗口                                  │
│                                                                 │
│                          ↑ Y轴                                  │
│                          │                                      │
│                    B ↗   │                                      │
│                     ╱    │    A+B                               │
│                    ╱     │   ↗                                  │
│             ──────●──────┼──→ A ──────→ X轴                     │
│                          │                                      │
│                          │                                      │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│ [Vector Demo Controls]                                          │
│ ○ Basic Shapes   ○ Vector Operations                           │
│ ○ Dot Product    ○ Cross Product                               │
│ Vector A: [0.50] [0.30]                                         │
│ Vector B: [0.20] [0.60]                                         │
│ [✓] Animate Vector B  Speed: [====1.0====]                     │
│ A · B = 0.280   A × B = 0.240   Angle = 38.7 deg              │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📐 向量运算详解

### 向量加法的几何意义

```
    向量加法遵循"首尾相连"法则：
    
    把B的起点放到A的终点，
    从A的起点到B的终点就是A+B
    
              B
             ↗│
            ╱ │
           ╱  │
          ╱   ↓ A+B
         ●────→ A
         
    公式：A + B = (Ax+Bx, Ay+By)
```

### 点积的投影意义

```
    点积可以计算一个向量在另一个向量上的投影长度：
    
    proj_A(B) = |B| × cos(θ) = (A·B) / |A|
    
    投影向量 = Â × (B·Â)  （Â是A的单位向量）
    
              B
             ↗│
            ╱ │
           ╱  │ 垂直线
          ╱   │
         ●────┼──→ A
              │
           投影点
           
    应用：
    • 计算光照强度（法线与光线的点积）
    • 判断物体在视野内还是视野外
    • 计算物体间的相对方向
```

### 叉积的面积意义

```
    2D叉积的绝对值等于两向量构成的平行四边形面积：
    
    |A × B| = |A| × |B| × sin(θ) = 平行四边形面积
    
         B
        ↗╲
       ╱  ╲  面积 = |A×B|
      ╱    ╲
     ●──────→ A
     
    符号意义：
    • A × B > 0：B在A的逆时针方向（左边）
    • A × B < 0：B在A的顺时针方向（右边）
    • A × B = 0：A和B平行
    
    应用：
    • 判断点在线段的哪一侧
    • 计算多边形面积
    • 判断转弯方向（左转/右转）
```

---

## 🔧 关键类说明

### Renderer2D - 2D渲染器

```cpp
class Renderer2D {
public:
    bool Initialize();
    void Shutdown();
    
    // 基础图形
    void DrawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color);
    void DrawTriangle(const Vertex2D& v1, const Vertex2D& v2, const Vertex2D& v3);
    void DrawRect(const Vec2& position, const Vec2& size, const Vec4& color);
    void DrawCircle(const Vec2& center, float radius, const Vec4& color, int segments = 32);
    void DrawLine(const Vec2& p1, const Vec2& p2, const Vec4& color, float width = 1.0f);
    void DrawPoint(const Vec2& position, const Vec4& color, float size = 4.0f);
    
    // 辅助绘制
    void DrawGrid(float spacing, const Vec4& color);    // 绘制网格
    void DrawAxes(float length);                        // 绘制坐标轴
    void DrawVector(const Vec2& from, const Vec2& to,   // 绘制向量箭头
                    const Vec4& color, float arrowSize);
};
```

### Vertex2D - 2D顶点结构

```cpp
struct Vertex2D {
    float x, y;      // 位置 (NDC坐标)
    float r, g, b, a; // 颜色 (0.0 ~ 1.0)
    float u, v;      // 纹理坐标 (0.0 ~ 1.0)
    
    Vertex2D(float x, float y, float r, float g, float b, float a);
};
```

---

## 🎯 练习建议

1. **观察向量加法**：切换到Vector Operations模式，拖动向量观察加法结果
2. **理解点积投影**：切换到Dot Product模式，观察投影如何随向量变化
3. **观察叉积符号**：切换到Cross Product模式，旋转向量B观察颜色变化
4. **启用动画**：勾选Animate，观察向量关系的连续变化

---

## 📁 本章文件

```
examples/02_triangle/
├── README.md           ← 本文档
└── main.cpp            ← 示例代码
```

---

## 🎯 下一章预告

**第三章：矩阵变换** 将学习：
- Model矩阵（平移、旋转、缩放）
- 投影矩阵（正交投影）
- 像素坐标与NDC坐标的转换
- 可交互的变换控制
