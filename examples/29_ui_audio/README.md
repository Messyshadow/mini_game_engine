# 第29章：UI与音频 + FMOD

## 本章核心

本章用FMOD替换miniaudio，实现3D空间音效，同时构建完整的游戏UI系统（HUD、敌人血条、伤害飘字）。

## 三大系统

### A. FMOD 3D音频系统

FMOD是专业级音频中间件，相比miniaudio提供更强大的3D空间音效支持。

**初始化流程：**
```
FMOD_System_Create → FMOD_System_Init → LoadSound → PlaySound
```

**FMOD C API 核心函数：**
- `FMOD_System_Create` / `FMOD_System_Init` — 创建并初始化系统
- `FMOD_System_CreateSound` — 加载音频文件（支持mp3/wav/ogg等）
- `FMOD_System_PlaySound` — 播放声音，返回Channel句柄
- `FMOD_Channel_Set3DAttributes` — 设置声源的3D位置
- `FMOD_System_Set3DListenerAttributes` — 设置听者位置（通常是摄像机）
- `FMOD_System_Update` — 每帧调用，更新3D计算

**2D vs 3D音效：**
| 特性 | 2D音效 | 3D音效 |
|------|--------|--------|
| 创建模式 | FMOD_DEFAULT | FMOD_3D |
| 音量 | 全局一致 | 随距离衰减 |
| 声道 | 立体声 | 左右声道随位置变化 |
| 适用场景 | UI、BGM、跳跃 | 攻击命中、敌人动作 |

### B. 3D空间音效

**3D听者（Listener）概念：**

3D音效系统需要知道"听者"在哪里、面朝什么方向。在游戏中，听者通常就是摄像机：

```
每帧更新：
  camPos = 摄像机世界坐标
  camFwd = 摄像机前方向量
  camUp  = (0, 1, 0)
  FMOD_System_Set3DListenerAttributes(system, 0, &camPos, &vel, &camFwd, &camUp)
```

**距离衰减：**
- 近处的声音响亮，远处的声音变小
- 左边的敌人攻击声从左声道传来
- `FMOD_System_Set3DSettings` 控制衰减曲线（rolloff scale）

**本章音效映射：**
| 事件 | 音效 | 类型 | 说明 |
|------|------|------|------|
| 按J攻击 | sword_swing | 2D | 挥剑动作音 |
| 命中敌人 | hit | 3D | 从敌人位置发出 |
| 空挥 | slash | 3D | 从hitbox位置发出 |
| 受伤 | hurt | 3D | 从攻击者位置发出 |
| 跳跃 | jump | 2D | 玩家自身动作 |
| 落地 | land | 2D | 玩家自身动作 |
| 冲刺 | dash | 2D | 玩家自身动作 |
| 拔剑/切换 | draw_sword | 2D | 场景切换时 |
| BGM | bgm_battle | 2D循环 | 战斗背景音乐 |

### C. 游戏UI系统

**世界坐标→屏幕坐标 数学推导：**

要在敌人头顶显示血条，需要将3D世界坐标投影到2D屏幕坐标：

```
1. 构建VP矩阵：VP = Projection × View

2. 裁剪坐标：clip = VP × [worldX, worldY, worldZ, 1]
   clipX = VP[0]*x + VP[4]*y + VP[8]*z  + VP[12]
   clipY = VP[1]*x + VP[5]*y + VP[9]*z  + VP[13]
   clipZ = VP[2]*x + VP[6]*y + VP[10]*z + VP[14]
   clipW = VP[3]*x + VP[7]*y + VP[11]*z + VP[15]

3. 透视除法 → NDC（范围[-1,1]）：
   ndcX = clipX / clipW
   ndcY = clipY / clipW

4. NDC → 屏幕坐标（注意Y轴翻转）：
   screenX = (ndcX × 0.5 + 0.5) × screenWidth
   screenY = (1.0 - (ndcY × 0.5 + 0.5)) × screenHeight

5. 剔除：如果 clipW < 0.01，物体在摄像机后面，不渲染
```

**伤害飘字设计：**
- 命中时在敌人头顶生成数字
- 数字每帧向上移动（worldPos.y += speed × dt）
- 透明度随生命周期衰减（alpha = min(1, life / fadeThreshold)）
- 暴击（第三段连招）用更大字号和金黄色

**HP血条颜色渐变：**
- HP > 60%：绿色 (50, 200, 50)
- HP 30~60%：黄色 (220, 200, 30)
- HP < 30%：红色 (220, 40, 40)

## 操作说明

| 按键 | 功能 |
|------|------|
| WASD | 移动 |
| Space | 跳跃 |
| Shift | 冲刺 |
| F | 闪避 |
| J | 攻击（三段连招） |
| E | 编辑面板 |
| R | 重置场景 |
| 1/2 | 切换场景 |
| 鼠标左键拖动 | 旋转摄像机 |
| 滚轮 | 调节距离 |

## 编辑面板

- **Camera**：摄像机参数
- **Audio**：主音量/BGM/SFX音量、3D衰减参数
- **UI**：血条尺寸/位置、飘字速度/生命周期/暴击字号
- **Scene**：场景切换、触发器状态
- **Particles**：粒子效果参数
- **Skybox**：天空盒模式/颜色
- **Lighting**：光照参数
- **Combat**：攻击参数、HP/MP调节
- **AI**：敌人属性
- **Objects**：场景物体

## 思考题

1. **FMOD vs miniaudio**：FMOD作为商业中间件，相比miniaudio这样的单头文件库，在3D音效、多平台支持、混音、DSP效果方面有哪些优势？

2. **3D音效优化**：如果场景中有100个敌人同时发出声音，如何做音频优化？（提示：优先级系统、最大同时播放数、距离剔除）

3. **世界坐标投影**：为什么在透视除法时需要检查 clipW > 0？如果 clipW 为负数意味着什么？

4. **伤害飘字**：如果多个伤害数字在同一位置重叠，如何避免视觉混乱？（提示：随机偏移、堆叠、合并）

5. **UI响应式布局**：当窗口大小改变时，HUD元素应该如何适配？固定像素位置 vs 百分比布局 vs 锚点系统各有什么优缺点？

## 下一章预告

第30章将整合所有系统，完成一个完整的3D动作游戏Demo，包括多关卡、BOSS战、完整的游戏循环。
