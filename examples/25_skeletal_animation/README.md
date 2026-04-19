# 第25章：骨骼动画

## 概述

加载Mixamo角色模型和动画，实现骨骼树遍历、关键帧插值、GPU顶点蒙皮。支持4个动画（Idle/Run/Punch/Death）的切换和过渡混合。

---

## 学习目标

1. 理解骨骼树（Skeleton Hierarchy）的数据结构
2. 掌握逆绑定矩阵（Offset Matrix）的含义
3. 实现动画关键帧线性插值（位置LERP、旋转NLERP、缩放LERP）
4. 理解顶点蒙皮公式和GPU蒙皮流程
5. 实现动画切换和线性混合过渡
6. 编写蒙皮顶点着色器（骨骼矩阵数组 + 权重混合）

---

## 数学原理

### 1. 骨骼树（Skeleton Hierarchy）

骨骼是一棵树结构。每根骨骼有一个父骨骼（根骨骼除外），子骨骼的变换相对于父骨骼。

```
Hips (根)
├── Spine
│   ├── Spine1
│   │   └── Spine2
│   │       ├── LeftShoulder → LeftArm → LeftForeArm → LeftHand
│   │       ├── RightShoulder → RightArm → RightForeArm → RightHand
│   │       └── Neck → Head
│   └── ...
├── LeftUpLeg → LeftLeg → LeftFoot → LeftToeBase
└── RightUpLeg → RightLeg → RightFoot → RightToeBase
```

**为什么用树结构？**

当你旋转上臂（LeftArm），前臂（LeftForeArm）和手（LeftHand）应该跟着旋转。树结构让子骨骼自动继承父骨骼的变换。

### 2. 骨骼空间与绑定姿态

模型制作时，角色处于**绑定姿态**（Bind Pose，通常是T-Pose）。每根骨骼在这个姿态下有一个确定的世界空间位置和朝向。

**逆绑定矩阵（Offset Matrix）**：把顶点从模型空间变换到骨骼的局部空间。

```
为什么需要逆绑定矩阵？

1. 顶点坐标存储在模型空间中
2. 骨骼动画在骨骼局部空间中操作
3. 需要先把顶点"搬到"骨骼空间，做完变换后再搬回来

流程：模型空间 → [Offset] → 骨骼空间 → [动画变换] → 世界空间
```

### 3. 全局变换的递归计算

每根骨骼的全局变换 = 从根到自身所有局部变换的连乘：

```
GlobalTransform(bone) = GlobalTransform(parent) × LocalTransform(bone)

根骨骼的GlobalTransform = Identity × LocalTransform(root)
```

在代码中就是沿树从根到叶递归：

```cpp
void ComputeNodeTransform(node, parentTransform, output) {
    Mat4 localTransform = InterpolateFromKeyframes(node, time);
    Mat4 globalTransform = parentTransform * localTransform;
    
    if (node is bone)
        output[boneIndex] = globalInverse * globalTransform * offsetMatrix;
    
    for (child : node.children)
        ComputeNodeTransform(child, globalTransform, output);
}
```

### 4. 顶点蒙皮公式

每个顶点最多受4根骨骼影响。最终位置是加权平均：

```
skinnedPosition = Σ(i=0..3) weight_i × FinalBone_i × vec4(position, 1.0)

其中 FinalBone_i = GlobalInverse × GlobalTransform_i × OffsetMatrix_i
```

**直观理解**：

```
weight_0=0.7 → 上臂骨骼拉这个顶点（拉力70%）
weight_1=0.3 → 前臂骨骼也拉这个顶点（拉力30%）
weight_2=0.0 → 不受影响
weight_3=0.0 → 不受影响

最终位置 = 0.7 × (上臂变换后的位置) + 0.3 × (前臂变换后的位置)
```

### 5. 关键帧插值

动画数据是离散的关键帧。两帧之间需要插值：

**位置和缩放：线性插值（LERP）**

```
result = a + (b - a) × t
t = (currentTime - frameA.time) / (frameB.time - frameA.time)
```

**旋转：归一化线性插值（NLERP）**

```
q_result = normalize( q_a × (1-t) + q_b × t )
```

NLERP是SLERP的简化版，速度不均匀但实际效果差异极小，而且计算量更低。

### 6. 动画混合

切换动画时直接跳变会很突兀。线性混合让两个动画平滑过渡：

```
blendedMatrices[i] = prevMatrices[i] × (1-t) + nextMatrices[i] × t
t 从0渐变到1（默认0.3秒）
```

