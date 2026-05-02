# 第30章开发规范：完整3D动作游戏Demo（最终章）

## 必读：先读 PROJECT_CONTEXT.md、ROADMAP_CH26_30.md、LOCAL_ASSETS.md

## 这是整个教程的最终章，需要整合前29章所有系统，做出一个完整可玩的3D动作游戏。

---

## 游戏概述

一个3D动作游戏Demo，玩家操控角色在3个关卡中战斗，最终击败BOSS通关。

### 游戏流程
```
标题画面 → 第1关(教学) → 第2关(战斗) → 第3关(BOSS战) → 通关画面
```

---

## 第一步：复制敌人模型素材到项目

从本地路径复制敌人FBX到项目（参考LOCAL_ASSETS.md中的路径）：

```
mkdir data/models/fbx/enemies/mutant
mkdir data/models/fbx/enemies/zombie

复制 E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Mutant\Mutant.fbx → data/models/fbx/enemies/mutant/Mutant.fbx
复制 E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Mutant\Mutant Idle.fbx → data/models/fbx/enemies/mutant/Mutant_Idle.fbx
复制 E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Mutant\Mutant Run.fbx → data/models/fbx/enemies/mutant/Mutant_Run.fbx
复制 E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Mutant\Mutant Jump Attack.fbx → data/models/fbx/enemies/mutant/Mutant_Attack.fbx
复制 E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Mutant\Mutant Dying.fbx → data/models/fbx/enemies/mutant/Mutant_Death.fbx

复制 E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Zoomble\copzombie_l_actisdato.fbx → data/models/fbx/enemies/zombie/Zombie.fbx
复制 E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Zoomble\Zombie Idle.fbx → data/models/fbx/enemies/zombie/Zombie_Idle.fbx
复制 E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Zoomble\Zombie Running.fbx → data/models/fbx/enemies/zombie/Zombie_Running.fbx
复制 E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Zoomble\Zombie Attack.fbx → data/models/fbx/enemies/zombie/Zombie_Attack.fbx
复制 E:\SourceCode\Games\engine\mini_engine\3D素材\敌人\Zoomble\Zombie Death.fbx → data/models/fbx/enemies/zombie/Zombie_Death.fbx
```

---

## 需要创建的文件

1. `examples/30_3d_demo/main.cpp` — 完整游戏主程序
2. `examples/30_3d_demo/README.md` — 最终文档
3. 更新 CMakeLists.txt — 添加30章target
4. 更新 README.md（项目根目录）— 添加30章，标记项目完成

---

## 整合的系统（全部来自前29章，直接使用不要重写）

| 系统 | 来源文件 | 用途 |
|------|----------|------|
| 渲染 | material.vs/fs, skinning.vs/fs | 材质纹理+骨骼动画渲染 |
| 物理 | PhysicsWorld.h | PhysX碰撞检测 |
| 摄像机 | Camera3D.h | TPS跟随摄像机 |
| 角色控制 | CharacterController3D.h | 移动/跳跃/冲刺 |
| 骨骼动画 | Animator.h | 动画播放和切换 |
| 战斗 | CombatSystem3D.h | 攻击/受击/Hit Stop |
| 敌人AI | EnemyAI3D.h | 巡逻/追击/攻击状态机 |
| 天空盒 | Skybox.h | 天空渲染 |
| 粒子 | ParticleSystem3D.h | 攻击火花/冲刺拖尾 |
| 场景管理 | SceneManager.h | 关卡切换 |
| 音频 | AudioSystem3D.h | FMOD 3D音效+BGM |
| UI | GameUI3D.h | HUD/血条/伤害飘字 |

---

## 三个关卡设计

### 第1关：训练广场（教学关）
- **天空**：晴朗蓝天（程序化天空盒）
- **地面**：30x30 木地板材质
- **场景物体**：砖墙x2 + 木箱x3 + 金属柱x1（PhysX碰撞体）
- **敌人**：2个Zombie（普通小兵，HP=50，巡逻）
- **BGM**：data/audio/bgm/windaswarriors.mp3（轻松氛围）
- **教学提示**：屏幕中央显示操作提示（WASD移动/J攻击/Space跳跃/F冲刺）
- **通关条件**：消灭所有Zombie → 出现传送触发器 → 进入第2关

