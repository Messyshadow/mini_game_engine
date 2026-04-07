# 第21章：光照系统

## 概述

实现完整的Phong着色模型，支持三种光源类型（方向光、点光源、聚光灯），可通过ImGui实时编辑所有光照参数。

---

## 学习目标

1. 理解Phong反射模型的三个分量（Ambient/Diffuse/Specular）
2. 掌握三种光源类型及其数学公式
3. 理解点光源的距离衰减
4. 理解聚光灯的锥角衰减
5. 学会在Shader中处理多光源
6. 理解法线矩阵（Normal Matrix）为什么必须用逆转置

---

## 数学原理

### 1. Phong反射模型

任何表面上一个点的最终颜色由三个分量相加：

```
最终颜色 = Ambient + Diffuse + Specular
         = 环境光 + 漫反射 + 镜面反射
```

#### Ambient（环境光）

模拟间接光照——光线在场景中多次反弹后的均匀亮度。没有方向，所有点亮度相同。

```
ambient = ambientStrength × lightColor × objectColor
```

ambientStrength通常设为0.1~0.3。太低全黑，太高失去立体感。

#### Diffuse（漫反射）

光线照射到粗糙表面后向各个方向均匀散射。亮度取决于光线与表面法线的夹角。

```
              光线
              ↓  θ
        ------+------  表面
              |
              法线N

diffuse = max(dot(N, L), 0.0) × lightColor × objectColor
```

其中L是光线方向（指向光源），N是表面法线。

**dot(N, L) = cos(θ)**：
- θ = 0°（垂直照射）→ cos = 1.0 → 最亮
- θ = 90°（平行照射）→ cos = 0.0 → 不亮
- θ > 90°（背面）→ cos < 0 → max钳为0

#### Specular（镜面反射）

光线照射到光滑表面后在反射方向上产生的高光亮点。取决于观察者方向。

```
              光线   法线N   反射R
                \     |     /
                 \    |    /     ← 观察者在这里看到高光
                  \   |   /
        ----------+---+---+----------  表面
```

```
R = reflect(-L, N)          // 反射方向
spec = pow(max(dot(V, R), 0.0), shininess)
specular = specularStrength × spec × lightColor
```

V是观察方向（片段指向摄像机），shininess是光泽度：
- shininess = 8 → 大范围柔和高光（哑光材质，如粘土）
- shininess = 32 → 中等高光（塑料）
- shininess = 128 → 小范围锐利高光（金属）
- shininess = 256+ → 极小亮点（镜面）

### 2. 方向光（Directional Light）

模拟太阳——距离无限远，所有光线平行，无衰减。

```
       ↓  ↓  ↓  ↓  ↓  ↓  ↓     平行光线
    ═══════════════════════════   地面
```

只需要一个方向向量，不需要位置。计算最简单。

### 3. 点光源（Point Light）

模拟灯泡——从一个点向所有方向发光，有距离衰减。

```
           ★ 光源位置
          /|\
         / | \
        /  |  \       光线向外扩散
       /   |   \      越远越暗
    ═══════════════   地面
```

#### 衰减公式

```
attenuation = 1.0 / (constant + linear × d + quadratic × d²)
```

| 参数 | 含义 | 效果 |
|------|------|------|
| constant | 常数项（通常=1） | 保证近处衰减不超过1 |
| linear | 线性衰减系数 | 控制中距离衰减速度 |
| quadratic | 二次衰减系数 | 控制远距离衰减速度 |
| d | 光源到片段的距离 | — |

常用参数表（覆盖半径）：

| 半径 | linear | quadratic |
|------|--------|-----------|
| 7 | 0.7 | 1.8 |
| 20 | 0.22 | 0.20 |
| 50 | 0.09 | 0.032 |
| 100 | 0.045 | 0.0075 |

### 4. 聚光灯（Spot Light）

模拟手电筒——有位置、有方向、有锥角限制。

```
         ★ 光源位置
         |
         |  ← 光照方向
        / \
       / θ \     ← θ = 内锥角，光照全强
      /     \
     / φ     \   ← φ = 外锥角，从全强渐变到0
    /         \
```

锥角衰减计算：

```
theta = dot(lightToFrag, spotDirection)   // 当前角度的cos值
epsilon = cos(cutOff) - cos(outerCutOff)  // 内外锥角的cos差
spotIntensity = clamp((theta - cos(outerCutOff)) / epsilon, 0, 1)
```

内锥角内 = 全亮，外锥角外 = 不亮，之间平滑渐变。

### 5. 法线矩阵（Normal Matrix）

**问题**：如果Model矩阵包含非均匀缩放（如X轴缩放2倍），直接用Model变换法线会导致法线不再垂直于表面。

**解决**：法线矩阵 = Model矩阵左上3×3的逆转置矩阵。

```cpp
// C++端预计算（比在GPU算更高效）
Mat3 normalMatrix = model.GetRotationMatrix().Inverse().Transposed();
shader->SetMat3("uNormalMatrix", normalMatrix.m);
```

