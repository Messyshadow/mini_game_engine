# 第28章：场景与关卡

## 学习目标

1. 理解天空盒渲染原理（为什么去掉平移、为什么z=w）
2. 掌握Billboard数学（粒子始终面向摄像机）
3. 实现3D粒子系统（发射器、生命周期、颜色插值）
4. 构建场景管理器（多场景切换、触发器系统）
5. 实现淡入淡出场景过渡效果

## 核心系统

### A. 程序化天空盒

天空盒是一个包围整个场景的大立方体，6个面渲染天空颜色。

**关键技术点：**

1. **去掉View矩阵平移**：`mat4 viewNoTranslation = mat4(mat3(uView))`
   - 摄像机只旋转不移动，天空盒始终"在无穷远处"
   - 如果不去掉平移，走动时天空会跟着移动

2. **z=w 技巧**：`gl_Position = pos.xyww`
   - 令z=w → NDC的z=w/w=1.0 → 始终在最远处
   - 配合 `glDepthFunc(GL_LEQUAL)` 确保天空盒不遮挡任何物体

3. **程序化渐变**：根据方向向量的y分量计算颜色
   ```
   t = dir.y * 0.5 + 0.5    // 映射到0~1
   color = mix(底色, 顶色, t)  // 线性插值
   + 地平线光晕效果
   ```

**三种预设：**
- 日光：蓝天白云（top=深蓝, bottom=浅灰蓝）
- 黄昏：橙紫渐变（top=深紫, bottom=橙红）
- 夜晚：深蓝黑（top=深蓝黑, bottom=暗蓝）

### B. 3D粒子系统

#### Billboard数学

粒子必须始终面向摄像机，从View矩阵提取摄像机坐标轴：

```
camRight = vec3(View[0][0], View[1][0], View[2][0])  // 第1列
camUp    = vec3(View[0][1], View[1][1], View[2][1])  // 第2列
```

用这两个向量构造面向摄像机的四边形：
```
worldPos = particlePos + camRight * localX * size + camUp * localY * size
```

#### 粒子生命周期

每个粒子有独立生命值：
```
life -= dt
t = 1 - life / maxLife    // 0→1 从出生到死亡
color = lerp(startColor, endColor, t)
size = baseSize * (1 - t * 0.5)  // 逐渐缩小
```

#### 三种粒子效果

| 效果 | 颜色 | 速度 | 生命 | 触发时机 |
|------|------|------|------|----------|
| 攻击火花 | 橙→红→透明 | 8m/s | 0.4s | 命中敌人时爆发15个 |
| 冲刺拖尾 | 蓝→深蓝→透明 | 2m/s | 0.5s | 冲刺中每帧3个 |
| 环境尘埃 | 米黄半透明 | 0.5m/s | 4s | 持续发射5个/秒 |

#### 渲染顺序

粒子半透明，必须最后渲染：
1. 开启混合 `GL_SRC_ALPHA, GL_ONE`（加法混合，让火花更亮）
2. 关闭深度写入 `glDepthMask(GL_FALSE)`
3. 逐粒子设置uniform并绘制quad
4. 恢复深度写入

### C. 场景管理与触发器

#### 场景结构

```
SceneConfig {
    name            // 场景名称
    playerSpawn     // 玩家出生点
    skyTop/Bottom/Horizon  // 天空颜色
    sunDir/Color/Intensity // 光照
    ambient         // 环境光
}
```

每个场景独立定义：地面、墙壁、箱子、敌人、触发器。

#### 触发器检测

```
Trigger3D {
    position    // 中心位置
    halfSize    // AABB半尺寸
    action      // "goto_arena" / "goto_training" / "show_text"
    oneShot     // 是否只触发一次
}

Contains(point):
    |point.x - pos.x| < halfSize.x &&
    |point.y - pos.y| < halfSize.y &&
    |point.z - pos.z| < halfSize.z
```

#### 场景过渡（淡入淡出）

```
FadeTransition:
    前半段: alpha 0→1 (画面变黑)
    中间: 切换场景数据
    后半段: alpha 1→0 (画面恢复)
```

用ImGui的ForegroundDrawList绘制全屏黑色矩形，alpha随时间变化。

## 场景设计

### 场景1：训练场（日光）
- 30x30木地板 + 砖墙 + 木箱 + 金属柱
- 2个巡逻敌人
- 右上角传送触发器 → 竞技场
- 中央文字提示触发器

### 场景2：竞技场（黄昏）
- 25x25金属地板 + 8根圆形排列柱子
- 中央升高平台
- 3个敌人（更强）
- 左下角传送触发器 → 训练场

## 渲染顺序

```
1. 天空盒（关闭深度写入，GL_LEQUAL）
2. 场景物体（材质着色器）
3. 敌人（球体+头部，状态染色）
4. 触发器可视化（半透明方块）
5. 攻击hitbox（半透明球）
6. 玩家X Bot（蒙皮着色器）
7. 粒子（加法混合，关闭深度写入）
8. ImGui（HUD、血条、触发器提示、过渡黑幕）
```

## 操作

| 按键 | 功能 |
|------|------|
| WASD | 移动 |
| Space | 跳跃 |
| J | 攻击（三段连招） |
| Shift | 冲刺 |
| F | 闪避 |
| 1/2 | 手动切换场景 |
| E | 编辑面板 |
| R | 重置当前场景 |
| 鼠标左键拖动 | 旋转摄像机 |
| 滚轮 | 调整距离 |

## 编辑面板（E键）

8个标签页：
- **Camera** — 摄像机参数
- **Scene** — 场景切换按钮、触发器列表
- **Particles** — 粒子参数（生命/大小/速度/颜色/扩散）
- **Skybox** — 天空颜色 + 日光/黄昏/夜晚预设
- **Lighting** — 光照参数 + 日光/黄昏/夜晚预设
- **AI** — 敌人属性
- **Combat** — 攻击参数
- **Objects** — 场景物体列表

## 新增文件

| 文件 | 说明 |
|------|------|
| source/engine3d/Skybox.h | 程序化天空盒（立方体+渐变shader） |
| source/engine3d/ParticleSystem3D.h | 3D粒子发射器（Billboard渲染） |
| source/engine3d/SceneManager.h | 场景配置+触发器+淡入淡出过渡 |
| data/shader/skybox.vs | 天空盒顶点着色器（去平移+z=w） |
| data/shader/skybox.fs | 程序化渐变天空 |
| data/shader/particle3d.vs | 粒子Billboard顶点着色器 |
| data/shader/particle3d.fs | 粒子软圆形片段着色器 |

## 思考题

1. **天空盒为什么要去掉View矩阵的平移部分？** 如果不去掉会出现什么现象？
2. **为什么粒子渲染使用加法混合(GL_ONE)而不是标准Alpha混合？** 什么场景适合用加法混合？
3. **Billboard的缺点是什么？** 如果粒子是细长的火焰/拖尾，纯Billboard会有什么问题？速度对齐Billboard(Velocity-aligned Billboard)如何解决？
4. **触发器用AABB检测有什么局限？** 如果需要圆形/不规则区域的触发器，应该怎么实现？
5. **场景切换时如何避免内存泄漏？** 当前实现在ClearScene中Destroy了所有Mesh，但如果场景之间有共享资源（如相同材质），更好的资源管理策略是什么？

## 下一章预告

第29章：UI与音频整合
- FMOD音频引擎集成（3D空间音效、音源衰减）
- 3D伤害数字飘字
- 完整HUD系统（HP/MP/技能冷却）
- 小地图
