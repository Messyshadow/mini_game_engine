# Shader 文件说明

---

## 📁 文件结构

```
data/shader/
├── README.md           ← 本文档
├── basic2d.vs/.fs      ← 基础2D图形渲染
├── sprite.vs/.fs       ← 精灵（带纹理）渲染
├── tilemap.vs/.fs      ← 瓦片地图渲染
└── line.vs/.fs         ← 线条/调试图形渲染
```

---

## 📄 文件命名规范

| 扩展名 | 说明 |
|--------|------|
| `.vs` | Vertex Shader（顶点着色器） |
| `.fs` | Fragment Shader（片段着色器） |

> 也可以使用 `.vert` / `.frag` 或 `.glsl`，根据团队习惯选择

---

## 🎨 着色器用途

### basic2d.vs / basic2d.fs
**基础2D图形渲染**

- 用于：三角形、矩形、圆形等纯色图形
- 输入：位置 + 颜色
- 特点：最简单的着色器，无纹理

```cpp
// 使用示例
shader.LoadFromFile("data/shader/basic2d.vs", "data/shader/basic2d.fs");
shader.SetMat4("uProjection", projection.m);
shader.SetMat4("uModel", model.m);
```

### sprite.vs / sprite.fs
**精灵渲染**

- 用于：带纹理的精灵、角色、UI元素
- 输入：位置 + UV坐标 + 颜色
- 特点：支持纹理采样、颜色混合、透明度裁剪

```cpp
// 使用示例
shader.LoadFromFile("data/shader/sprite.vs", "data/shader/sprite.fs");
shader.SetInt("uTexture", 0);  // 纹理单元0
shader.SetVec4("uColor", 1, 1, 1, 1);  // 白色（原色）
```

### tilemap.vs / tilemap.fs
**瓦片地图渲染**

- 用于：大规模瓦片地图的批量渲染
- 输入：位置 + UV坐标
- 特点：支持视图矩阵（摄像机移动）

```cpp
// 使用示例
shader.LoadFromFile("data/shader/tilemap.vs", "data/shader/tilemap.fs");
shader.SetInt("uTileset", 0);
shader.SetVec4("uTint", 1, 1, 1, 1);
shader.SetMat4("uView", camera.GetViewMatrix().m);
```

### line.vs / line.fs
**线条渲染**

- 用于：调试绘制、网格、坐标轴、向量可视化
- 输入：位置 + 颜色
- 特点：简单直接，适合调试用途

---

## 📐 坐标系统

```
OpenGL NDC坐标系：           像素坐标系（经过投影矩阵转换）：
                           
     Y                           (0, height)
     ↑                              ↑
     │ (0,1)                        │
     │                              │
(-1,0)├───────→ X (1,0)     (0,0) ──┼────────→ (width, 0)
     │                              │
     │ (0,-1)                       │
                                    ↓
                                (0, 0)
```

---

## 🔧 Uniform变量说明

### 矩阵类型

| 名称 | 类型 | 说明 |
|------|------|------|
| `uProjection` | mat4 | 投影矩阵，将世界/像素坐标转换为NDC |
| `uModel` | mat4 | 模型矩阵，物体的变换（平移、旋转、缩放） |
| `uView` | mat4 | 视图矩阵，摄像机的逆变换 |

### 纹理类型

| 名称 | 类型 | 说明 |
|------|------|------|
| `uTexture` | sampler2D | 通用纹理 |
| `uTileset` | sampler2D | 瓦片集纹理 |

### 颜色类型

| 名称 | 类型 | 说明 |
|------|------|------|
| `uColor` | vec4 | 全局颜色乘数 |
| `uTint` | vec4 | 颜色叠加 |

---

## 💡 编写Shader的最佳实践

### 1. 添加注释头
```glsl
/**
 * myshader.vs - 我的着色器
 * 
 * 用途：XXX
 * 作者：XXX
 * 日期：XXX
 */
```

### 2. 分组组织代码
```glsl
// ============================================================================
// 顶点属性输入
// ============================================================================
layout (location = 0) in vec2 aPos;

// ============================================================================
// Uniform变量
// ============================================================================
uniform mat4 uProjection;
```

### 3. 使用有意义的变量名
```glsl
// 好的命名
in vec2 aPos;           // a = attribute（属性）
out vec2 vTexCoord;     // v = varying（传递变量）
uniform mat4 uModel;    // u = uniform

// 避免
in vec2 p;
out vec2 t;
uniform mat4 m;
```

### 4. 常量定义
```glsl
const float PI = 3.14159265359;
const float EPSILON = 0.0001;
```

---

## 🐛 调试技巧

### 1. 输出纯色检测着色器是否工作
```glsl
void main() {
    FragColor = vec4(1.0, 0.0, 1.0, 1.0); // 品红色
}
```

### 2. 可视化UV坐标
```glsl
void main() {
    FragColor = vec4(vTexCoord, 0.0, 1.0); // RG = UV
}
```

### 3. 可视化法线（3D用）
```glsl
void main() {
    FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
}
```

---

## 📚 学习资源

- [Learn OpenGL - Shaders](https://learnopengl.com/Getting-started/Shaders)
- [The Book of Shaders](https://thebookofshaders.com/)
- [Shadertoy](https://www.shadertoy.com/) - 在线Shader实验平台