---

## Shader解析

### skinning.vs（蒙皮顶点着色器）

基于material.vs扩展，新增骨骼变换。与material.vs的区别用`★`标记：

```glsl
// ★ 新增输入：骨骼ID和权重
layout (location = 4) in ivec4 aBoneIDs;     // 4个骨骼索引（整数）
layout (location = 5) in vec4  aBoneWeights;  // 4个骨骼权重（浮点）

// ★ 新增Uniform
uniform mat4 uBones[100];   // 骨骼最终变换矩阵
uniform bool uUseSkinning;  // 蒙皮开关

void main() {
    if (uUseSkinning) {
        // ★ 蒙皮核心：加权混合4个骨骼矩阵
        mat4 boneTransform = mat4(0.0);
        for (int i = 0; i < 4; i++) {
            if (aBoneIDs[i] >= 0)
                boneTransform += aBoneWeights[i] * uBones[aBoneIDs[i]];
        }
        localPos    = boneTransform * vec4(aPos, 1.0);
        localNormal = mat3(boneTransform) * aNormal;
    } else {
        localPos    = vec4(aPos, 1.0);
        localNormal = aNormal;
    }
    // 后续与material.vs相同：世界变换、TBN、投影
}
```

**关键点**：`aBoneIDs`使用`ivec4`（整数向量），必须用`glVertexAttribIPointer`而不是`glVertexAttribPointer`上传，否则GPU会把整数当浮点解释，导致骨骼索引错误。

### skinning.fs（蒙皮片段着色器）

与material.fs完全相同。蒙皮只影响顶点位置和法线，不影响光照和着色逻辑。

---

## 新增源码

### engine3d/Animator.h + Animator.cpp

| 类/结构体 | 说明 |
|-----------|------|
| `SkinnedVertex` | 扩展Vertex3D，增加boneIDs[4]和boneWeights[4] |
| `BoneInfo` | 骨骼索引 + 逆绑定矩阵 |
| `AnimChannel` | 一根骨骼的所有关键帧（位置/旋转/缩放） |
| `AnimClip` | 一个动画片段（持续时间 + 所有通道） |
| `BoneNode` | 骨骼树节点 |
| `SkinnedMesh` | 带骨骼权重的GPU网格 |
| `SkeletalModel` | 骨骼模型（网格 + 骨骼树 + 骨骼信息） |
| `Animator` | 静态工具类：加载模型/动画、计算骨骼矩阵、混合 |

---

## 操作说明

| 按键 | 功能 |
|------|------|
| 1/2/3/4 | 切换动画：Idle / Run / Punch / Death |
| Space | 播放/暂停 |
| +/- | 加速/减速（0.1x ~ 5.0x） |
| WASD | 移动角色（自动切换Idle/Run） |
| 鼠标左键拖动 | 旋转摄像机 |
| 滚轮 | 缩放 |
| B | 显示骨骼可视化 |
| E | 编辑面板 |
| R | 重置位置和时间 |

## ImGui编辑面板

| 标签页 | 内容 |
|--------|------|
| Camera | Yaw/Pitch/Distance/FOV |
| Animation | 动画选择列表、速度、播放控制、时间轴、混合设置 |
| Objects | 模型信息、角色位置/朝向/缩放、显示选项 |
| Lighting | 方向光/点光源参数、环境光 |

---

## 思考题

1. 为什么逆绑定矩阵（Offset Matrix）是在模型导出时就确定的，而不是运行时计算的？如果角色有两套不同的绑定姿态，Offset Matrix会不同吗？

2. 为什么`aBoneIDs`要用`ivec4`而不是`vec4`？如果错误地用`glVertexAttribPointer`代替`glVertexAttribIPointer`上传整数属性，会出什么bug？

3. NLERP（归一化线性插值）在什么情况下和SLERP（球面线性插值）差距最大？为什么大多数游戏引擎仍然选择NLERP？

4. 每个顶点最多受4根骨骼影响（MAX_BONE_INFLUENCE=4），如果某个顶点实际受6根骨骼影响，Assimp的`aiProcess_LimitBoneWeights`标志会怎么处理？

5. 动画混合时直接对矩阵做线性插值`mat = a*(1-t) + b*t`，这在数学上是否严格正确？为什么实际效果还不错？什么情况下会出问题？

---

## 下一章预告

第26章：3D战斗系统 — 碰撞体（Hitbox/Hurtbox）、近战攻击、连招、Hit Stop、受击反馈。
