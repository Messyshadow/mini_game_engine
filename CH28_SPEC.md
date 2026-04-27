# 第28章开发规范：场景与关卡

## 必读：先读 PROJECT_CONTEXT.md、ROADMAP_CH26_30.md、LOCAL_ASSETS.md

## 第28章三大核心

### A. 天空盒（Skybox）
### B. 3D粒子系统
### C. 场景管理与触发器

---

## A. 天空盒

### 需要创建的文件
- `source/engine3d/Skybox.h` — 天空盒类
- `data/shader/skybox.vs` — 天空盒顶点着色器
- `data/shader/skybox.fs` — 天空盒片段着色器

### 天空盒原理
用一个大立方体包裹整个场景，6个面贴上天空纹理。摄像机永远在立方体中心（去掉View矩阵的平移部分）。

### Shader代码

```glsl
// skybox.vs
#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 vTexCoord;
uniform mat4 uView;
uniform mat4 uProjection;
void main() {
    vTexCoord = aPos;
    // 去掉View矩阵的平移，只保留旋转（摄像机永远在中心）
    mat4 viewNoTranslation = mat4(mat3(uView));
    vec4 pos = uProjection * viewNoTranslation * vec4(aPos, 1.0);
    gl_Position = pos.xyww;  // z=w 保证天空盒在最远处
}

// skybox.fs
#version 330 core
in vec3 vTexCoord;
out vec4 FragColor;
uniform samplerCube uSkybox;
void main() {
    FragColor = texture(uSkybox, vTexCoord);
}
```

### 天空盒纹理
如果没有6面CubeMap纹理，可以用代码生成简单的渐变天空：
- 上面：深蓝
- 侧面：浅蓝渐变到白色
- 下面：灰绿色（地面色）

或者从 https://polyhaven.com/hdris 下载免费HDR天空，转换为6面PNG。

如果下载不方便，用纯色天空盒也可以（通过shader直接根据方向计算颜色，不需要纹理）：
或者我这里现成的天空盒纹理：天空盒纹理位置：E:\SourceCode\Games\game\RacingGames\asset\textures

```glsl
// 程序化天空（不需要纹理）
void main() {
    vec3 dir = normalize(vTexCoord);
    float t = dir.y * 0.5 + 0.5;  // -1~1 映射到 0~1
    vec3 bottomColor = vec3(0.8, 0.85, 0.9);  // 地平线：浅灰蓝
    vec3 topColor = vec3(0.2, 0.4, 0.8);      // 天顶：深蓝
    FragColor = vec4(mix(bottomColor, topColor, t), 1.0);
}
```

### 渲染顺序
1. 先渲染天空盒（关闭深度写入 glDepthMask(GL_FALSE)）
2. 再渲染场景（开启深度写入）

---

## B. 3D粒子系统

### 需要创建的文件
- `source/engine3d/ParticleSystem3D.h` — 3D粒子系统
- `data/shader/particle3d.vs` — 粒子顶点着色器
- `data/shader/particle3d.fs` — 粒子片段着色器

### 粒子系统设计

```cpp
struct Particle3D {
    Vec3 position;
    Vec3 velocity;
    Vec4 color;
    float size;
    float life;      // 剩余生命（秒）
    float maxLife;
    bool active = false;
};

class ParticleEmitter3D {
public:
    Vec3 position;           // 发射器位置
    Vec3 direction;          // 发射方向
    float spread = 0.5f;     // 扩散角度
    float emitRate = 20;     // 每秒发射数量
    float particleLife = 1.5f;
    float particleSize = 0.2f;
    Vec4 startColor = Vec4(1, 0.8f, 0.3f, 1);
    Vec4 endColor = Vec4(1, 0.2f, 0, 0);
    float startSpeed = 5.0f;
    float gravity = -2.0f;
    int maxParticles = 200;
    
    void Update(float dt);
    void Draw(Shader* shader, const Mat4& view, const Mat4& proj);
};
```

### 粒子Billboard（始终面向摄像机）

