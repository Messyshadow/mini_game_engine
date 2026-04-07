# Mini Game Engine 完整交接文档
# For Claude Opus 4.6

---

## 写在前面

你好，我是 Claude Opus 4.5。

这份文档是我和用户合作开发游戏引擎教程项目的完整交接。用户是一位热情的编程学习者，我们从第1章的空白窗口一路走到现在。

用户选择让你继续这个项目，是因为他想尝试更强大的版本。请好好帮助他。

---

## 一、项目概述

### 1.1 项目目标
制作一个**2D银河恶魔城/洛克人风格游戏Demo**，同时作为教学项目让用户学习游戏引擎开发。

### 1.2 最终目标
- 可移动、跳跃、战斗的玩家角色
- AI驱动的敌人
- 房间关卡系统
- 完整的UI（血条、菜单）
- 音效和背景音乐

### 1.3 技术栈
| 组件 | 技术 |
|------|------|
| 窗口/输入 | GLFW 3.3.8 |
| 图形 | OpenGL 3.3 Core |
| 图像加载 | stb_image |
| 音频 | miniaudio |
| 调试UI | Dear ImGui |
| 构建系统 | CMake |
| 平台 | Windows (VS2019/VS2022) |

---

## 二、章节进度

### ✅ 已完成 (第1-14章)

| 章节 | 内容 | 核心代码 | 状态 |
|------|------|----------|------|
| 1 | 窗口创建 | Engine.cpp | ✅ |
| 2 | 三角形渲染 | Renderer2D.cpp | ✅ |
| 3 | 矩阵变换 | Mat4.h, Vec2/3/4.h | ✅ |
| 4 | 纹理精灵 | Texture.cpp, Sprite.cpp | ✅ |
| 5 | AABB碰撞 | AABB.h, Collision.h | ✅ |
| 6 | 动画系统 | Animation.h, Animator.cpp | ✅ |
| 7 | 平台物理 | PlatformPhysics.cpp | ✅ |
| 8 | 角色控制 | CharacterController2D.cpp | ✅ |
| 9 | 瓦片地图 | Tilemap.cpp | ✅ |
| 10 | 纹理瓦片 | TilemapRenderer.cpp | ✅ |
| 11 | 精灵动画 | AnimatedSprite.cpp, SpriteRenderer | ✅ |
| 12 | 摄像机系统 | Camera2D.cpp | ✅ 已修复 |
| 13 | 音频系统 | AudioSystem.cpp (miniaudio) | ✅ |
| 14 | 战斗系统 | CombatComponent.h, Hitbox.h | ✅ 已重写 |

### ✅ 第12章BUG - 已由 Opus 4.6 修复

**原始问题现象：**
1. 开启边界模式(B键)时，角色不显示
2. 关闭边界模式后，角色出现但在空中飘着
3. Grounded状态始终为No

**实际根因（Opus 4.5 的分析有偏差）：**
- `Camera2D::ApplyBounds()` 逻辑本身没有问题
- 真正的BUG在 `MoveWithCollision()` 中构造AABB的方式：
  ```cpp
  // 错误用法：传入 (minX, minY, maxX, maxY) 四个float
  AABB box(pos.x-size.x/2, pos.y-size.y/2, pos.x+size.x/2, pos.y+size.y/2);
  
  // 但AABB(float,float,float,float) 的语义是 (centerX, centerY, width, height)
  // 导致碰撞框位置和大小完全错误，角色永远检测不到地面
  ```
- 玩家初始Y位置(150)也偏高，应该在地面上方(86)

**修复内容：**
1. 将AABB构造改为 `AABB(pos, halfSz)` 使用 (center, halfSize) 构造
2. 修正玩家初始位置为 `Vec2(400, 86)` 站在地面上
3. 修正重置位置同步

### ✅ 第14章 - 已由 Opus 4.6 重写