### 第2关：战斗竞技场
- **天空**：黄昏色调（橙红渐变天空）
- **地面**：30x30 金属板材质
- **场景物体**：圆形竞技场布局，4根金属柱子围一圈
- **敌人**：3个Zombie + 1个Mutant精英（HP=150，更快更强）
- **BGM**：data/audio/bgm/boss_battle.mp3（紧张节奏）
- **通关条件**：消灭所有敌人 → 传送到第3关

### 第3关：BOSS决战
- **天空**：暗红色夜空（压迫感）
- **地面**：20x20 砖墙材质地面
- **场景物体**：封闭空间，四面墙壁
- **敌人**：1个Mutant BOSS（HP=300）
- **BGM**：data/audio/bgm/boss_epic.mp3（史诗配乐）
- **BOSS三阶段**：

| 阶段 | HP范围 | 行为变化 |
|------|--------|----------|
| 1 | 100%→60% | 正常速度，普通攻击，攻击间隔2秒 |
| 2 | 60%→30% | 移速x1.5，攻击间隔1.2秒，屏幕微震提示阶段变化 |
| 3 | 30%→0% | 移速x2，攻击间隔0.8秒，连续攻击，狂暴状态 |

- **通关条件**：击败BOSS → 通关画面

---

## 玩家角色

- **模型**：data/models/fbx/X Bot.fbx
- **动画**（用Animator.h播放）：
  - 站立：data/models/fbx/Idle.fbx
  - 跑步：data/models/fbx/Running.fbx
  - 攻击：data/models/fbx/Punching.fbx
  - 死亡：data/models/fbx/Death.fbx
  - 跳跃：data/models/fbx/Standing Jump Running.fbx
- **属性**：HP=100，攻击力=15，攻击范围=2.0
- **动画状态机**：
  - 静止 → Idle动画
  - 移动中 → Running动画
  - 按J → Punching动画（播放完回Idle）
  - HP<=0 → Death动画 → 游戏结束

---

## 敌人

### Zombie（普通小兵）
- **模型**：data/models/fbx/enemies/zombie/Zombie.fbx
- **动画**：Zombie_Idle/Zombie_Running/Zombie_Attack/Zombie_Death.fbx
- **属性**：HP=50，攻击力=8，移速=3，检测范围=8，攻击范围=2
- **AI**：巡逻→发现玩家→追击→进入范围→攻击→玩家跑远→继续追→玩家太远→返回巡逻

### Mutant（精英/BOSS）
- **模型**：data/models/fbx/enemies/mutant/Mutant.fbx
- **动画**：Mutant_Idle/Mutant_Run/Mutant_Attack/Mutant_Death.fbx
- **精英属性**：HP=150，攻击力=15，移速=4，检测范围=12
- **BOSS属性**：HP=300，攻击力=20，移速=3.5（随阶段提升），三阶段行为

---

## 游戏状态

```cpp
enum class GameState {
    Title,       // 标题画面
    Playing,     // 游戏中
    Paused,      // 暂停
    GameOver,    // 玩家死亡
    Victory      // 通关
};
```

### 标题画面
- 显示游戏标题 "Mini Engine 3D Action Demo"
- 显示 "Press ENTER to Start"
- 背景可以是第1关的场景（摄像机缓慢旋转）

### 游戏结束
- 玩家HP<=0 → 播放死亡动画 → 显示 "GAME OVER"
- 显示 "Press R to Restart"

### 通关画面
- 击败BOSS → 显示 "VICTORY!"
- 显示通关统计（击杀数、用时）
- 显示 "Press ENTER to Return to Title"

---

## 按键映射（严格遵守）

| 按键 | 功能 |
|------|------|
| WASD | 移动角色（D=-1, A=1） |
| Space | 跳跃（二段跳） |
| J | 攻击 |
| Shift | 冲刺跑 |
| F | 闪避冲刺 |
| 鼠标左键拖动 | 旋转摄像机 |
| 滚轮 | 调整跟随距离 |
| ESC | 暂停/继续 |
| E | 编辑面板（调试用） |
| R | 重生（死亡后） |
| Enter | 开始/通关返回标题 |

