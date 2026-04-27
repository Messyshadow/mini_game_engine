# 第27章：敌人AI + PhysX物理集成

## 学习目标

1. **PhysX物理引擎** — 初始化流程、刚体类型、碰撞体注册
2. **射线检测** — PhysX Raycast实现地面检测
3. **AI状态机** — Patrol巡逻/Chase追击/Attack攻击/Dead死亡
4. **视野检测** — 基于距离+FOV角度的玩家发现机制
5. **PhysX碰撞** — 场景所有物体注册为静态碰撞体，角色不能穿过

## PhysX物理引擎

### 初始化流程

```
PxCreateFoundation → PxCreatePhysics → createScene → createMaterial
     ↓                    ↓                ↓
   内存分配器          物理世界         场景（重力、碰撞过滤）
```

1. **Foundation** — PhysX底层，管理内存分配和错误回调
2. **Physics** — 物理世界对象，创建刚体、形状、材质的工厂
3. **Scene** — 物理场景，包含所有Actor，执行模拟
4. **Material** — 物理材质（静摩擦、动摩擦、弹性系数）

### 刚体类型

| 类型 | 说明 | 用途 |
|------|------|------|
| **PxRigidStatic** | 静态刚体，不会移动 | 地面、墙壁、箱子、柱子 |
| **PxRigidDynamic** | 动态刚体，受力和重力影响 | 可推动的物体、弹丸 |
| **Kinematic** | 动态刚体+eKINEMATIC标志，由代码控制位置 | 玩家角色、电梯 |

### Kinematic刚体

玩家角色使用Kinematic刚体：
- 不受PhysX重力影响（由CharacterController3D自己处理重力）
- 通过 `setKinematicTarget()` 设置目标位置
- PhysX自动处理与静态碰撞体的碰撞
- 其他刚体无法推动Kinematic体

```cpp
// 创建Kinematic胶囊体（竖立旋转90度）
body = physics->createRigidDynamic(PxTransform(pos));
body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
PxShape* shape = physics->createShape(PxCapsuleGeometry(radius, halfHeight), *material);
shape->setLocalPose(PxTransform(PxQuat(PxHalfPi, PxVec3(0,0,1))));
```

### 射线检测（Raycast）

PhysX射线检测用于：
- 地面检测：从角色脚下向下发射射线
- 视线检测：判断敌人与玩家之间是否有障碍物

```
origin ●───────→ direction
       |  maxDist  |
       └───────────┘
       如果命中：返回hitPoint和hitNormal
```

```cpp
PxRaycastBuffer hit;
bool result = scene->raycast(origin, direction, maxDist, hit);
if (result && hit.hasBlock) {
    PxVec3 hitPoint = hit.block.position;
    PxVec3 hitNormal = hit.block.normal;
}
```

### 固定时间步进

物理模拟使用固定时间步（1/60秒），用累加器处理帧率波动：

```cpp
accumulator += dt;
while (accumulator >= stepSize) {
    scene->simulate(stepSize);
    scene->fetchResults(true);
    accumulator -= stepSize;
}
```

## AI状态机

### 状态转换图

```
          发现玩家
Patrol ──────────→ Chase
  ↑                  │
  │ 丢失目标         │ 进入攻击范围
  │                  ↓
  ├──────────── Attack
  │                  │
  │                  │ HP <= 0
  │                  ↓
  │               Dead
  │
  ├── Retreat（低HP时回撤）
```

### 各状态行为

**Patrol（巡逻）**
- 在A、B两点之间往返移动
- 到达目标点后切换方向
- 每帧检测是否能看到玩家

**Chase（追击）**
- 向玩家位置移动
- 进入攻击范围 → 切换到Attack
- 玩家超出loseRange → 返回Patrol

**Attack（攻击）**
- 面向玩家
- 按攻击冷却节奏攻击
- 玩家跑出攻击范围 → 切换到Chase