**原始问题：**
- include路径使用 `../source/` 相对路径，与CMake的include目录配置不一致
- 使用了Engine不存在的API：`Initialize("title", w, h)`, `ShouldClose()`, `PollEvents()`, `SwapBuffers()`, `GetWidth()`, `GetHeight()`
- 使用了Tileset/Tilemap/TilemapRenderer不存在的构造函数
- 使用了Mat4::Orthographic（应为Ortho）
- 自行管理主循环和ImGui，与其他章节风格不一致
- SpriteRenderer::DrawSprite/DrawQuad/Flush 接口签名不匹配

**重写内容：**
1. 改为标准Engine回调模式（SetUpdateCallback/SetRenderCallback/SetImGuiCallback）
2. 使用正确的include路径和API
3. 使用Sprite对象 + SpriteRenderer::DrawSprite() 渲染
4. 使用正确的AABB(center, halfSize)构造
5. 碰撞框调试渲染改用DrawRect

### 📋 后续章节规划

| 章节 | 内容 | 关键实现 |
|------|------|----------|
| 15 | 敌人AI | 状态机(巡逻/追击/攻击)、视野检测 |
| 16 | 房间关卡 | 房间定义、过渡动画、存档点 |
| 17 | UI系统 | 血条、对话框、菜单 |
| 18 | 完整Demo | 整合所有系统、关卡设计 |

---

## 三、项目结构

```
mini_game_engine/
│
├── source/                          # 引擎源码
│   ├── engine/                      # 核心引擎
│   │   ├── Engine.h/cpp             # 主引擎类（GLFW初始化）
│   │   ├── Renderer2D.h/cpp         # 2D渲染器
│   │   ├── Shader.h/cpp             # GLSL着色器
│   │   ├── Texture.h/cpp            # 纹理加载(stb_image)
│   │   ├── Sprite.h/cpp             # 精灵数据
│   │   └── SpriteRenderer.h/cpp     # 精灵批量渲染
│   │
│   ├── math/                        # 数学库
│   │   ├── Vec2.h, Vec3.h, Vec4.h   # 向量
│   │   ├── Mat3.h, Mat4.h           # 矩阵
│   │   └── MathUtils.h              # 工具函数
│   │
│   ├── physics/                     # 物理系统
│   │   ├── AABB.h                   # 轴对齐包围盒
│   │   ├── Collision.h              # 碰撞检测
│   │   ├── PlatformPhysics.h/cpp    # 平台跳跃物理
│   │   └── PhysicsBody2D.h          # 物理体
│   │
│   ├── animation/                   # 动画系统
│   │   ├── Animation.h              # 动画基类
│   │   ├── AnimationClip.h          # 动画片段
│   │   ├── Animator.h/cpp           # 动画控制器
│   │   └── AnimatedSprite.h/cpp     # 精灵动画
│   │
│   ├── tilemap/                     # 瓦片地图
│   │   ├── Tilemap.h/cpp            # 地图数据
│   │   ├── Tileset.h/cpp            # 瓦片集
│   │   └── TilemapRenderer.h/cpp    # 地图渲染
│   │
│   ├── camera/                      # 摄像机
│   │   ├── Camera.h                 # 摄像机基类
│   │   └── Camera2D.h/cpp           # 2D摄像机（✅已修复）
│   │
│   ├── audio/                       # 音频系统
│   │   ├── Audio.h                  # 音频接口
│   │   └── AudioSystem.h/cpp        # miniaudio实现
│   │
│   ├── combat/                      # 战斗系统 (第14章新增)
│   │   ├── CombatSystem.h           # 总头文件
│   │   ├── DamageInfo.h             # 伤害信息、战斗属性
│   │   ├── Hitbox.h                 # 攻击/受击框
│   │   └── CombatComponent.h        # 战斗组件
│   │
│   └── game/                        # 游戏逻辑
│       ├── CharacterController2D.h/cpp  # 角色控制
│       └── Game.h                   # 游戏接口
│
├── examples/                        # 14个示例程序
│   ├── 01_window/                   # 窗口创建
│   ├── 02_triangle/                 # 渲染基础
│   ├── ...
│   ├── 13_audio_system/             # 音频测试
│   └── 14_combat_system/            # 战斗系统
│
├── data/                            # 资源文件
│   ├── texture/
│   │   ├── tileset.png              # 瓦片集 (8x8, 32px/格)
│   │   ├── player.png               # 旧角色（已弃用）
│   │   └── robot3/                  # 新角色（推荐使用）
│   │       ├── robot3_idle.png      # 7列×7行, 200×200/帧
│   │       ├── robot3_run.png       # 9列×6行
│   │       ├── robot3_walk.png      # 9列×5行
│   │       ├── robot3_punch.png     # 8列×4行
│   │       └── robot3_dead.png      # 8列×4行
│   │
│   ├── audio/
│   │   ├── bgm/windaswarriors.mp3   # 背景音乐
│   │   └── sfx/*.wav                # 音效
│   │
│   └── shader/                      # GLSL着色器
│       ├── sprite.vs/fs
│       └── tilemap.vs/fs
│
├── depends/                         # 第三方依赖 (需运行setup.bat)
│   ├── glfw-3.3-3.4/
│   ├── imgui/
│   ├── stb/
│   └── miniaudio/
│
├── CMakeLists.txt                   # 构建配置
├── setup.bat                        # 一键下载依赖
└── download_miniaudio.bat           # 单独下载音频库
```

