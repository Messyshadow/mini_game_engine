# 第22章：材质与纹理

## 概述

将纹理贴图应用到3D物体上，实现真实感渲染。学习漫反射贴图、法线贴图、粗糙度贴图的原理和应用。

---

## 学习目标

1. 理解纹理映射的原理（UV坐标）
2. 掌握漫反射贴图（Diffuse Map）的使用
3. 理解法线贴图（Normal Map）如何在不增加多边形的情况下添加表面细节
4. 理解粗糙度贴图（Roughness Map）对高光的影响
5. 学会TBN矩阵的计算和切线空间变换
6. 掌握OpenGL多纹理单元的使用

---

## 数学原理

### 1. UV坐标与纹理映射

每个3D顶点除了位置和法线，还有一组UV坐标（也叫纹理坐标），指定该顶点对应纹理图上的哪个位置。

```
纹理图（2D图片）          3D物体表面
   V                        
   1 ┌──────────┐           ┌──────┐
     │          │           │      │
     │  ★(0.3, │    映射    │  ★   │
     │    0.7)  │  ──────→  │      │
     │          │           │      │
   0 └──────────┘           └──────┘
     0          1 U
```

UV值范围通常是[0,1]。超出范围时OpenGL可以设置GL_REPEAT（平铺重复）。

### 2. 漫反射贴图（Diffuse Map / Color Map）

最基础的纹理——存储表面颜色。在Shader中替代物体的纯色：

```glsl
// 不用纹理
vec3 albedo = uBaseColor.rgb;

// 用漫反射贴图
vec3 albedo = texture(uDiffuseMap, uv).rgb;  // 从纹理采样颜色
```

### 3. 法线贴图（Normal Map）

**核心问题**：如何让一个平坦的表面看起来有凹凸细节，而不增加多边形？

**解决方案**：在纹理中存储每个像素的法线方向，代替顶点的法线进行光照计算。

```
   没有法线贴图：                有法线贴图：
   平坦的光照                    凹凸感的光照（实际几何仍然是平的）
   ┌──────────┐                ┌──────────┐
   │          │                │ ▓░▓░▓░▓░ │
   │          │                │ ░▓░▓░▓░▓ │
   │          │                │ ▓░▓░▓░▓░ │
   └──────────┘                └──────────┘
```

#### 切线空间（Tangent Space）

法线贴图中的法线存储在**切线空间**中，而非世界空间。这样同一张法线贴图可以贴到不同朝向的面上。

```
切线空间坐标系：
  T (Tangent)  — 沿纹理U方向（通常是右）
  B (Bitangent) — 沿纹理V方向（通常是上）
  N (Normal)    — 表面法线方向（通常是外）

蓝紫色 (0.5, 0.5, 1.0) 在切线空间 = (0, 0, 1) = 指向N方向 = 平坦表面
偏红色 = X方向有偏移 = 向左或向右倾斜
偏绿色 = Y方向有偏移 = 向上或向下倾斜
```

#### TBN矩阵

将切线空间法线转换到世界空间：

```glsl
// 从法线贴图读取切线空间法线
vec3 tangentNormal = texture(uNormalMap, uv).rgb;
tangentNormal = tangentNormal * 2.0 - 1.0;  // [0,1] → [-1,1]

// TBN矩阵：切线空间 → 世界空间
vec3 worldNormal = normalize(TBN * tangentNormal);
```

TBN矩阵的三列分别是T（切线）、B（副切线）、N（法线），都在世界空间中。

### 4. 粗糙度贴图（Roughness Map）

灰度图，控制表面每个像素的粗糙程度：

```
黑色(0.0) = 完全光滑 = shininess很高 = 极锐利的高光（镜面）
白色(1.0) = 完全粗糙 = shininess很低 = 几乎没有高光（哑光）
```

转换公式：

```glsl
float roughness = texture(uRoughnessMap, uv).r;  // 0~1
float shininess = max(2.0, (1.0 - roughness) * 256.0);
```

### 5. 纹理平铺（Tiling）

将UV坐标乘以一个系数，纹理就会在表面上重复：

```glsl
vec2 uv = vTexCoord * uTexTiling;  // tiling=2 → 纹理重复2次
```

配合GL_REPEAT包裹模式，超出[0,1]范围的UV会自动重复。

### 6. OpenGL多纹理单元

OpenGL可以同时绑定多个纹理到不同的"插槽"（纹理单元）：

