# 第26章：3D战斗系统 (3D Combat System)

## 学习目标

1. **AABB碰撞检测** — 角色不能穿过墙壁和箱子，简单位置修正
2. **球体碰撞检测** — 球球碰撞、球胶囊碰撞、点到线段距离
3. **Hitbox/Hurtbox机制** — 攻击判定区域与受击判定区域分离
4. **骨骼动画攻击** — 按J播放Punching骨骼动画，攻击中锁定移动
5. **Hit Stop顿帧效果** — 全局时间缩放实现打击感
6. **连招状态机** — 三段攻击连招（输入缓冲、伤害递增）
7. **受击视觉反馈** — 敌人闪红、击退、无敌时间、头顶血条

## AABB碰撞检测

### 原理

角色用球体碰撞体（位置+半径），场景物体用AABB（轴对齐包围盒）。每帧检测角色球体是否与AABB相交，若相交则将角色推出：

```
1. 找到AABB上离角色最近的点closest
   closest.x = clamp(playerPos.x, aabb.min.x, aabb.max.x)
   closest.y = clamp(playerPos.y, aabb.min.y, aabb.max.y)
   closest.z = clamp(playerPos.z, aabb.min.z, aabb.max.z)

2. 计算角色到最近点的距离
   diff = playerPos - closest
   dist = |diff|

3. 如果 dist < playerRadius，说明穿透了
   pushDir = normalize(diff)
   playerPos = closest + pushDir * playerRadius
```

### AABB相交测试

两个AABB相交的充要条件是三个轴上都有重叠：

```
intersects = (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
             (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
             (a.min.z <= b.max.z && a.max.z >= b.min.z)
```

## 球体碰撞检测数学

### 球球碰撞（Sphere-Sphere）

```
d² = (Ax-Bx)² + (Ay-By)² + (Az-Bz)²
collision = d² ≤ (rA + rB)²
```

### 点到线段距离（Point-to-Segment Distance）

```
t = dot(AP, AB) / dot(AB, AB)
t = clamp(t, 0, 1)
closest = A + t × AB
distance = |P - closest|
```

### 球胶囊碰撞（Sphere-Capsule）

```
dist = PointToSegmentDistance(sphere.center, capsule.bottom, capsule.top)
collision = dist ≤ (sphere.radius + capsule.radius)
```

## Hit Stop原理

Hit Stop（顿帧）增强打击感的核心技术：

1. **触发** — 攻击命中时启动计时器（通常0.05~0.15秒）
2. **效果** — 游戏时间缩放到接近0（如0.05），物理和动画几乎暂停
3. **恢复** — 计时器归零后时间缩放恢复为1.0
4. **注意** — 计时器用真实时间(realDt)递减，不受缩放影响

```cpp
hitStop.Update(realDt);
float gameDt = realDt * hitStop.GetTimeScale();
player.Update(gameDt);  // 移动、动画等使用缩放后的时间
```

## 连招状态机设计

```
None → Attack1 (按J)
Attack1 → Attack2 (攻击结束后ComboWindow内再按J)
Attack2 → Attack3 (同上)
Attack3/超时 → None → 自动回Idle动画

伤害递增：Attack1=1.0× | Attack2=1.2× | Attack3=1.8×
```

攻击期间：
- 播放Punching骨骼动画（SwitchAnim切到索引2）
- 锁定WASD移动输入（g_comboState != None时不读取移动键）
- 攻击结束后进入ComboWindow，超时未输入则自动SwitchAnim回Idle

## Shader说明

复用第22/25章的着色器：

- **material.vs/fs** — 场景物体、敌人球体渲染（Phong光照+PBR材质贴图）
- **skinning.vs/fs** — 玩家X Bot骨骼动画渲染（基于material扩展，增加uBones[100]矩阵）

敌人受击闪红通过 `g_matShader->SetVec4("uBaseColor", Vec4(1, 0.15, 0.15, 1))` 实现。

## 操作说明

| 按键 | 功能 |
|------|------|
| WASD | 移动角色（D=-1, A=1, 相对摄像机方向） |
| Space | 跳跃（支持二段跳） |
| J | 攻击（三段连招，播放Punching骨骼动画） |
| Shift | 冲刺跑 |
| F | 闪避冲刺（有冷却） |
| 鼠标左键拖动 | 旋转摄像机 |
| 滚轮 | 调整距离 |
| E | 编辑面板 |
| R | 重置（角色+敌人全部重置） |

## ImGui编辑面板

5个标签页：

- **Camera** — Yaw/Pitch/Distance/FOV/Height/灵敏度
- **Combat** — 攻击伤害/击退力/Hitbox半径/Hit Stop时长/无敌时间/时间缩放
- **Player** — HP/位置/移动速度/冲刺速度/跳跃力/闪避速度
- **Enemies** — 每个敌人的HP/位置/状态/复活按钮
- **Lighting** — 方向光(方向/颜色/强度) + 点光源(位置/颜色/强度)

## 思考题

1. **碰撞检测优化** — 100+敌人时逐一检测效率低，如何用空间划分（网格/八叉树/BVH）加速？

2. **GJK算法** — 球体碰撞是最简单的情况。凸多面体碰撞（OBB vs OBB）如何用GJK+EPA实现？

3. **动画帧判定** — 当前在按键瞬间检测碰撞。如何改为基于动画帧的判定（特定帧范围内激活Hitbox）？

4. **受击硬直与浮空** — 如何实现被连续攻击时的硬直和浮空连击？

5. **分离轴定理(SAT)** — AABB碰撞是SAT的特例。如何扩展到OBB（有向包围盒）碰撞？

## 下一章预告

**第27章：敌人AI + PhysX** — 行为树AI、A*寻路、多敌人协作攻击、PhysX物理引擎集成。
