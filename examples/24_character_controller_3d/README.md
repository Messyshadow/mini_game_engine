# 第24章：3D角色控制器

## 概述

实现3D角色控制器——WASD移动角色、跳跃（二段跳）、冲刺跑、闪避冲刺。TPS摄像机跟随角色，角色朝向平滑转向移动方向。

---

## 学习目标

1. 实现基于加速度的角色移动（非瞬间满速）
2. 理解相对摄像机方向的移动计算
3. 实现跳跃和重力系统
4. 实现闪避冲刺（Dash）
5. 掌握角色朝向的平滑转向
6. 理解空中操控与地面操控的区别

---

## 数学原理

### 1. 相对摄像机的移动方向

玩家按W时，角色应该向**摄像机面对的方向**前进，不是世界Z轴方向：

```
摄像机朝向（yaw角决定）
     ↗
    ╱    按W = 角色向这个方向走
   ╱
  ★ 角色
```

```cpp
float camRad = cameraYaw * 0.0174533f;  // 度→弧度
Vec3 camForward(sin(camRad), 0, cos(camRad));  // 摄像机前方（忽略Y）
Vec3 camRight(cos(camRad), 0, -sin(camRad));   // 摄像机右方

Vec3 moveDir = camForward * inputW + camRight * inputD;
moveDir = normalize(moveDir);  // 对角线移动不超速
```

### 2. 加速度移动

不是设置速度=目标速度，而是**逐渐趋近**目标速度：

```cpp
// 每帧靠近目标速度
velocity.x += (targetVel.x - velocity.x) * min(1.0, acceleration * dt);
```

效果：松手后角色有惯性滑行，不是瞬间停下。

**空中操控更弱**：`airControl = 0.3` 意味着空中加速度只有地面的30%。

### 3. 角色朝向平滑转向

角色面朝移动方向，但不是瞬间转过去——平滑插值：

```cpp
targetYaw = atan2(moveDir.x, moveDir.z);  // 目标朝向

// 处理角度环绕（如从350°转到10°，应该转20°而不是340°）
float diff = targetYaw - currentYaw;
while (diff > π) diff -= 2π;
while (diff < -π) diff += 2π;

currentYaw += diff * min(1.0, turnSpeed * dt);
```

### 4. 重力与跳跃

```cpp
// 每帧更新
if (!grounded) {
    velocity.y -= gravity * dt;     // 重力加速
    if (velocity.y < -maxFall)      // 限制最大下落速度
        velocity.y = -maxFall;
}

// 跳跃：直接设置垂直速度
if (jumpPressed && jumpCount < maxJumps) {
    velocity.y = jumpForce;
    jumpCount++;
}
```

### 5. 冲刺（Dash）

短暂的高速移动，有冷却时间：

```
正常移动: ●──────→ (6m/s)
冲刺:     ●══════════→ (20m/s, 持续0.2秒)
冷却:     [CD: 1.0s]
```

冲刺方向 = 角色当前朝向。冲刺中忽略重力和正常移动输入。

### 6. 简化地面检测

本章用最简单的方式——与固定高度比较：

```cpp
if (position.y <= groundHeight + halfHeight) {
    position.y = groundHeight + halfHeight;
    grounded = true;
}
```

第27章引入PhysX后会替换为射线检测（Raycast），可以支持斜面和复杂地形。

---

## 操作说明

| 按键 | 功能 |
|------|------|
| WASD | 移动角色（相对摄像机方向） |
| Space | 跳跃（支持二段跳） |
| Shift | 冲刺跑（加速移动） |
| F | 闪避冲刺（短暂高速，有1秒冷却） |
| 鼠标左键拖动 | 旋转摄像机 |
| 滚轮 | 调整跟随距离 |
| Tab | 切换Orbit/TPS模式 |
| E | 编辑面板 |
| R | 重置角色位置 |
| L | 切换光照 |

## ImGui编辑面板

| 标签页 | 内容 |
|--------|------|
| Physics | 移动速度/加速度/空中控制/跳跃力/重力/冲刺参数/转向速度 |
| Camera | 模式/距离/高度/平滑/灵敏度 |
| Objects | 场景物体的位置/材质/属性 |
| Lighting | 方向光+点光源+预设 |

---

## 场景设计

| 物体 | 用途 |
|------|------|
| 大地面(30x30) | 自由奔跑区域 |
| 砖墙x2 | 障碍物参照 |
| 金属柱 | 高处参照 |
| 木箱堆叠 | 跳跃测试 |
| 金属球 | 移动参照物 |
| 平台 | 跳上去测试跳跃高度 |
| X Bot | 玩家角色（朝向跟随移动方向） |

---

## 新增文件

| 文件 | 说明 |
|------|------|
| `source/engine3d/CharacterController3D.h` | 3D角色控制器（移动/跳跃/冲刺/朝向） |
| `examples/24_character_controller_3d/main.cpp` | 角色控制演示 |

---

## 思考题

1. 为什么移动用加速度而不是直接设速度？对手感有什么影响？
2. airControl=0.3意味着什么？设为0和设为1分别是什么体验？
3. 角度环绕处理（while diff>π）为什么必要？不处理会出什么BUG？
4. 冲刺中为什么要跳过重力？如果不跳过会怎样？
5. 简化地面检测（和固定高度比较）有什么缺陷？什么场景下会出问题？

---

## 下一章预告

第25章：骨骼动画 — 从FBX读取骨骼层级，顶点蒙皮，播放Idle/Run/Attack/Death动画，动画状态机。这是3D部分最难的一章。