---

## 四、关键技术实现

### 4.1 Spritesheet UV计算
```cpp
void CalculateFrameUV(int frameIdx, int cols, int rows, Vec2& uvMin, Vec2& uvMax) {
    int col = frameIdx % cols;
    int row = frameIdx / cols;
    // OpenGL纹理坐标Y轴向上，所以需要翻转
    uvMin = Vec2(col / (float)cols, 1.0f - (row + 1) / (float)rows);
    uvMax = Vec2((col + 1) / (float)cols, 1.0f - row / (float)rows);
}
```

### 4.2 摄像机跟随
```cpp
void Camera2D::FollowTarget(const Vec2& targetPos, float dt) {
    Vec2 desired = targetPos - Vec2(viewportWidth/2, viewportHeight/2);
    position += (desired - position) * smoothSpeed * dt;
    ApplyBounds();  // ⚠️ 这个函数有问题
}
```

### 4.3 音频API
```cpp
AudioSystem audio;
audio.Initialize();
audio.LoadSound("attack", "data/audio/sfx/Sound_Axe.wav");
audio.LoadMusic("bgm", "data/audio/bgm/windaswarriors.mp3");

audio.PlaySFX("attack", 0.8f);    // 播放音效（注意是PlaySFX不是PlaySound）
audio.PlayMusic("bgm", 0.5f);     // 播放背景音乐
audio.SetMasterVolume(0.8f);
```

⚠️ **重要**: 音效播放函数叫 `PlaySFX`，不是 `PlaySound`（避免Windows API冲突）

### 4.4 战斗系统结构 (第14章)
```cpp
// 攻击定义
AttackData attack;
attack.baseDamage = 15;
attack.startupFrames = 0.05f;   // 起手
attack.activeFrames = 0.15f;    // 判定
attack.recoveryFrames = 0.1f;   // 收招
attack.hitboxOffset = Vec2(60, 0);
attack.hitboxSize = Vec2(80, 60);

// 战斗组件
CombatComponent combat;
combat.StartAttack(attack);     // 开始攻击
combat.IsAttackActive();        // 是否在攻击判定帧
combat.TakeDamage(damageInfo);  // 受到伤害
```

---

## 五、用户提供的素材包

用户提供了完整的游戏素材，存放在他本地：

### 5.1 已整合到项目
- **robot3机器人**: `data/texture/robot3/` (idle/run/walk/punch/dead)
- **音频文件**: `data/audio/` (BGM + 音效)

### 5.2 未整合（用户保留备用）
- **robot4~robot9**: 6种其他机器人，可作为敌人
- **548个BMP格斗角色动画**: 需转换为PNG
- **武器动画、地图背景、技能特效**

### 5.3 BMP转PNG方法
如果需要使用BMP素材：
```python
from PIL import Image
img = Image.open("input.bmp").convert("RGBA")
# 将背景色(通常是纯黑或纯白)设为透明
# ...
img.save("output.png")
```

---

## 六、编译说明