**Dead（死亡）**
- HP降为0触发
- 不再渲染和更新

### 视野检测（FOV）

```
         前方向(forward)
            ↑
           /|\
          / | \
         /  |  \  fovAngle/2
        /   |   \
       /    |    \
      ------●------  detectionRange
```

判定条件：
1. 玩家距离 < detectionRange
2. 玩家方向与敌人朝向的夹角 < fovAngle/2

```cpp
float dot = forward · dirToPlayer;  // 点积 = cos(θ)
float halfFov = fovAngle * 0.5 * π / 180;
canSee = (dot >= cos(halfFov));
```

## 行为树 vs 状态机

| 对比 | 状态机(FSM) | 行为树(BT) |
|------|-------------|------------|
| 结构 | 状态+转换条件 | 树形节点（Selector/Sequence/Action） |
| 扩展性 | 状态多时转换爆炸 | 节点可复用、组合灵活 |
| 调试 | 直观，一眼看出当前状态 | 需要可视化工具 |
| 适用 | 简单AI（3-5个状态） | 复杂AI（BOSS、多阶段） |
| 本章 | ✅ 使用FSM | 留给更复杂的BOSS AI |

本章使用FSM，因为3个敌人的行为简单明确（巡逻→追击→攻击），状态机足够清晰。

## 场景设计

```
      ┌──────────────── Arena Wall N ────────────────┐
      │                                               │
      │   Wall1(-5,3)      ●PatrolA   ●PatrolB       │
  W   │       ■            Enemy A巡逻路线            │  E
  a   │                                               │  a
  l   │   Player(0,0)     Pillar(7,7)                 │  l
  l   │      ★              ■                         │  l
      │                                               │
  W   │   Crate1(-7,-5)  Wall2(5,-3)                  │  E
      │      ■              ■                         │
      │                                               │
      │   Enemy B巡逻     Wall3(0,-8)   Enemy C巡逻   │
      │   (-8,-2)↔(-2,-8)    ■        (4,-4)↔(10,-10)│
      │                                               │
      └──────────────── Arena Wall S ────────────────┘
```

## 操作说明

| 按键 | 功能 |
|------|------|
| WASD | 移动角色（相对摄像机方向） |
| Space | 跳跃（支持二段跳） |
| J | 攻击（三段连招） |
| Shift | 冲刺跑 |
| F | 闪避冲刺（有冷却） |
| 鼠标左键拖动 | 旋转摄像机 |
| 滚轮 | 调整距离 |
| E | 编辑面板 |
| R | 重置（角色+敌人全部重置） |

## ImGui编辑面板

5个标签页：

- **Camera** — Yaw/Pitch/Distance/FOV/Height/灵敏度
- **Physics** — PhysX状态、步进时间、调试可视化
- **AI** — 每个敌人的状态/HP/检测范围/攻击范围/速度/FOV/冷却
- **Combat** — 攻击伤害/击退力/Hitbox半径/Hit Stop/无敌时间/玩家HP
- **Lighting** — 方向光+点光源参数

## 思考题

1. **A*寻路** — 当前敌人直线追击玩家，遇到墙壁会卡住。如何实现网格化A*寻路，让敌人绕过障碍物？

2. **行为树实现** — 如果要把FSM改为行为树，Selector和Sequence节点如何设计？如何处理并行行为（边跑边检测）？

3. **多敌人协作** — 3个敌人同时追击玩家时会挤在一起。如何实现包围策略（一个正面、两个侧翼）？

4. **PhysX Character Controller** — 本章用Kinematic刚体模拟角色。PhysX自带的PxController有什么优势？如何处理台阶和斜坡？

5. **碰撞过滤** — 当前所有物体都互相碰撞。如何用PhysX的碰撞组（Collision Group）实现：敌人之间不碰撞、子弹穿过友方？

## 下一章预告

**第28章：场景与关卡** — 场景管理器、触发器系统、粒子特效、天空盒、阴影映射。
