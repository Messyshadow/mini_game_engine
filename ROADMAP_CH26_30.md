# 后续章节开发规范（第26-30章）

## 必读：先读 PROJECT_CONTEXT.md 了解项目全部API和代码规范

## 资源清单

### 已在项目中的资源

| 目录 | 内容 | 用于 |
|------|------|------|
| data/models/fbx/X Bot.fbx | Mixamo角色模型（带骨骼） | 全部3D章节 |
| data/models/fbx/Idle.fbx | 待机动画 | 动画状态机 |
| data/models/fbx/Running.fbx | 跑步动画 | 移动时播放 |
| data/models/fbx/Punching.fbx | 拳击动画 | 攻击动作 |
| data/models/fbx/Death.fbx | 死亡动画 | 敌人死亡 |
| data/models/fbx/Punch Combo.fbx | 连击动画 | 连招系统 |
| data/models/fbx/Great Sword Slash.fbx | 剑斩动画 | 武器攻击 |
| data/models/fbx/Standing Jump Running.fbx | 跳跃动画 | 跳跃时播放 |
| data/texture/materials/bricks/ | 砖墙材质(diffuse/normal/roughness) | 场景 |
| data/texture/materials/wood/ | 木板材质 | 场景 |
| data/texture/materials/metal/ | 金属材质 | 场景 |
| data/texture/materials/metalplates/ | 金属板材质 | 场景 |
| data/texture/materials/woodfloor/ | 木地板材质 | 地面 |
| data/audio/sfx3d/hit.mp3 | 命中音效 | 战斗 |
| data/audio/sfx3d/hurt.mp3 | 受伤音效 | 战斗 |
| data/audio/sfx3d/sword_swing.mp3 | 挥剑音效 | 攻击 |
| data/audio/sfx3d/slash.wav | 劈砍音效 | 攻击 |
| data/audio/sfx3d/draw_sword.mp3 | 拔刀音效 | 战斗开始 |
| data/audio/sfx3d/unsheathe.mp3 | 出鞘音效 | 战斗 |
| data/audio/sfx3d/dash.mp3 | 冲刺音效 | 闪避 |
| data/audio/sfx3d/jump.mp3 | 跳跃音效 | 跳跃 |
| data/audio/sfx3d/land.mp3 | 落地音效 | 着陆 |
| data/audio/sfx3d/metal_draw.mp3 | 金属拔剑 | 战斗 |
| data/audio/bgm/ | 4首BGM | 背景音乐 |
| depends/assimp/ | Assimp 6.0 (lib+dll+headers) | 模型加载 |
| depends/fmod/core/ | FMOD Core (fmod.h+lib+dll x64) | 第29章音频 |
| depends/fmod/studio/ | FMOD Studio (lib+dll x64) | 第29章音频 |
| reference/physx_example/ | PhysX用法参考(main.cpp+CMakeLists) | 第27章物理 |

### PhysX需要下载（第27章时）

下载地址：https://github.com/ThisisGame/PhysX/releases/download/v6.4/physx-4.1.7z
解压后放到 depends/physx/
参考 reference/physx_example/CMakeLists.txt.Physx 配置

头文件：depends/physx/physx/include + depends/physx/pxshared/include
链接库：PhysX_64, PhysXFoundation_64, PhysXExtensions_static_64, PhysXPvdSDK_static_64
DLL：PhysXFoundation_64.dll, PhysXDevice64.dll, PhysXCooking_64.dll, PhysXCommon_64.dll, PhysX_64.dll

---

## 第26章：3D战斗系统

详见 CH26_SPEC.md

---

## 第27章：敌人AI + PhysX

### 需要创建的文件
1. source/engine3d/EnemyAI3D.h — 行为树+A*寻路
2. source/engine3d/PhysicsWorld.h — PhysX封装
3. examples/27_enemy_ai/main.cpp
4. examples/27_enemy_ai/README.md

### 核心内容
- 行为树（Selector/Sequence/Condition/Action节点）
- 敌人状态：Patrol/Chase/Attack/Retreat/Dead
- 视野检测（角度+距离）
- 简单A*寻路（网格地图）
- PhysX集成：替换自写物理，刚体碰撞
- PhysX射线检测替代简化地面检测