**为什么是逆转置？** 数学证明：如果M变换一个平面上的向量v，则M的逆转置变换该平面的法线n，能保证变换后n仍垂直于变换后的v。当Model矩阵只包含旋转（正交矩阵）时，逆转置等于自身，可以省略。

---

## Shader解析

### phong.vs（顶点着色器）

```glsl
// 新增：法线矩阵（CPU预计算传入）
uniform mat3 uNormalMatrix;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = vec3(worldPos);           // 传递世界空间位置
    vNormal = uNormalMatrix * aNormal;   // 用法线矩阵变换法线
    gl_Position = uProjection * uView * worldPos;
}
```

对比第19章的basic3d.vs：
- 法线变换从 `mat3(transpose(inverse(uModel)))` 改为CPU预计算的 `uNormalMatrix`，GPU效率更高。

### phong.fs（片段着色器）

关键函数解析：

```glsl
// 方向光计算
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 objColor) {
    vec3 lightDir = normalize(light.direction);  // 归一化光照方向
    
    float diff = max(dot(normal, lightDir), 0.0); // 漫反射（Lambert）
    
    vec3 reflectDir = reflect(-lightDir, normal);  // 反射方向
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess); // Phong高光
    
    vec3 ambient  = uAmbientStrength * light.color;
    vec3 diffuse  = diff * light.color;
    vec3 specular = spec * light.color * 0.5;
    
    return (ambient + diffuse + specular) * objColor * light.intensity;
}
```

```glsl
// 点光源（多了距离衰减）
vec3 CalcPointLight(...) {
    vec3 lightDir = normalize(light.position - fragPos); // 方向=光源位置-片段位置
    
    // ... 漫反射+镜面反射和方向光一样 ...
    
    float dist = length(light.position - fragPos);       // 计算距离
    float attenuation = 1.0 / (constant + linear*dist + quadratic*dist*dist);
    
    // 所有分量都乘以衰减
    return (ambient + diffuse + specular) * attenuation * objColor * intensity;
}
```

```glsl
// 聚光灯（多了锥角检测）
vec3 CalcSpotLight(...) {
    // ... 先算和点光源一样的漫反射+镜面+衰减 ...
    
    float theta = dot(lightDir, normalize(light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float spotIntensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // diffuse和specular乘以spotIntensity（锥角外=0）
    // ambient不乘（环境光不受锥角限制）
}
```

主函数中累加所有光源：

```glsl
void main() {
    vec3 result = vec3(0.0);
    for (int i = 0; i < uNumDirLights; i++)   result += CalcDirLight(...);
    for (int i = 0; i < uNumPointLights; i++) result += CalcPointLight(...);
    for (int i = 0; i < uNumSpotLights; i++)  result += CalcSpotLight(...);
    FragColor = vec4(result, 1.0);
}
```

---

## 操作说明

| 按键 | 功能 |
|------|------|
| 鼠标左键+拖动 | 旋转摄像机 |
| 滚轮 | 缩放 |
| E | 编辑面板 |
| L | 开关所有方向光和点光源 |
| F | 开关聚光灯（手电筒，跟随摄像机） |
| W | 线框模式 |

## ImGui编辑面板

| 标签页 | 可编辑内容 |
|--------|-----------|
| Camera | Yaw/Pitch/Distance/Target/FOV |
| Objects | 每个物体的位置/旋转/缩放/颜色/Shininess |
| Lighting | 全局Ambient + 每个光源的所有参数 + 场景预设（日光/黄昏/夜晚） |
| Render | 背景色/线框/GPU信息/场景统计 |

---

## 场景设计

| 物体 | 材质特点 | Shininess | 观察重点 |
|------|----------|-----------|----------|
| 白色球体 | 高光泽 | 128 | 能清晰看到高光亮点 |
| 红色立方体 | 哑光 | 8 | 高光很模糊分散 |
| 金色球体 | 金属感 | 256 | 极小极锐利的高光 |
| 彩色立方体 | 中等 | 32 | 顶点颜色+光照的配合 |
| X Bot角色 | 中等 | 32 | 复杂几何体上的光照效果 |

红色和蓝色点光源照射在物体上会产生混合色光效果。

---

## 新增文件

| 文件 | 说明 |
|------|------|
| `data/shader/phong.vs` | Phong顶点着色器（法线矩阵变换） |
| `data/shader/phong.fs` | Phong片段着色器（3种光源+Phong反射） |
| `examples/21_lighting_system/main.cpp` | 光照系统演示 |

---

## 思考题

1. 把Shininess从32改到256，高光有什么变化？改到1呢？
2. 把环境光Ambient设为0，场景变成什么样？为什么游戏一般不会设为0？
3. 点光源的linear和quadratic都设为0会怎样？这代表什么物理含义？
4. 聚光灯的内锥角和外锥角相等时（没有柔和过渡），边缘看起来怎样？
5. 如果场景有100个点光源，Shader里的循环会很慢。有什么优化方案？（提示：延迟渲染）

---

## 下一章预告

第22章：材质与纹理 — 纹理贴图加载到3D模型、漫反射贴图、镜面反射贴图、法线贴图（让平面看起来有凹凸细节）。
