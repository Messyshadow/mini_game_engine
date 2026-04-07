# 第19章：3D渲染基础

## 概述

从2D迈入3D的第一步。本章学习透视投影、MVP矩阵管线、深度测试等3D渲染的核心概念，并渲染出旋转的彩色立方体和球体。

---

## 学习目标

1. 理解3D坐标系（OpenGL右手系）
2. 掌握透视投影矩阵的原理
3. 理解完整的MVP变换管线
4. 学会深度测试和背面剔除
5. 构建Mesh3D类（顶点数据+OpenGL缓冲管理）
6. 编写3D着色器（顶点颜色+简单光照）

---

## 数学原理

### 1. 3D坐标系

OpenGL使用**右手坐标系**：

```
        Y (上)
        |
        |
        |_____ X (右)
       /
      /
     Z (指向屏幕外，朝向你)
```

对比2D：2D只有X（右）和Y（上），3D增加了Z轴（深度）。

**右手定则**：右手四指从X轴弯向Y轴，拇指指向的方向就是Z轴正方向。

### 2. MVP变换管线

3D渲染的核心——将3D世界中的一个点最终映射到2D屏幕上：

```
模型空间                世界空间              摄像机空间             裁剪空间              屏幕
(0,1,0)  ──Model──→  (3,5,2)  ──View──→  (1,3,-5)  ──Proj──→  (0.2,0.6,0.9,1)  ──→  (640,360)
           位移                   摄像机               透视投影                    视口变换
           旋转                   位置朝向             近大远小
           缩放
```

在着色器中一行搞定：
```glsl
gl_Position = Projection * View * Model * vec4(position, 1.0);
```

**注意**：矩阵乘法从右往左读——先Model，再View，最后Projection。

### 3. 模型矩阵（Model Matrix）

将物体从**自身坐标系**变换到**世界坐标系**。

组合方式：平移 × 旋转 × 缩放（注意顺序！）

```cpp
Mat4 model = Mat4::Translate(posX, posY, posZ)    // 3. 最后平移
           * Mat4::RotateY(angle)                  // 2. 然后旋转
           * Mat4::Scale(scaleX, scaleY, scaleZ);  // 1. 先缩放
```

**为什么这个顺序？** 因为矩阵乘法是从右往左作用的。如果先平移再旋转，物体会绕世界原点旋转而不是自身中心旋转。

### 4. 视图矩阵（View Matrix）

将世界坐标变换到**摄像机坐标系**。本质是"把整个世界反向移动和旋转，让摄像机在原点朝-Z看"。

用LookAt函数生成：
```cpp
Mat4 view = Mat4::LookAt(
    eye,    // 摄像机位置，如(0, 5, 10)
    target, // 看向的点，如(0, 0, 0)
    up      // 上方向，通常(0, 1, 0)
);
```

**LookAt内部原理**：

1. 计算前方向：`forward = normalize(eye - target)`
2. 计算右方向：`right = normalize(cross(up, forward))`
3. 计算真实上方向：`realUp = cross(forward, right)`
4. 构造旋转矩阵+平移矩阵的组合

### 5. 投影矩阵（Projection Matrix）

#### 正交投影 vs 透视投影

```
正交投影（2D用的）：          透视投影（3D用的）：
    ┌────────┐                   \        /
    │        │                    \      /
    │   立方  │  → 无近大远小       \    /  → 有近大远小
    │   体    │                    \  /     更接近人眼
    └────────┘                     \/
```

#### 透视投影矩阵

```cpp
Mat4 proj = Mat4::Perspective(fov, aspect, near, far);
```

四个参数含义：

| 参数 | 含义 | 典型值 |
|------|------|--------|
| fov | 视野角度（弧度） | 45°=0.785 |
| aspect | 宽高比 | 1280/720=1.778 |
| near | 近裁面距离 | 0.1 |
| far | 远裁面距离 | 100.0 |

**透视投影矩阵的数学**（参考《3D数学基础》）：

```
设 f = 1/tan(fov/2), a = aspect, n = near, far = far

        ┌  f/a   0      0              0     ┐
Proj =  │   0    f      0              0     │
        │   0    0   (f+n)/(n-f)   2fn/(n-f) │
        │   0    0     -1              0     │
        └                                    ┘
```

这个矩阵做了两件事：
1. 将视锥体（frustum）变形为标准立方体
2. 将Z值映射到[-1,1]范围（OpenGL NDC）

### 6. 深度测试（Depth Test）

**问题**：3D中物体有前后关系，后面的物体不应该画在前面的上面。

**解决**：深度缓冲（Z-Buffer）。每个像素记录当前最近的Z值，新像素只有更近时才会覆盖。

```cpp
glEnable(GL_DEPTH_TEST);  // 启用深度测试
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 每帧清除深度缓冲
```

**如果忘了启用深度测试**：后绘制的物体永远覆盖先绘制的，不管远近，画面会乱套。

### 7. 背面剔除（Back-face Culling）

**优化**：立方体的背面永远看不到，不需要渲染。

```cpp
glEnable(GL_CULL_FACE);   // 启用背面剔除
glCullFace(GL_BACK);       // 剔除背面
```

OpenGL通过顶点的绕序（winding order）判断正面/背面：
- 逆时针（CCW）= 正面（默认）
- 顺时针（CW）= 背面（被剔除）

---

## Shader解析

### 顶点着色器 (basic3d.vs)

