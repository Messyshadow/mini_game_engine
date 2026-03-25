# 第五章：2D碰撞检测

---

## 📚 本章概述

本章将学习2D游戏中的碰撞检测系统：
1. AABB（轴对齐包围盒）碰撞检测
2. 圆形碰撞检测
3. 混合碰撞检测（AABB vs Circle）
4. 碰撞响应（推出）

---

## 🎯 学习目标

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           本章核心知识点                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. AABB (Axis-Aligned Bounding Box) 轴对齐包围盒                           │
│                                                                             │
│     特点：边与坐标轴平行，用中心点+半尺寸表示                                 │
│                                                                             │
│        ┌─────────────┐                                                     │
│        │             │                                                     │
│        │    center   │  halfSize = (w/2, h/2)                             │
│        │      ●      │                                                     │
│        │             │                                                     │
│        └─────────────┘                                                     │
│                                                                             │
│     碰撞检测：两个AABB碰撞 ⟺ 在X轴和Y轴上都有重叠                           │
│                                                                             │
│  2. Circle 圆形碰撞体                                                        │
│                                                                             │
│        ╭─────────╮                                                         │
│       ╱           ╲                                                        │
│      │   center    │  radius = r                                          │
│      │      ●      │                                                       │
│       ╲           ╱                                                        │
│        ╰─────────╯                                                         │
│                                                                             │
│     碰撞检测：两个圆碰撞 ⟺ 圆心距离 < 半径之和                              │
│                                                                             │
│  3. 碰撞响应                                                                 │
│                                                                             │
│     检测到碰撞后，需要：                                                     │
│     ① 计算穿透深度 (penetration)                                           │
│     ② 计算推出方向 (normal)                                                │
│     ③ 将物体推出：position += normal × penetration                        │
│                                                                             │
│         碰撞前：          碰撞后（推出）：                                   │
│       ┌───┐              ┌───┐                                             │
│       │ A │←→│ B │       │ A │  │ B │                                      │
│       └───┘  └───┘       └───┘  └───┘                                      │
│         重叠                 分离                                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 💻 代码结构

### 文件清单

```
05_collision/
└── main.cpp          ← 示例代码（约443行）
```

### 核心代码解析

#### 1. AABB碰撞检测

```cpp
// AABB结构
struct AABB {
    Vec2 center;    // 中心点
    Vec2 halfSize;  // 半尺寸
    
    Vec2 GetMin() const { return center - halfSize; }
    Vec2 GetMax() const { return center + halfSize; }
};

// AABB vs AABB 碰撞检测
HitResult Collision::AABBvsAABB(const AABB& a, const AABB& b) {
    HitResult result;
    
    // 计算两个AABB在X轴和Y轴上的距离
    Vec2 delta = b.center - a.center;                    // 中心点差
    Vec2 overlap = a.halfSize + b.halfSize - Vec2(std::abs(delta.x), std::abs(delta.y));
    
    // 如果任一轴上没有重叠，则没有碰撞
    if (overlap.x <= 0 || overlap.y <= 0) {
        result.hit = false;
        return result;
    }
    
    result.hit = true;
    
    // 选择穿透最小的轴作为推出方向
    if (overlap.x < overlap.y) {
        result.penetration = overlap.x;
        result.normal = Vec2(delta.x > 0 ? -1.0f : 1.0f, 0.0f);
    } else {
        result.penetration = overlap.y;
        result.normal = Vec2(0.0f, delta.y > 0 ? -1.0f : 1.0f);
    }
    
    return result;
}
```

#### 2. 圆形碰撞检测

```cpp
// Circle vs Circle 碰撞检测
HitResult Collision::CirclevsCircle(const Circle& a, const Circle& b) {
    HitResult result;
    
    Vec2 delta = b.center - a.center;
    float distSquared = delta.x * delta.x + delta.y * delta.y;  // 避免开方
    float radiusSum = a.radius + b.radius;
    
    // 距离平方 > 半径和平方 → 没碰撞
    if (distSquared > radiusSum * radiusSum) {
        result.hit = false;
        return result;
    }
    
    result.hit = true;
    
    float dist = std::sqrt(distSquared);
    result.penetration = radiusSum - dist;
    
    // 推出方向：从A指向B的单位向量
    if (dist > 0.0001f) {
        result.normal = delta / dist * -1.0f;  // 指向A的方向
    } else {
        result.normal = Vec2(1, 0);  // 重合时默认向右推
    }
    
    return result;
}
```

