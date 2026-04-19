# 第23章：3D摄像机系统

## 概述

实现三种3D摄像机模式（轨道/第一人称/第三人称），延续第22章的材质纹理和第21章的Phong光照场景。Tab键切换模式。

---

## 学习目标

1. 理解欧拉角摄像机的数学原理（Yaw/Pitch→方向向量）
2. 实现轨道摄像机（Orbit Camera）— 模型查看器常用
3. 实现FPS第一人称摄像机 — WASD自由移动+鼠标自由看
4. 实现TPS第三人称摄像机 — 跟随角色+平滑插值
5. 理解鼠标锁定（GLFW_CURSOR_DISABLED）
6. 掌握摄像机模式切换的状态管理

---

## 数学原理

### 1. 欧拉角→方向向量

用两个角度定义摄像机朝向：

```
Yaw（偏航角）— 水平旋转，绕Y轴
Pitch（俯仰角）— 垂直旋转，绕X轴

        Y (上)
        |   / forward
        |  /
        | / pitch角
        |/_________ X
       / yaw角
      /
     Z
```

从Yaw和Pitch计算前方向向量：

```cpp
forward.x = cos(pitch) * sin(yaw)   // pitch=0时在XZ平面，yaw决定水平方向
forward.y = sin(pitch)               // pitch决定仰角
forward.z = cos(pitch) * cos(yaw)
```

**Pitch限制在[-89°, 89°]**：如果到达±90°，forward向量退化为纯上/下方向，cross(forward, up)为零向量，摄像机会翻转（万向锁问题）。

### 2. 轨道摄像机（Orbit Camera）

摄像机在一个球面上绕目标点旋转：

```
         ★ 目标点(orbitTarget)
        /|\ 
       / | \  distance
      /  |  \
     /   |   \
    ●         ●     ← 摄像机可以在球面上任意位置
   眼睛位置
```

```cpp
eye.x = target.x + distance * cos(pitch) * sin(yaw)
eye.y = target.y + distance * sin(pitch)
eye.z = target.z + distance * cos(pitch) * cos(yaw)

viewMatrix = LookAt(eye, target, up)
```

### 3. FPS摄像机

摄像机就是玩家的"眼睛"，直接操控摄像机位置：

```cpp
// 前方向（忽略Y，保持水平移动）
flatForward = normalize(forward.x, 0, forward.z)
right = cross(flatForward, up)

// WASD移动
position += flatForward * forwardInput * speed * dt
position += right * rightInput * speed * dt
position.y += upInput * speed * dt    // Space/Ctrl

// 鼠标控制视角
yaw += mouseDeltaX * sensitivity
pitch -= mouseDeltaY * sensitivity   // Y轴反转（屏幕Y向下）
```

**鼠标锁定**：FPS模式需要把鼠标锁在窗口中心：
```cpp
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
```

### 4. TPS摄像机

摄像机跟随一个角色，保持一定距离和角度：

```
                    ● 摄像机
                   /|
                  / |
         distance/ |height
                /  |
               /   |
              ★    ← 角色位置(tpsTarget)
```

**平滑跟随**（指数插值，比线性插值更自然）：

```cpp
float t = 1.0 - exp(-smoothSpeed * dt);   // 越快→t越大→跟随越紧
currentPos = currentPos + (idealPos - currentPos) * t;
```

这比简单的 `lerp(current, target, 0.1)` 更好，因为它**帧率无关**——不管30FPS还是144FPS，跟随速度一致。

### 5. 角色移动方向（相对摄像机）

TPS模式下WASD的移动方向是**相对于摄像机朝向**的，不是世界坐标：

```cpp
// 按W时，角色朝摄像机看的方向移动（忽略Y）
Vec3 camForward = Vec3(sin(camYaw), 0, cos(camYaw));
Vec3 camRight = Vec3(cos(camYaw), 0, -sin(camYaw));

Vec3 moveDir = camForward * forwardInput + camRight * rightInput;
moveDir = normalize(moveDir);
playerPos += moveDir * speed * dt;
```

---

## 三种模式对比

| 特性 | Orbit | FPS | TPS |
|------|-------|-----|-----|
| 操控对象 | 摄像机绕点旋转 | 摄像机自由移动 | 角色移动，摄像机跟随 |
| 鼠标 | 左键拖动旋转 | 自由移动（锁定） | 左键拖动旋转视角 |
| WASD | 不使用 | 移动摄像机 | 移动角色 |
| 滚轮 | 缩放距离 | 调FOV | 调跟随距离 |
| 典型用途 | 模型查看器 | 射击游戏 | 动作游戏 |
| LookAt目标 | 固定点 | eye+forward | 角色位置 |

---

## 操作说明

| 按键 | Orbit模式 | FPS模式 | TPS模式 |
|------|-----------|---------|---------|
| Tab | 切换到FPS | 切换到TPS | 切换到Orbit |
| 鼠标左键拖动 | 旋转视角 | — | 旋转视角 |
| 鼠标移动 | — | 自由看（已锁定） | — |
| WASD | — | 移动摄像机 | 移动角色 |
| Space/Ctrl | — | 上升/下降 | — |
| Shift | — | 加速跑 | — |
| 滚轮 | 缩放距离 | 调FOV | 调跟随距离 |
| E | 编辑面板 | 编辑面板 | 编辑面板 |
| L | 切换光照 | 切换光照 | 切换光照 |

## ImGui编辑面板

| 标签页 | 内容 |
|--------|------|
| Camera | 模式切换、Yaw/Pitch/FOV、模式专属参数（距离/速度/灵敏度/平滑） |
| Objects | 每个物体的位置/缩放/材质/光泽度 |
| Lighting | 方向光+点光源参数、日光/夜晚预设 |
| Render | 背景色/线框/GPU信息 |

---

## 新增文件

| 文件 | 说明 |
|------|------|
| `source/engine3d/Camera3D.h` | 3D摄像机类（Orbit/FPS/TPS三模式） |
| `examples/23_3d_camera/main.cpp` | 摄像机演示（材质场景+角色跟随） |

---

## 思考题

1. FPS摄像机为什么用flatForward（去掉Y分量）而不是直接用forward移动？如果不去掉Y会怎样？
2. 为什么Pitch要限制在[-89°, 89°]而不是[-90°, 90°]？90°时cross(forward, up)的结果是什么？
3. TPS的指数平滑 `1-exp(-speed*dt)` 比 `speed*dt` 有什么优势？
4. 从Orbit切换到FPS时，为什么要把当前眼睛位置赋给fpsPosition？不赋值会怎样？
5. 如果TPS摄像机和墙壁之间有物体遮挡，怎么解决？（提示：射线检测）

---

## 下一章预告

第24章：3D角色控制器 — 重力、地面射线检测、WASD移动角色（不是摄像机）、跳跃、冲刺。