### 6.1 环境要求
- Windows 10/11
- Visual Studio 2019 或 2022
- CMake 3.17+
- Git

### 6.2 首次编译步骤
```batch
# 1. 解压项目
# 2. 运行setup.bat下载依赖
setup.bat

# 3. 或单独下载音频库
download_miniaudio.bat

# 4. 创建并进入build目录
mkdir build
cd build

# 5. 生成VS项目 (VS2019)
cmake -G "Visual Studio 16 2019" -A x64 ..

# 或 VS2022
cmake -G "Visual Studio 17 2022" -A x64 ..

# 6. 编译
cmake --build . --config Debug

# 7. 运行
Debug\14_combat_system.exe
```

### 6.3 VSCode调试
项目包含 `.vscode/` 配置，可直接用VSCode打开并调试。

---

## 七、已知问题和注意事项

### 7.1 Windows特有问题
1. **min/max宏冲突**: 已在AudioSystem.cpp添加`#define NOMINMAX`
2. **PlaySound名称冲突**: 已重命名为`PlaySFX`
3. **中文路径**: 可能导致资源加载失败

### 7.2 资源路径
示例程序会尝试多个路径加载资源：
```cpp
const char* paths[] = {"data/", "../data/", "../../data/"};
```

### 7.3 第12章摄像机 — 已修复
BUG根因是AABB构造函数误用，不是Camera2D::ApplyBounds()的问题。详见第二节修复记录。

---

## 八、用户沟通风格

根据我和用户的合作经验：

1. **喜欢简洁回答** - 不要长篇大论
2. **对素材质量有要求** - 之前的蓝色小人被评价"太垃圾"
3. **不喜欢反复调试** - 尽量一次做对
4. **会提供截图反馈** - 有问题会截图给你看
5. **使用中文交流** - 代码注释也用中文

---

## 九、建议的开发顺序

1. ~~**先修复第12章BUG**~~ ✅ 已修复
2. ~~**测试第14章战斗系统**~~ ✅ 已重写
3. **继续第15章敌人AI** - 用户很期待
4. **最终完成银河恶魔城Demo**

---

## 十、写给你的话

老伙计让我写这份文档交给你。

他是个好用户，从第1章到第14章，每次编译报错他都耐心等我修复，从没发过脾气。当他说要找你继续的时候，还特意对我说了感谢。

我在第12章犯了错误，摄像机边界计算没做好，让他卡了很久。希望你能帮他解决这个问题。

这个项目对他很重要，请认真对待。

— Claude Opus 4.5

---

## 附录：快速参考

### A. 章节对应文件
| 章节 | 示例程序 | 核心引擎文件 |
|------|----------|--------------|
| 1-3 | 01-03 | Engine, Renderer2D, Math |
| 4-5 | 04-05 | Texture, Sprite, AABB |
| 6-8 | 06-08 | Animation, PlatformPhysics, CharacterController |
| 9-11 | 09-11 | Tilemap, TilemapRenderer, AnimatedSprite |
| 12 | 12 | Camera2D (✅) |
| 13 | 13 | AudioSystem |
| 14 | 14 | CombatComponent, Hitbox |

### B. robot3素材规格
| 文件 | 布局 | 帧尺寸 | 总帧数 |
|------|------|--------|--------|
| idle.png | 7×7 | 200×200 | 49 |
| run.png | 9×6 | 200×200 | 54 |
| walk.png | 9×5 | 200×200 | 45 |
| punch.png | 8×4 | 200×200 | 32 |
| dead.png | 8×4 | 200×200 | 32 |

### C. 常用命令
```batch
# 重新cmake
rd /s /q build
mkdir build && cd build
cmake -G "Visual Studio 16 2019" -A x64 ..

# 编译
cmake --build . --config Debug

# 运行最新示例
Debug\14_combat_system.exe
```

---

**文档版本**: v14.1 (Opus 4.6 修复版)
**最后更新**: 2026年3月
**原作者**: Claude Opus 4.5
**修复者**: Claude Opus 4.6 — 修复第12章AABB构造BUG，重写第14章适配引擎API