#### 3. 圆形与AABB碰撞检测

```cpp
// Circle vs AABB 碰撞检测
HitResult Collision::CirclevsAABB(const Circle& circle, const AABB& aabb) {
    HitResult result;
    
    // 找到AABB上离圆心最近的点
    Vec2 closestPoint;
    closestPoint.x = std::clamp(circle.center.x, aabb.GetMin().x, aabb.GetMax().x);
    closestPoint.y = std::clamp(circle.center.y, aabb.GetMin().y, aabb.GetMax().y);
    
    // 计算最近点到圆心的距离
    Vec2 delta = circle.center - closestPoint;
    float distSquared = delta.x * delta.x + delta.y * delta.y;
    
    if (distSquared > circle.radius * circle.radius) {
        result.hit = false;
        return result;
    }
    
    result.hit = true;
    
    float dist = std::sqrt(distSquared);
    result.penetration = circle.radius - dist;
    
    if (dist > 0.0001f) {
        result.normal = delta / dist;
    } else {
        // 圆心在AABB内部
        result.normal = Vec2(0, 1);
    }
    
    return result;
}
```

#### 4. 碰撞响应

```cpp
void ResolveCollisions() {
    for (auto& obstacle : g_obstacles) {
        HitResult hit;
        
        // 根据碰撞体类型选择检测方法
        if (g_player.type == ColliderType::Box && obstacle.type == ColliderType::Box) {
            hit = Collision::AABBvsAABB(g_player.GetAABB(), obstacle.GetAABB());
        }
        else if (g_player.type == ColliderType::Circle && obstacle.type == ColliderType::Circle) {
            hit = Collision::CirclevsCircle(g_player.GetCircle(), obstacle.GetCircle());
        }
        else if (g_player.type == ColliderType::Circle && obstacle.type == ColliderType::Box) {
            hit = Collision::CirclevsAABB(g_player.GetCircle(), obstacle.GetAABB());
        }
        // ... 其他组合
        
        if (hit.hit) {
            // 碰撞响应：将玩家推出碰撞区域
            g_player.position += hit.normal * hit.penetration;
        }
    }
}
```

---

## 🎮 操作说明

| 按键 | 功能 |
|------|------|
| W/A/S/D | 移动玩家（蓝色物体） |
| 空格 | 切换玩家碰撞形状（方块/圆形） |
| R | 重置玩家位置 |
| ESC | 退出程序 |

---

## 📊 界面说明

```
┌─────────────────────────────────────────────────────────────────┐
│                        渲染窗口                                  │
│  ┌────────────────────────────────────────────────────────┐    │
│  │                                                        │    │
│  │   灰色墙壁（静态AABB）                                  │    │
│  │ ┌──────────────────────────────────────────────────┐  │    │
│  │ │                                                  │  │    │
│  │ │                                                  │  │    │
│  │ │    ╭───╮                                        │  │    │
│  │ │    │玩家│    ┌─────┐      ⬤                     │  │    │
│  │ │    │蓝色│    │ 红色 │    绿色                    │  │    │
│  │ │    ╰───╯    │ AABB │    圆形                    │  │    │
│  │ │             └─────┘                              │  │    │
│  │ │                                                  │  │    │
│  │ │      ┌──────────────────┐      ⬤                │  │    │
│  │ │      │    橙色AABB      │    紫色               │  │    │
│  │ │      └──────────────────┘    圆形               │  │    │
│  │ │                                                  │  │    │
│  │ └──────────────────────────────────────────────────┘  │    │
│  │   灰色地面（静态AABB）                                  │    │
│  └────────────────────────────────────────────────────────┘    │
│                                                                 │
│   碰撞时：玩家变黄色，障碍物变亮                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────┐  ┌─────────────────────────┐
│ Collision Demo          │  │ Collision Info          │
├─────────────────────────┤  ├─────────────────────────┤
│ FPS: 60.0               │  │ AABB vs AABB:           │
│ ─────────────────────── │  │ • Check X,Y overlap     │
│ Controls:               │  │ • Fast and simple       │
│ • WASD  - Move player   │  │ ─────────────────────── │
│ • Space - Toggle shape  │  │ Circle vs Circle:       │
│ • R     - Reset         │  │ • Compare dist to radii │
│ ─────────────────────── │  │ • Use squared distance  │
│ Player:                 │  │ ─────────────────────── │
│   Position: (320, 400)  │  │ Collision Response:     │
│   Shape: Box (AABB)     │  │ • Calc penetration      │
│   Colliding: YES        │  │ • Push by normal×pen   │
│ ─────────────────────── │  └─────────────────────────┘
│ Hit! Pen: 5.2           │
│ Normal: (-1.0, 0.0)     │
└─────────────────────────┘
```

