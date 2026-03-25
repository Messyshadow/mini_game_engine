# 第三章：矩阵变换

---

## 📚 本章概述

本章将深入学习2D图形变换的数学基础：
1. Model矩阵的构建（平移、旋转、缩放）
2. 正交投影矩阵
3. 像素坐标系与NDC坐标系的转换
4. 变换的组合顺序

---

## 🎯 学习目标

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           本章核心知识点                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. 变换流水线 (Transformation Pipeline)                                     │
│                                                                             │
│     本地坐标 ──→ Model矩阵 ──→ 世界坐标 ──→ Projection ──→ NDC坐标          │
│      (0,0)       (TRS)       (640,360)      (Ortho)      (-1~1)            │
│                                                                             │
│  2. TRS变换顺序                                                              │
│                                                                             │
│     Model = Translation × Rotation × Scale                                  │
│                                                                             │
│     实际应用顺序（从右到左）：                                                │
│     ① 先缩放 (Scale)     - 改变大小                                         │
│     ② 再旋转 (Rotation)  - 围绕原点旋转                                      │
│     ③ 后平移 (Translation) - 移动到目标位置                                  │
│                                                                             │
│  3. 正交投影 (Orthographic Projection)                                       │
│                                                                             │
│     将像素坐标 [0, width] × [0, height] 映射到 NDC [-1, 1] × [-1, 1]        │
│                                                                             │
│     像素坐标系：              NDC坐标系：                                     │
│     (0,720)────(1280,720)    (-1,1)────(1,1)                                │
│        │           │            │         │                                 │
│        │           │     ──→    │         │                                 │
│        │           │            │         │                                 │
│     (0,0)────(1280,0)        (-1,-1)───(1,-1)                               │
│                                                                             │
│  4. 变换矩阵详解                                                             │
│                                                                             │
│     平移矩阵：        旋转矩阵：           缩放矩阵：                          │
│     | 1 0 tx |       | cos -sin 0 |       | sx 0  0 |                      │
│     | 0 1 ty |       | sin  cos 0 |       | 0  sy 0 |                      │
│     | 0 0 1  |       | 0    0   1 |       | 0  0  1 |                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 💻 代码结构

### 文件清单

```
03_transform/
└── main.cpp          ← 示例代码（约282行）
```

### 核心代码解析

#### 1. 创建正交投影矩阵

```cpp
// 将像素坐标映射到NDC
// 参数：左、右、下、上 边界
g_projection = Mat4::Ortho2D(0.0f, (float)width, 0.0f, (float)height);

// 内部实现：
// | 2/(r-l)    0       -(r+l)/(r-l) |
// |   0     2/(t-b)    -(t+b)/(t-b) |
// |   0        0            1       |
```

#### 2. 构建Model矩阵

```cpp
// TRS变换：平移 × 旋转 × 缩放
// 注意：矩阵乘法从右到左应用，所以实际顺序是 缩放→旋转→平移
Mat4 model = Mat4::TRS2D(g_position, g_rotation, Vec2(1.0f, 1.0f));

// 等价于：
// Mat4 T = Mat4::Translation2D(g_position);
// Mat4 R = Mat4::Rotation2D(g_rotation);
// Mat4 S = Mat4::Scale2D(scale);
// Mat4 model = T * R * S;
```

#### 3. 变换顶点坐标

```cpp
Vec2 TransformVertex(const Vec2& localPos, const Mat4& model, const Mat4& projection) {
    // 步骤1：本地坐标 → 世界坐标（应用Model矩阵）
    Vec4 worldPos = model * Vec4(localPos, 0.0f, 1.0f);
    
    // 步骤2：世界坐标 → NDC（应用Projection矩阵）
    Vec4 clipPos = projection * worldPos;
    
    return Vec2(clipPos.x, clipPos.y);
}
```

#### 4. 绘制变换后的正方形

```cpp
// 定义本地坐标（以中心为原点）
float halfW = g_scale.x / 2.0f;
float halfH = g_scale.y / 2.0f;

Vec2 localVerts[4] = {
    Vec2(-halfW, -halfH),  // 左下
    Vec2( halfW, -halfH),  // 右下
    Vec2( halfW,  halfH),  // 右上
    Vec2(-halfW,  halfH),  // 左上
};

// 变换每个顶点
Vec2 transformedVerts[4];
for (int i = 0; i < 4; i++) {
    transformedVerts[i] = TransformVertex(localVerts[i], model, g_projection);
}

// 绘制（用两个三角形）
g_renderer->DrawTriangle(transformedVerts[0], transformedVerts[1], transformedVerts[2], fillColor);
g_renderer->DrawTriangle(transformedVerts[0], transformedVerts[2], transformedVerts[3], fillColor);
```

---

## 🎮 操作说明

| 按键 | 功能 |
|------|------|
| W/A/S/D | 上下左右移动精灵 |
| Q | 逆时针旋转 |
| E | 顺时针旋转 |
| Auto Rotate | 自动旋转开关 |
| Reset | 重置所有变换 |
| ESC | 退出程序 |

---

## 📊 界面说明

