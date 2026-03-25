# 第16章：房间与关卡系统

## 学习目标

1. 理解房间（关卡）的数据结构设计
2. 实现房间切换与淡入淡出过渡效果
3. 掌握触发器系统（传送门、存档点）
4. 学会用代码定义关卡，为第18章的数据驱动做铺垫

---

## 核心概念

### 什么是房间系统？

银河恶魔城类游戏的世界由多个"房间"组成，每个房间是一个独立的游戏区域。玩家通过传送门在房间之间穿梭。

```
 ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
 │  Training    │    │   Dungeon    │    │  BOSS Room   │
 │  Ground      │───→│   Dark       │───→│  Chamber     │
 │  (1个敌人)   │←───│   (3个敌人)  │←───│  (BOSS级敌人)│
 └──────────────┘    └──────────────┘    └──────────────┘
```

### 房间数据结构

每个房间包含以下数据：

| 数据 | 说明 |
|------|------|
| name | 唯一标识符（用于切换引用） |
| displayName | 显示名称 |
| width × height | 瓦片尺寸 |
| bgColor | 背景颜色（不同房间不同氛围） |
| platforms | 平台列表（地面、墙壁、悬空平台） |
| enemySpawns | 敌人出生点 |
| triggers | 触发器列表（传送门、存档点） |
| defaultSpawn | 玩家默认出生位置 |
| bgmName | 背景音乐 |

### 触发器系统

触发器是放在场景中的隐形区域，玩家进入后触发效果：

| 类型 | 效果 | 交互方式 |
|------|------|----------|
| Door（传送门） | 切换到另一个房间 | 靠近后按W键进入 |
| Checkpoint（存档点） | 设置重生位置 | 走过自动保存 |
| Event（事件） | 触发自定义逻辑 | 预留，后续使用 |

### 房间切换过渡

切换房间不是瞬间完成的，有一个"淡出→加载→淡入"的过渡过程：

```
当前房间                              新房间
  [正常] → [淡出变黑] → [全黑/加载] → [淡入显现] → [正常]
  alpha:0    0→1           1            1→0          0
```

---

## 本章包含的3个房间

### 房间1：Training Ground（训练场）

- 30×15瓦片，深蓝背景
- 1个近战敌人（练习用）
- 1个存档点（入口处）
- 右侧传送门 → 地牢

### 房间2：Dark Dungeon（地牢）

- 45×15瓦片，暗紫色背景
- 3个敌人（2近战 + 1远程）
- 中间有存档点
- 左侧传送门 → 训练场
- 右侧传送门 → BOSS间

### 房间3：BOSS Chamber（BOSS间）

- 35×15瓦片，暗红色背景
- 2个强力敌人（高血量高攻击）
- 左侧传送门 → 地牢

---

## 操作说明

| 按键 | 功能 |
|------|------|
| A/D | 移动 |
| Space | 跳跃 |
| J | 攻击 |
| W 或 ↑ | 进入传送门（靠近传送门时） |
| Tab | 显示/隐藏调试信息 |
| R | 从存档点重生（回到最近的存档点） |

### 传送门使用方法

1. 走到传送门旁（蓝色发光区域）
2. ImGui面板会提示"Press W to enter"
3. 按W键，画面淡出变黑
4. 自动加载新房间
5. 画面淡入，玩家出现在新房间的对应位置

---

## 新增文件

### 引擎源码

| 文件 | 说明 |
|------|------|
| `source/scene/RoomSystem.h` | 房间、触发器、房间管理器定义 |
| `source/scene/RoomSystem.cpp` | 房间管理器实现（切换、过渡、触发器检测） |

### 素材

| 文件 | 说明 |
|------|------|
| `data/audio/sfx/transition.mp3` | 房间切换转场音效 |
| `data/audio/bgm/vivo.mp3` | 第二首BGM（地牢/BOSS间使用） |

### 示例

| 文件 | 说明 |
|------|------|
| `examples/16_room_system/main.cpp` | 3个房间的完整关卡演示 |

---

## 关键代码解析

### 房间定义（代码方式）

```cpp
Room r;
r.name = "dungeon";
r.displayName = "Dark Dungeon";
r.width = 45; r.height = 15;
r.bgColor = Vec4(0.08f, 0.06f, 0.12f, 1);

r.AddGround(0, 45);           // 全地面
r.AddWalls(10);                // 左右墙壁
r.AddPlatform(18, 5, 6);      // 悬空平台

r.AddEnemy(EnemyType::Melee, Vec2(280, 86), 200, 500, 50, 10);
r.AddCheckpoint(Vec2(450, 86));
r.AddDoor(Vec2(60, 86), "training", Vec2(860, 86), "<- Training");

g_roomMgr.AddRoom(r);
```

### 淡入淡出过渡

```cpp
void RoomManager::UpdateTransition(float dt) {
    if (m_transState == TransitionState::FadeOut) {
        m_transAlpha += speed * dt;     // 逐渐变黑
        if (m_transAlpha >= 1.0f) {
            LoadRoom(pendingRoom);       // 全黑时加载新房间
            m_transState = FadeIn;       // 开始淡入
        }
    } else if (m_transState == TransitionState::FadeIn) {
        m_transAlpha -= speed * dt;     // 逐渐显现
        if (m_transAlpha <= 0) {
            m_transState = None;         // 过渡结束
        }
    }
}
```

渲染时只需在最后画一个覆盖全屏的黑色半透明矩形：

```cpp
DrawRect(screenCenter, screenSize, Vec4(0, 0, 0, transitionAlpha));
```

### 存档点与重生

- 玩家走过存档点时自动保存当前房间名和位置
- 玩家死亡或按R键时，自动过渡到存档的房间和位置
- 血量自动回满

---

## 设计要点

### 为什么 loseRange 和 detectionRange 设不同的值？

（同第15章）防止敌人在检测边界上抖动切换状态。

### 为什么传送门需要按W而不是自动触发？

自动触发会导致玩家不小心碰到就被传送走，体验很差。按键确认让玩家有控制权。而存档点用自动触发是因为存档是无害的操作。

### 每个房间的BGM可以不同

房间切换时回调函数会自动切换BGM。训练场用欢快的曲子，地牢和BOSS间用紧张的曲子。

---

## 思考题

1. 如果要实现"打败所有敌人才能开门"的机制，需要在哪里加判断？
2. 如何实现房间内的"锁门→打败敌人→开门"的房间锁定机制？
3. 第18章会将房间数据改为JSON文件加载，想想Room结构体的哪些字段需要序列化？

---

## 下一章预告

第17章：UI系统 — 血条HUD、伤害飘字、对话框、暂停菜单。