```cpp
// CPU端
glActiveTexture(GL_TEXTURE0);    // 激活0号插槽
glBindTexture(GL_TEXTURE_2D, diffuseTexID);
glActiveTexture(GL_TEXTURE1);    // 激活1号插槽
glBindTexture(GL_TEXTURE_2D, normalTexID);
glActiveTexture(GL_TEXTURE2);    // 激活2号插槽
glBindTexture(GL_TEXTURE_2D, roughnessTexID);

// 告诉Shader每个sampler2D对应哪个插槽
shader->SetInt("uDiffuseMap", 0);
shader->SetInt("uNormalMap", 1);
shader->SetInt("uRoughnessMap", 2);
```

---

## Shader解析

### material.vs（顶点着色器）新增TBN计算

```glsl
// 简化版TBN计算（基于法线生成切线）
vec3 N = vNormal;
vec3 T;
if (abs(N.y) < 0.999)
    T = normalize(cross(N, vec3(0, 1, 0)));  // 和上方向叉乘得到切线
else
    T = normalize(cross(N, vec3(1, 0, 0)));  // 法线朝上时用右方向
vec3 B = normalize(cross(N, T));             // 副切线 = 法线 × 切线
vTBN = mat3(T, B, N);                       // 组合成TBN矩阵
```

### material.fs（片段着色器）核心流程

```glsl
void main() {
    // 1. 从漫反射贴图获取颜色
    vec3 albedo = texture(uDiffuseMap, uv * tiling).rgb;
    
    // 2. 从法线贴图获取法线（切线空间→世界空间）
    vec3 normalTS = texture(uNormalMap, uv * tiling).rgb * 2.0 - 1.0;
    vec3 normal = normalize(TBN * normalTS);
    
    // 3. 从粗糙度贴图获取shininess
    float roughness = texture(uRoughnessMap, uv * tiling).r;
    float shininess = (1.0 - roughness) * 256.0;
    
    // 4. 用这些数据做Phong光照计算
    // ... 和第21章一样 ...
}
```

---

## 操作说明

| 按键 | 功能 |
|------|------|
| 鼠标左键拖动 | 旋转摄像机 |
| 滚轮 | 缩放 |
| E | 编辑面板 |
| 1~5 | 切换选中材质 |
| N | 开关法线贴图（对比有无的效果差异） |
| L | 开关光照 |
| W | 线框模式 |

## ImGui编辑面板

| 标签页 | 内容 |
|--------|------|
| Camera | 摄像机参数 |
| Objects | 每个物体的位置/旋转/缩放/平铺次数/材质选择 |
| Materials | 每个材质的Shininess/Tiling，贴图加载状态 |
| Lighting | 方向光+点光源参数 |
| Render | 背景色/线框/GPU信息 |

---

## 场景设计

| 物体 | 材质 | 观察重点 |
|------|------|----------|
| 地面 | 木地板 | 纹理平铺效果 |
| 左侧立方体 | 砖墙 | **按N对比法线贴图效果**——开启时砖缝有明显凹凸 |
| 右侧球体 | 金属 | 粗糙度贴图影响高光分布 |
| 背景墙 | 金属板 | 大面积纹理 |
| 前方球体 | 木板 | 木纹纹理+法线细节 |

---

## 新增文件

| 文件 | 说明 |
|------|------|
| `source/engine3d/Texture3D.h` | 3D纹理加载器 + Material3D材质类 |
| `data/shader/material.vs` | 材质顶点着色器（TBN矩阵计算） |
| `data/shader/material.fs` | 材质片段着色器（漫反射+法线+粗糙度贴图） |
| `data/texture/materials/bricks/` | 砖墙材质（diffuse/normal/roughness） |
| `data/texture/materials/wood/` | 木板材质 |
| `data/texture/materials/metal/` | 金属材质 |
| `data/texture/materials/metalplates/` | 金属板材质 |
| `data/texture/materials/woodfloor/` | 木地板材质 |

---

## 思考题

1. 按N键关闭法线贴图后，砖墙立方体的外观有什么变化？为什么？
2. 把Tiling从1改到5，纹理有什么变化？改到0.5呢？
3. 法线贴图为什么看起来是蓝紫色的？纯蓝色(0,0,1)在切线空间代表什么？
4. 如果不用TBN矩阵，直接把法线贴图的值当世界空间法线用，会出什么问题？
5. 粗糙度为0的金属表面和粗糙度为1的有什么视觉区别？

---

## 下一章预告

第23章：3D摄像机 — FPS第一人称摄像机（WASD+鼠标）和TPS第三人称摄像机（跟随角色），可切换模式。