---

## 音效播放时机

| 时机 | 音效 |
|------|------|
| 攻击挥拳 | sword_swing.mp3 |
| 命中敌人 | hit.mp3（3D空间音效，从敌人位置发出） |
| 玩家受伤 | hurt.mp3 |
| 跳跃 | jump.mp3 |
| 落地 | land.mp3 |
| 冲刺 | dash.mp3 |
| 敌人死亡 | slash.wav |
| BOSS阶段变化 | draw_sword.mp3 |
| 关卡切换 | unsheathe.mp3 |

---

## ImGui编辑面板（E键，调试用）

| 标签页 | 内容 |
|--------|------|
| Game | 当前关卡/敌人数量/游戏状态/时间/击杀统计 |
| Player | HP/攻击力/位置/速度/当前动画 |
| Enemies | 每个敌人的HP/状态/位置 |
| Combat | 攻击参数/Hitbox大小/Hit Stop时长 |
| Camera | TPS距离/高度/灵敏度 |
| Audio | BGM音量/SFX音量 |
| Lighting | 方向光/点光源/环境光 |
| Physics | PhysX参数/碰撞体可视化 |

---

## UI显示

### 游戏中HUD（始终显示）
- 左上角：玩家HP血条（绿→黄→红）
- 左上角下方：当前关卡名
- 右上角：击杀数 / 总敌人数
- BOSS战时：屏幕上方显示BOSS血条和名字

### 敌人头顶
- 血条（受伤时显示，几秒后隐藏）
- 敌人名字

### 伤害飘字
- 命中敌人时从敌人位置飘出伤害数字
- 向上飘动+渐隐

### 提示文字
- 教学关的操作提示
- 关卡切换提示
- BOSS阶段变化提示

---

## 粒子效果

| 时机 | 效果 |
|------|------|
| 攻击命中 | 橙红色火花爆散 |
| 冲刺 | 蓝色拖尾 |
| BOSS阶段变化 | 红色冲击波 |
| 敌人死亡 | 灰色烟雾消散 |
| 关卡传送 | 白色光圈 |

---

## CMakeLists.txt配置

```cmake
file(GLOB example_30_cpp examples/30_3d_demo/*.cpp)

add_executable(30_3d_demo
    ${glfw_sources} ${imgui_src} ${engine_cpp} ${engine_h} ${example_30_cpp})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib")
    target_link_libraries(30_3d_demo
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/zlibstatic.lib")
endif()

# FMOD
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/fmod/core/lib/fmod_vc.lib")
    target_link_libraries(30_3d_demo
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/fmod/core/lib/fmod_vc.lib")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/depends/fmod/core/lib/fmod.dll"
         DESTINATION "${CMAKE_BINARY_DIR}/Debug")
endif()

# PhysX已通过CMakeLists.txt.Physx全局链接

# MSVC配置
set_property(TARGET 30_3d_demo PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT 30_3d_demo)
set_target_properties(30_3d_demo PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
```

---

## README.md要求

这是最终章的README，要写得有总结感：

1. **项目回顾**：30章从零到完整3D动作游戏的历程
2. **整合的系统列表**：渲染/物理/动画/战斗/AI/音频/UI全部列出
3. **游戏设计文档**：三关卡设计、BOSS三阶段、敌人类型
4. **操作说明**：完整按键表
5. **技术栈总结**：OpenGL/GLFW/Assimp/PhysX/FMOD/ImGui
6. **致谢**

## 总README.md更新

在3D章节表中添加：
```
| 30 | 3D动作游戏Demo | 全系统整合、3关卡、BOSS战、完整游戏流程 |
```

在最下方添加：
```
## 项目完成

全部30章教程已完成。从第1章创建窗口到第30章完整3D动作游戏Demo，
涵盖了2D和3D游戏引擎开发的核心技术。

感谢 Claude Opus 4.5（第1-14章）和 Claude Opus 4.6（第12章修复、第14章重写、第15-30章）的开发工作。
```