### PhysX集成CMake配置
```cmake
include_directories("depends/physx/physx/include")
include_directories("depends/physx/pxshared/include")
if(MSVC)
    target_link_libraries(27_enemy_ai
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysX_64.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXFoundation_64.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXExtensions_static_64.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXPvdSDK_static_64.lib")
    # 拷贝DLL
    file(COPY "depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXFoundation_64.dll" DESTINATION "${CMAKE_BINARY_DIR}/Debug")
    file(COPY "depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXDevice64.dll" DESTINATION "${CMAKE_BINARY_DIR}/Debug")
    file(COPY "depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXCooking_64.dll" DESTINATION "${CMAKE_BINARY_DIR}/Debug")
    file(COPY "depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXCommon_64.dll" DESTINATION "${CMAKE_BINARY_DIR}/Debug")
    file(COPY "depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysX_64.dll" DESTINATION "${CMAKE_BINARY_DIR}/Debug")
endif()
```

---

## 第28章：场景与关卡

### 需要创建的文件
1. source/engine3d/SceneManager.h — 场景管理器
2. source/engine3d/Skybox.h — 天空盒
3. source/engine3d/ParticleSystem3D.h — 3D粒子系统
4. data/shader/skybox.vs/fs — 天空盒shader
5. data/shader/particle3d.vs/fs — 粒子shader
6. examples/28_scene_level/main.cpp
7. examples/28_scene_level/README.md

### 核心内容
- 场景管理器（加载/切换场景）
- 天空盒（6面CubeMap纹理）
- 阴影映射（Shadow Map，方向光阴影）
- 3D粒子系统（Billboard面向摄像机）
- 触发器系统（进入区域触发事件）

### 天空盒shader要点
```glsl
// skybox.vs - 去掉平移，只保留旋转
gl_Position = (projection * mat4(mat3(view)) * vec4(aPos, 1.0)).xyww;
// skybox.fs - 立方体贴图采样
FragColor = texture(uSkybox, vTexCoord); // samplerCube
```

---

## 第29章：UI与音频 + FMOD

### 需要创建的文件
1. source/engine3d/AudioSystem3D.h — FMOD音频封装
2. source/engine3d/UI3D.h — 3D HUD系统
3. examples/29_ui_audio/main.cpp
4. examples/29_ui_audio/README.md

### FMOD集成
头文件：depends/fmod/core/inc/fmod.h
库文件：depends/fmod/core/lib/fmod_vc.lib
DLL：depends/fmod/core/lib/fmod.dll

```cmake
include_directories("depends/fmod/core/inc")
include_directories("depends/fmod/studio/inc")
target_link_libraries(29_ui_audio
    "${CMAKE_CURRENT_SOURCE_DIR}/depends/fmod/core/lib/fmod_vc.lib")
file(COPY "depends/fmod/core/lib/fmod.dll" DESTINATION "${CMAKE_BINARY_DIR}/Debug")
```

### FMOD基本用法
```cpp
#include "fmod.h"
FMOD_SYSTEM* fmodSystem;
FMOD_System_Create(&fmodSystem);
FMOD_System_Init(fmodSystem, 512, FMOD_INIT_NORMAL, 0);

// 播放音效
FMOD_SOUND* sound;
FMOD_System_CreateSound(fmodSystem, "data/audio/sfx3d/hit.mp3", FMOD_DEFAULT, 0, &sound);
FMOD_System_PlaySound(fmodSystem, sound, 0, false, 0);

// 3D音效
FMOD_System_CreateSound(fmodSystem, path, FMOD_3D, 0, &sound);
FMOD_Channel_Set3DAttributes(channel, &pos, &vel);
FMOD_System_Set3DListenerAttributes(fmodSystem, 0, &listenerPos, &vel, &forward, &up);
```

### 音效文件（已在项目中）
data/audio/sfx3d/ 下有10个音效文件（hit/hurt/slash/sword_swing/dash/jump/land等）

---

## 第30章：完整3D动作游戏Demo

### 需要创建的文件
1. examples/30_3d_demo/main.cpp
2. examples/30_3d_demo/README.md
3. data/levels/3d/ — 3D关卡数据

### 核心内容
- 整合所有系统（渲染/物理/动画/战斗/AI/音频/UI）
- 3个关卡（教学关→战斗关→BOSS关）
- BOSS战（多阶段，随HP变化行为改变）
- 完整游戏流程（标题→游戏→通关）

---

## 每章通用要求

1. 每章有ImGui编辑面板（E键切换），包含多个标签页
2. 每章有详细README.md（学习目标/数学原理/Shader解析/操作说明/思考题/下一章预告）
3. 每章更新CMakeLists.txt和项目根目录README.md
4. 严格遵守PROJECT_CONTEXT.md中的API规范
5. 不要使用Camera3D中不存在的成员
6. 不要限制角色移动范围
7. A/D输入映射：D=-1, A=1