```glsl
#version 330 core

// 输入：从VBO读取的顶点属性
layout (location = 0) in vec3 aPos;      // 顶点位置
layout (location = 1) in vec3 aNormal;   // 顶点法线
layout (location = 2) in vec3 aColor;    // 顶点颜色
layout (location = 3) in vec2 aTexCoord; // UV坐标

// 输出：传递给片段着色器
out vec3 vColor;    // 颜色
out vec3 vNormal;   // 法线（世界空间）
out vec3 vFragPos;  // 片段位置（世界空间）

// Uniform：每帧由CPU设置的全局变量
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    // 1. 将顶点位置变换到世界空间
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    
    // 2. MVP变换：世界空间 → 裁剪空间
    gl_Position = uProjection * uView * worldPos;
    
    // 3. 传递数据
    vColor = aColor;
    vFragPos = vec3(worldPos);
    
    // 4. 法线变换（关键！）
    // 不能直接用Model矩阵变换法线——非均匀缩放会导致法线不再垂直于表面
    // 需要用Model矩阵的"逆转置矩阵"
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
}
```

**为什么法线不能直接用Model矩阵变换？**

假设一个平面法线朝上(0,1,0)，对平面做X轴缩放2倍：
- 如果直接用Model变换法线，法线还是(0,1,0)，看起来没问题
- 但如果是斜面，缩放后法线方向就不再垂直于表面了

用逆转置矩阵（inverse transpose）可以保证法线始终垂直于表面。

### 片段着色器 (basic3d.fs)

```glsl
#version 330 core

in vec3 vColor;
in vec3 vNormal;
in vec3 vFragPos;

out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uAmbientStrength;
uniform bool uUseLighting;
uniform bool uUseVertexColor;
uniform vec4 uBaseColor;

void main() {
    // 确定物体基础颜色
    vec3 objectColor = uUseVertexColor ? vColor : uBaseColor.rgb;
    
    if (uUseLighting) {
        // 环境光：无论有没有直接光照都存在的基础亮度
        vec3 ambient = uAmbientStrength * uLightColor;
        
        // 漫反射（Lambertian反射）：
        // 光越垂直于表面，反射越强
        // dot(normal, lightDir) = cos(θ)
        vec3 norm = normalize(vNormal);
        vec3 lightDir = normalize(uLightDir);
        float diff = max(dot(norm, lightDir), 0.0);  // 负值钳为0
        vec3 diffuse = diff * uLightColor;
        
        // 合并
        vec3 result = (ambient + diffuse) * objectColor;
        FragColor = vec4(result, 1.0);
    } else {
        FragColor = vec4(objectColor, 1.0);
    }
}
```

**Lambertian漫反射原理**：

```
          光线
          ↓  θ
    ------+------  表面
          |
          法线

当θ=0°（垂直入射）：dot = cos(0) = 1.0 → 最亮
当θ=90°（平行入射）：dot = cos(90) = 0.0 → 不亮
当θ>90°（从背面来）：dot < 0 → 钳为0（max防止负值）
```

---

## 新增文件

| 文件 | 说明 |
|------|------|
| `source/engine3d/Mesh3D.h` | 3D网格类（Vertex3D结构、VAO/VBO/EBO管理） |
| `source/engine3d/Mesh3D.cpp` | 网格实现（含立方体/平面/球体生成） |
| `data/shader/basic3d.vs` | 3D顶点着色器（MVP变换+法线变换） |
| `data/shader/basic3d.fs` | 3D片段着色器（顶点颜色+Lambertian光照） |
| `examples/19_3d_basics/main.cpp` | 3D渲染演示+ImGui编辑 |

---

## 操作说明

| 按键/操作 | 功能 |
|-----------|------|
| 鼠标右键+拖动 | 旋转摄像机视角 |
| 鼠标滚轮 | 缩放（改变摄像机距离） |
| E | 显示/隐藏ImGui编辑面板 |
| 1 | 只显示立方体 |
| 2 | 只显示球体 |
| 3 | 显示全部 |
| L | 切换光照开关 |
| W | 切换线框模式 |

## ImGui编辑面板（E键）

| 标签页 | 可编辑内容 |
|--------|-----------|
| Camera | Yaw/Pitch/Distance、FOV、近远裁面、目标点 |
| Objects | 每个物体的位置/旋转/缩放/颜色/自动旋转速度 |
| Lighting | 光照方向/颜色/环境光强度、预设（日光/黄昏/月光） |
| Render | 背景颜色、线框模式、显示模式、OpenGL信息 |

---

## 思考题

1. 修改FOV到120°和10°分别是什么效果？为什么FPS游戏通常用90°而不是45°？
2. 如果去掉 `glEnable(GL_DEPTH_TEST)`，画面会变成什么样？
3. 把 `glCullFace(GL_BACK)` 改成 `GL_FRONT`，你会看到什么？
4. 为什么Mesh3D的立方体需要24个顶点而不是8个？（提示：法线不同）
5. 法线变换为什么要用逆转置矩阵？在什么情况下可以省略 `transpose(inverse())`？

---

## 与2D的关键区别

| 概念 | 2D | 3D |
|------|-----|-----|
| 坐标轴 | X, Y | X, Y, Z |
| 投影 | Ortho（正交） | Perspective（透视） |
| 深度 | 不需要 | GL_DEPTH_TEST |
| 法线 | 不需要 | 光照必须 |
| 背面剔除 | 不需要 | GL_CULL_FACE |
| 清除缓冲 | 只清颜色 | 清颜色+深度 |
| 摄像机 | 2D偏移 | LookAt矩阵 |

---

## 下一章预告

第20章：3D模型加载 — 集成Assimp库，加载真正的3D模型（OBJ/FBX），替代手写的立方体和球体。届时需要你下载Assimp库和Mixamo角色模型。
