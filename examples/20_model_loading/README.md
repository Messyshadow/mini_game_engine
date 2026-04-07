# 第20章：3D模型加载

## 概述

集成Assimp库，实现OBJ和FBX格式的3D模型加载。从手写顶点数据升级到加载真正的3D美术资产。

---

## 学习目标

1. 理解3D模型文件格式（OBJ/FBX/GLTF）的区别
2. 掌握Assimp库的集成和使用
3. 理解模型加载管线（文件→Assimp解析→引擎格式→GPU）
4. 学会处理多Mesh模型和场景节点树

---

## 数学原理

### 模型空间与世界空间

每个模型文件里的顶点坐标都是相对于模型自身原点的（模型空间）。不同建模软件导出的坐标系可能不同：

| 软件 | 上方向 | 前方向 |
|------|--------|--------|
| 3ds Max | Z-up | Y-forward |
| Maya | Y-up | Z-forward |
| Blender | Z-up | -Y-forward |
| OpenGL | Y-up | -Z-forward |

Assimp会自动帮我们转换坐标系，这就是为什么要用 `aiProcess_FlipUVs` 等标志。

### 模型缩放

不同来源的模型尺寸差异巨大：
- Mixamo角色：约100个单位高
- 手写立方体：1个单位
- 兰博基尼：可能几百个单位

所以加载后需要根据实际情况调整 Scale。第20章通过ImGui的Scale滑块实时调。

---

## Assimp集成

### Assimp是什么

Assimp（Open Asset Import Library）是一个开源的模型加载库，支持40+种3D文件格式。

### 加载流程

```
1. 创建 Assimp::Importer
2. 调用 importer.ReadFile(path, flags)
3. 获得 aiScene* 场景对象
4. 递归遍历 aiScene->mRootNode
5. 对每个 aiMesh 提取顶点/法线/UV/索引
6. 转换为引擎的 Vertex3D + indices
7. 创建 Mesh3D 上传到GPU
```

### 后处理标志（Post-Processing Flags）

```cpp
unsigned int flags = aiProcess_Triangulate      // 将多边形面转为三角形
                   | aiProcess_GenNormals       // 自动生成法线（如果没有）
                   | aiProcess_FlipUVs          // 翻转UV的Y轴
                   | aiProcess_CalcTangentSpace; // 计算切线（法线贴图用）
```

### 场景节点树

```
aiScene
├── mRootNode
│   ├── child: "Body"
│   │   └── mMeshes: [0]     ← 引用第0个mesh
│   ├── child: "Head"
│   │   └── mMeshes: [1]     ← 引用第1个mesh
│   └── child: "Weapon"
│       └── mMeshes: [2, 3]  ← 引用第2、3个mesh
├── mMeshes[0]: 身体网格数据
├── mMeshes[1]: 头部网格数据
├── mMeshes[2]: 武器网格A
└── mMeshes[3]: 武器网格B
```

每个节点可以引用多个Mesh，也可以有子节点。我们用递归遍历整棵树。

---

## Shader解析

本章复用第19章的basic3d着色器。模型顶点的法线来自Assimp解析而非手写，光照效果更真实。

对于没有顶点颜色的模型（大多数模型都没有），我们用法线方向生成伪颜色来可视化法线：

```cpp
// 法线方向映射到颜色（-1~1 → 0~1）
v.color = Vec3(
    abs(normal.x) * 0.5 + 0.5,  // X法线 → 红色分量
    abs(normal.y) * 0.5 + 0.5,  // Y法线 → 绿色分量
    abs(normal.z) * 0.5 + 0.5   // Z法线 → 蓝色分量
);
```

---

## 文件格式对比

| 格式 | 后缀 | 骨骼动画 | 材质 | 文件大小 | 推荐场景 |
|------|------|----------|------|----------|----------|
| OBJ | .obj | ❌ | .mtl文件 | 较大（文本） | 静态模型 |
| FBX | .fbx | ✅ | 内嵌 | 中等（二进制） | 角色+动画 |
| GLTF | .gltf/.glb | ✅ | 内嵌 | 小（JSON+二进制） | Web/现代引擎 |
| DAE | .dae | ✅ | 内嵌 | 大（XML） | 旧项目兼容 |

**结论**：静态模型用OBJ就够了，带动画的角色用FBX。

---

## 操作说明

| 按键 | 功能 |
|------|------|
| 鼠标右键+拖动 | 旋转摄像机 |
| 滚轮 | 缩放 |
| E | 编辑面板 |
| L | 切换光照 |
| W | 线框模式 |

## ImGui编辑面板

| 标签页 | 内容 |
|--------|------|
| Camera | Yaw/Pitch/Distance/FOV/Target |
| Objects | 每个物体的位置/旋转/缩放/颜色/自动旋转、模型信息（Mesh数/顶点数/三角面数） |
| Lighting | 方向/颜色/环境光 + 预设 |
| Render | 背景色、线框、OpenGL信息、场景总统计 |

---

## 新增文件

| 文件 | 说明 |
|------|------|
| `source/engine3d/ModelLoader.h` | Assimp模型加载器（支持OBJ/FBX/GLTF） |
| `depends/assimp/` | Assimp库（头文件+lib+dll） |
| `data/models/fbx/` | FBX模型文件（Mixamo角色+动画） |
| `data/models/obj/` | OBJ模型文件（StopSign等） |

---

## 编译配置

CMakeLists.txt需要新增Assimp的配置：

```cmake
# Assimp头文件
include_directories("depends/assimp/include")

# 链接Assimp库
target_link_libraries(20_model_loading 
    ${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/zlibstatic.lib)

# 拷贝DLL到输出目录
file(COPY depends/assimp/bin/assimp-vc142-mt.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
```

---

## 思考题

1. 为什么FBX模型加载后需要缩放到0.02？这个缩放值怎么确定？
2. 如果一个模型有10万个三角形，渲染时会比1000个三角形慢多少？瓶颈在哪里？
3. aiProcess_Triangulate为什么是必须的？如果不加会怎样？
4. 模型的法线从Assimp读取和用aiProcess_GenNormals自动生成，效果有什么区别？
5. 为什么递归遍历场景节点树，而不是直接遍历aiScene->mMeshes数组？

---

## 下一章预告

第21章：光照系统 — Phong着色模型（环境光+漫反射+镜面反射），方向光/点光源/聚光灯。
