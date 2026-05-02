# 第30章：完整3D动作游戏Demo（最终章）

这是Mini Game Engine教程的最终章。整合了前29章所有系统，实现一个完整可玩的3D动作游戏Demo。

## 项目回顾

从第1章创建窗口开始，经历30章的开发：
- **第1-4章**：基础渲染管线（窗口、三角形、矩阵、纹理）
- **第5-8章**：核心系统（碰撞、动画、物理、角色控制）
- **第9-13章**：世界构建（瓦片地图、摄像机、音频）
- **第14-18章**：2D游戏玩法（战斗、AI、关卡、UI、完整2D Demo）
- **第19-25章**：3D基础（渲染、模型、光照、材质、摄像机、角色控制、骨骼动画）
- **第26-29章**：3D游戏系统（战斗、AI、场景、音频UI）
- **第30章**：全部整合，完整3D动作游戏

## 整合的系统

| 系统 | 来源 | 用途 |
|------|------|------|
| 渲染 | material.vs/fs, skinning.vs/fs | 材质纹理渲染 + 骨骼动画渲染 |
| 天空盒 | Skybox.h | 程序化渐变天空（每关不同色调） |
| 物理 | AABB碰撞 | 场景物体碰撞解算 |
| 摄像机 | Camera3D.h | TPS第三人称跟随 |
| 角色控制 | CharacterController3D.h | 移动/跳跃/冲刺 |
| 骨骼动画 | Animator.h | 角色动画播放+混合切换 |
| 战斗 | CombatSystem3D.h | 三段连招、Hit Stop、击退 |
| 敌人AI | EnemyAI3D.h | 巡逻/追击/攻击状态机 |
| 粒子 | ParticleSystem3D.h | 攻击火花/冲刺拖尾/死亡烟雾/环境粒子 |
| 场景管理 | SceneManager.h | 3关卡配置+淡入淡出过渡 |
| 音频 | AudioSystem3D.h (FMOD) | 3D空间音效 + BGM |
| UI | GameUI3D.h | HUD血条/敌人血条/BOSS血条/伤害飘字/提示 |

## 游戏设计

### 三个关卡

| 关卡 | 名称 | 天空 | 敌人 | BGM |
|------|------|------|------|-----|
| 1 | 训练广场 | 晴朗蓝天 | 2x Zombie | windaswarriors.mp3 |
| 2 | 战斗竞技场 | 黄昏色调 | 3x Zombie + 1x Mutant精英 | boss_battle.mp3 |
| 3 | BOSS决战 | 暗红夜空 | 1x Mutant BOSS (HP=300) | boss_epic.mp3 |

### BOSS三阶段

| 阶段 | HP范围 | 行为 |
|------|--------|------|
| 1 | 100%→60% | 正常速度，攻击间隔2秒 |
| 2 | 60%→30% | 移速x1.5，攻击加速 |
| 3 | 30%→0% | 移速x2，狂暴连续攻击 |

### 敌人类型

- **Zombie**：HP=50，攻击力=8，普通巡逻小兵
- **Mutant精英**：HP=150，攻击力=15，更快更强
- **Mutant BOSS**：HP=300，攻击力=20，三阶段行为变化

## 操作说明

| 按键 | 功能 |
|------|------|
| WASD | 移动角色 |
| Space | 跳跃 |
| J | 攻击（三段连招） |
| Shift | 冲刺跑 |
| F | 闪避冲刺 |
| 鼠标左键拖动 | 旋转摄像机 |
| 滚轮 | 调整跟随距离 |
| ESC | 暂停/继续 |
| E | 编辑面板（调试） |
| R | 重生（死亡后） |
| Enter | 开始/通关返回标题 |

## 游戏流程

```
标题画面 (ENTER) → 第1关(教学) → 击败所有敌人 → 传送门 →
第2关(竞技场) → 击败所有敌人 → 传送门 →
第3关(BOSS战) → 击败BOSS → 通关画面 (ENTER) → 返回标题
```

## 技术栈总结

| 技术 | 版本/说明 | 用途 |
|------|-----------|------|
| OpenGL | 3.3 Core Profile | 3D渲染 |
| GLFW | 3.3.8 | 窗口管理和输入 |
| Assimp | 5.x | FBX模型和动画加载 |
| FMOD | Core API | 3D空间音效 |
| Dear ImGui | 1.89+ | 调试UI/HUD渲染 |
| stb_image | — | 纹理图像加载 |
| CMake | 3.17+ | 跨IDE构建 |

## 构建运行

```batch
cd build
cmake --build . --config Debug
Debug\30_3d_demo.exe
```

## 编辑面板（E键）

8个标签页：Game / Player / Enemies / Camera / Audio / Lighting / Combat

可实时调节：
- 玩家HP、攻击力、位置
- 敌人状态、HP、复活
- 摄像机距离、FOV、高度
- 音量（Master/BGM/SFX）
- 光照方向、颜色、环境光
- 战斗参数（伤害/击退/Hit Stop/无敌时间）

## 致谢

感谢以下开源项目和资源：
- GLFW、OpenGL、Dear ImGui、Assimp、FMOD、stb_image
- Mixamo（角色模型和动画）
- 爱给网（音效素材）

本教程从零到完整3D动作游戏，全部30章完成。