---

## 📐 碰撞检测详解

### AABB重叠检测原理

```
    两个AABB在X轴上重叠的条件：
    
    A.max.x > B.min.x  且  B.max.x > A.min.x
    
          A                    B
    ├─────────┤          ├─────────┤
    min      max         min      max
    
    重叠：  A ├───────────┤
            B      ├───────────┤
                   ↑     ↑
                重叠区域
    
    不重叠：A ├─────────┤
            B               ├─────────┤
                   ↑
                有间隙
```

### 穿透深度计算

```
    穿透深度 = 两物体需要分开的最小距离
    
    AABB情况：选择X轴和Y轴中穿透较小的那个
    
    ┌───────────┐
    │     A     │
    │   ┌───────┼───┐
    │   │ 重叠  │   │
    └───┼───────┘   │
        │     B     │
        └───────────┘
    
    X轴穿透 = overlapX
    Y轴穿透 = overlapY
    
    如果 overlapX < overlapY：
        从X轴方向推出（水平）
    否则：
        从Y轴方向推出（垂直）
```

### 碰撞法线方向

```
    法线(Normal)指向需要推出的方向
    
    通常是从B指向A（将A推开）
    
        ┌───┐
        │ A │ ← normal
        │ ←─┼───┐
        └───┤ B │
            └───┘
    
    推出公式：A.position += normal * penetration
```

---

## 🔧 关键类说明

### AABB - 轴对齐包围盒

```cpp
struct AABB {
    Vec2 center;     // 中心点
    Vec2 halfSize;   // 半尺寸 (宽/2, 高/2)
    
    AABB() = default;
    AABB(const Vec2& center, const Vec2& halfSize);
    
    Vec2 GetMin() const;  // 左下角
    Vec2 GetMax() const;  // 右上角
    
    bool Contains(const Vec2& point) const;       // 点是否在内部
    bool Intersects(const AABB& other) const;     // 是否相交
    Vec2 GetPenetration(const AABB& other) const; // 获取穿透向量
};
```

### Circle - 圆形碰撞体

```cpp
struct Circle {
    Vec2 center;   // 圆心
    float radius;  // 半径
    
    Circle() = default;
    Circle(const Vec2& center, float radius);
    
    bool Contains(const Vec2& point) const;
    bool Intersects(const Circle& other) const;
};
```

### HitResult - 碰撞结果

```cpp
struct HitResult {
    bool hit = false;        // 是否碰撞
    float penetration = 0;   // 穿透深度
    Vec2 normal;             // 推出方向（单位向量）
    Vec2 point;              // 碰撞点（可选）
};
```

### Collision - 碰撞检测工具类

```cpp
class Collision {
public:
    static HitResult AABBvsAABB(const AABB& a, const AABB& b);
    static HitResult CirclevsCircle(const Circle& a, const Circle& b);
    static HitResult CirclevsAABB(const Circle& circle, const AABB& aabb);
    static HitResult PointvsAABB(const Vec2& point, const AABB& aabb);
    static HitResult PointvsCircle(const Vec2& point, const Circle& circle);
};
```

---

## 🎯 练习建议

1. **测试不同碰撞组合**：按空格切换玩家形状，观察不同碰撞效果
2. **观察推出方向**：故意撞入障碍物，观察从哪个方向被推出
3. **理解穿透深度**：快速移动使穿透更深，观察推出距离
4. **添加新障碍物**：修改代码添加更多碰撞体

---

## 📁 本章文件

```
examples/05_collision/
├── README.md           ← 本文档
└── main.cpp            ← 示例代码
```

---

## 🎯 下一章预告

**第六章：精灵动画系统** 将学习：
- 帧动画原理
- Animator动画控制器
- 动画状态切换
- 动画事件回调