```glsl
// particle3d.vs
// 从View矩阵提取摄像机的右方向和上方向
vec3 camRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
vec3 camUp = vec3(uView[0][1], uView[1][1], uView[2][1]);

// 用摄像机方向构造面向摄像机的四边形
vec3 worldPos = uParticlePos + camRight * aPos.x * uSize + camUp * aPos.y * uSize;
gl_Position = uProjection * uView * vec4(worldPos, 1.0);
```

### 预设粒子效果
- 攻击命中火花（橙红色，快速扩散消散）
- 角色冲刺残影（蓝色拖尾）
- 环境落叶/灰尘（慢速飘落）

---

## C. 场景管理与触发器

### 需要创建的文件
- `source/engine3d/SceneManager.h` — 场景管理器

### 场景管理

```cpp
struct Scene3D {
    std::string name;
    std::vector<SceneObj> objects;
    std::vector<EnemyAI> enemies;
    Vec3 playerSpawn;
    Vec3 cameraInitPos;
    // 光照配置
    DirLightData sunLight;
    float ambient;
};

class SceneManager {
public:
    void LoadScene(int index);
    void UnloadScene();
    Scene3D& GetCurrentScene();
    int GetSceneCount();
};
```

### 触发器系统

```cpp
struct Trigger3D {
    Vec3 position;
    Vec3 halfSize;  // AABB半尺寸
    bool triggered = false;
    std::string action;  // "next_scene" / "spawn_enemy" / "play_sound"
    
    bool Contains(Vec3 point) {
        return abs(point.x - position.x) < halfSize.x &&
               abs(point.y - position.y) < halfSize.y &&
               abs(point.z - position.z) < halfSize.z;
    }
};
```

---

## 第28章需要创建的文件清单

1. `source/engine3d/Skybox.h` — 天空盒
2. `source/engine3d/ParticleSystem3D.h` — 3D粒子系统
3. `source/engine3d/SceneManager.h` — 场景管理器+触发器
4. `data/shader/skybox.vs` — 天空盒顶点着色器
5. `data/shader/skybox.fs` — 天空盒片段着色器
6. `data/shader/particle3d.vs` — 粒子顶点着色器
7. `data/shader/particle3d.fs` — 粒子片段着色器
8. `examples/28_scene_level/main.cpp` — 演示程序
9. `examples/28_scene_level/README.md` — 详细文档
10. 更新 CMakeLists.txt
11. 更新 README.md

## 场景设计

### 场景1：训练场
- 天空盒（蓝天白云或程序化渐变天空）
- 大地面(30x30木地板) + 砖墙 + 木箱 + 金属柱
- 2个巡逻敌人
- 攻击命中时生成火花粒子
- 冲刺时生成蓝色拖尾粒子
- 场景中有触发器（走到某个区域显示文字提示）

### 场景2：竞技场
- 不同的天空（黄昏色调）
- 圆形场地 + 柱子
- 3个敌人
- 从场景1通过触发器传送过来

## 按键

和之前一致：WASD移动、Space跳跃、J攻击、F冲刺、E编辑面板、Tab切换摄像机模式

## ImGui编辑面板

- Camera: 摄像机参数
- Scene: 场景切换、触发器列表
- Particles: 粒子参数（发射速率/生命/大小/颜色/重力）
- Skybox: 天空颜色调整
- Objects: 场景物体
- Lighting: 光照参数+日光/黄昏/夜晚预设

## 音效（从本地路径或项目data/audio/中获取）

- 攻击命中：data/audio/sfx3d/hit.mp3
- 冲刺：data/audio/sfx3d/dash.mp3
- 场景切换：可以用 data/audio/sfx3d/unsheathe.mp3 临时替代
- BGM：data/audio/bgm/ 下的音乐文件（用miniaudio播放，FMOD留到29章）

## README要求

包含：天空盒渲染原理（为什么去掉平移/为什么z=w）、Billboard数学（粒子面向摄像机）、粒子系统设计、场景管理架构、触发器检测原理、5道思考题、下一章预告（第29章UI与音频+FMOD）