```
┌─────────────────────────────────────────────────────────────────┐
│                        渲染窗口                                  │
│  ┌────────────────────────────────────────────────────────┐    │
│  │ 100px网格                                               │    │
│  │    │    │    │    │    │    │    │    │    │    │      │    │
│  │ ───┼────┼────┼────┼────┼────┼────┼────┼────┼────┼───  │    │
│  │    │    │    │    │  ╱╲│    │    │    │    │    │      │    │
│  │ ───┼────┼────┼────┼─╱──╲────┼────┼────┼────┼────┼───  │    │
│  │    │    │    │    │╱ ● ╲    │    │    │    │    │      │    │
│  │ ───┼────┼────┼────╱─────╲───┼────┼────┼────┼────┼───  │    │
│  │    │    │    │   ╱   →   ╲  │    │    │    │    │      │    │
│  │ ───┼────┼────┼──╱─────────╲─┼────┼────┼────┼────┼───  │    │
│  │    │    │    │              │    │    │    │    │      │    │
│  └────────────────────────────────────────────────────────┘    │
│   ↑ X轴红色    Y轴绿色    精灵中心黄点    前向红线               │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────┐  ┌─────────────────────────┐
│ Transform Demo          │  │ Matrix Info             │
├─────────────────────────┤  ├─────────────────────────┤
│ FPS: 60.0               │  │ Model Matrix (TRS):     │
│ Window: 1280 x 720      │  │ |  0.87  0.50  0  640 | │
│ ─────────────────────── │  │ | -0.50  0.87  0  360 | │
│ Controls: WASD, Q/E     │  │ |  0     0     1    0 | │
│ ─────────────────────── │  │ |  0     0     0    1 | │
│ Transform:              │  │ ─────────────────────── │
│ Position: [640] [360]   │  │ Projection (Ortho2D):  │
│ Rotation: [30.0] deg    │  │ | 0.002  0     0   -1 | │
│ Size: [100] [100]       │  │ | 0      0.003 0   -1 | │
│ ─────────────────────── │  │ | 0      0     1    0 | │
│ [✓] Auto Rotate         │  │ | 0      0     0    1 | │
│ Speed: [====1.0====]    │  │                         │
│ ─────────────────────── │  │ Pipeline:               │
│ [Reset]                 │  │ Local→Model→World→     │
│                         │  │ Projection→NDC          │
└─────────────────────────┘  └─────────────────────────┘
```

---

## 📐 变换数学详解

### 为什么变换顺序是 T × R × S？

```
    矩阵乘法从右到左应用，所以 M = T × R × S 的效果是：
    
    V' = M × V = T × R × S × V
                        ↑
                        先应用S（缩放）
                    ↑
                    再应用R（旋转）
                ↑
                最后应用T（平移）
    
    为什么这个顺序重要？
    
    错误顺序 (先平移再旋转)：         正确顺序 (先旋转再平移)：
    
         ↗ 旋转后飞到奇怪位置                    ↗ 旋转后位置正确
        ╱                                       ╱
    ───●───→ 先平移到这里               ───────●───→ 在原点旋转
       ↑                                        ↑
       原点                                      原点，然后平移
```

### 2D旋转矩阵推导

```
    将点 (x, y) 绕原点旋转角度 θ：
    
    x' = x×cos(θ) - y×sin(θ)
    y' = x×sin(θ) + y×cos(θ)
    
    写成矩阵形式：
    
    | x' |   | cos(θ)  -sin(θ) |   | x |
    | y' | = | sin(θ)   cos(θ) | × | y |
    
              ↑ 这就是2D旋转矩阵
```

### 正交投影矩阵推导

```
    将 [left, right] × [bottom, top] 映射到 [-1, 1] × [-1, 1]：
    
    对于X坐标：
    x' = 2/(right-left) × x - (right+left)/(right-left)
    
    对于Y坐标：
    y' = 2/(top-bottom) × y - (top+bottom)/(top-bottom)
    
    简化版 Ortho2D(0, width, 0, height)：
    x' = 2/width × x - 1
    y' = 2/height × y - 1
```

---

## 🔧 关键类说明

### Mat4 - 4x4矩阵类

```cpp
struct Mat4 {
    float m[16];  // 列主序存储
    
    // 静态工厂方法
    static Mat4 Identity();                    // 单位矩阵
    static Mat4 Translation2D(const Vec2& t);  // 平移矩阵
    static Mat4 Rotation2D(float radians);     // 旋转矩阵
    static Mat4 Scale2D(const Vec2& s);        // 缩放矩阵
    static Mat4 TRS2D(const Vec2& t, float r, const Vec2& s);  // 组合变换
    static Mat4 Ortho2D(float left, float right, float bottom, float top);
    static Mat4 Ortho(float l, float r, float b, float t, float n, float f);
    
    // 运算符
    Mat4 operator*(const Mat4& other) const;   // 矩阵乘法
    Vec4 operator*(const Vec4& v) const;       // 矩阵×向量
};
```

### 坐标系统对比

| 坐标系 | X范围 | Y范围 | 原点位置 | 用途 |
|--------|-------|-------|----------|------|
| 本地坐标 | 任意 | 任意 | 物体中心 | 定义物体形状 |
| 世界/像素坐标 | 0~1280 | 0~720 | 左下角 | 物体在场景中的位置 |
| NDC | -1~1 | -1~1 | 屏幕中心 | GPU渲染 |

---

## 🎯 练习建议

1. **观察变换顺序**：注释掉TRS中的某个变换，观察效果
2. **理解投影**：修改窗口大小，观察投影矩阵的变化
3. **组合变换**：尝试创建更复杂的变换（如绕任意点旋转）
4. **观察矩阵值**：在Matrix Info面板观察矩阵随变换的变化

---

## 📁 本章文件

```
examples/03_transform/
├── README.md           ← 本文档
└── main.cpp            ← 示例代码
```

---

## 🎯 下一章预告

**第四章：纹理与精灵** 将学习：
- 纹理加载和使用
- 精灵(Sprite)的概念
- UV坐标和纹理映射
- 精灵图集(Sprite Sheet)
