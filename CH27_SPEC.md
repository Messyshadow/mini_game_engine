# 第27章开发规范：敌人AI + PhysX物理集成

## 必读：先读 PROJECT_CONTEXT.md、LOCAL_ASSETS.md、ROADMAP_CH26_30.md

## 第27章两大核心

### A. PhysX物理引擎集成
### B. 敌人AI系统（行为树）

---

## A. PhysX集成

### 下载和安装

1. 下载PhysX：https://github.com/ThisisGame/PhysX/releases/download/v6.4/physx-4.1.7z
2. 解压到 depends/physx/ 目录
3. 解压后目录结构应为：
```
depends/physx/
├── physx/
│   ├── include/          ← 头文件
│   └── bin/
│       └── win.x86_64.vc142.mt/
│           └── debug/    ← lib和dll文件
└── pxshared/
    └── include/          ← 共享头文件
```

### CMake配置（参考 reference/physx_example/CMakeLists.txt.Physx）

```cmake
# 头文件
include_directories("depends/physx/physx/include")
include_directories("depends/physx/pxshared/include")

# 链接（只给27章target）
if(MSVC)
    target_link_libraries(27_enemy_ai
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysX_64.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXFoundation_64.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXExtensions_static_64.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXPvdSDK_static_64.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXCommon_64.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/physx/physx/bin/win.x86_64.vc142.mt/debug/PhysXCooking_64.lib")
    # DLL拷贝
    file(GLOB PHYSX_DLLS "depends/physx/physx/bin/win.x86_64.vc142.mt/debug/*.dll")
    file(COPY ${PHYSX_DLLS} DESTINATION "${CMAKE_BINARY_DIR}/Debug")
    file(COPY ${PHYSX_DLLS} DESTINATION "${CMAKE_BINARY_DIR}/Release")
endif()
```

### PhysX初始化代码（参考 reference/physx_example/main.cpp）

```cpp
#include "PxPhysicsAPI.h"
using namespace physx;

// 全局变量
PxDefaultAllocator gAllocator;
PxDefaultErrorCallback gErrorCallback;
PxFoundation* gFoundation = nullptr;
PxPhysics* gPhysics = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;
PxScene* gScene = nullptr;

void InitPhysX() {
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true);
    
    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0, -9.81f, 0);
    gDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    gScene = gPhysics->createScene(sceneDesc);
}

// 每帧调用
void StepPhysics(float dt) {
    gScene->simulate(dt);
    gScene->fetchResults(true);
}

// 创建地面
void CreateGround() {
    PxMaterial* mat = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
    PxRigidStatic* ground = PxCreatePlane(*gPhysics, PxPlane(0,1,0,0), *mat);
    gScene->addActor(*ground);
}

// 创建刚体箱子
PxRigidDynamic* CreateBox(PxVec3 pos, PxVec3 halfExtents) {
    PxMaterial* mat = gPhysics->createMaterial(0.5f, 0.5f, 0.3f);
    PxRigidDynamic* body = gPhysics->createRigidDynamic(PxTransform(pos));
    PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtents), *mat);
    body->attachShape(*shape);
    shape->release();
    PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
    gScene->addActor(*body);
    return body;
}

// 创建静态碰撞体（墙壁、场景物体）
PxRigidStatic* CreateStaticBox(PxVec3 pos, PxVec3 halfExtents) {
    PxMaterial* mat = gPhysics->createMaterial(0.5f, 0.5f, 0.3f);
    PxRigidStatic* body = gPhysics->createRigidStatic(PxTransform(pos));
    PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtents), *mat);
    body->attachShape(*shape);
    shape->release();
    gScene->addActor(*body);
    return body;
}

// 射线检测（地面检测用）
bool Raycast(PxVec3 origin, PxVec3 dir, float maxDist, PxRaycastBuffer& hit) {
    return gScene->raycast(origin, dir, maxDist, hit);
}

// 清理
void CleanupPhysX() {
    if(gScene) { gScene->release(); gScene = nullptr; }
    if(gDispatcher) { gDispatcher->release(); gDispatcher = nullptr; }
    if(gPhysics) { gPhysics->release(); gPhysics = nullptr; }
    if(gFoundation) { gFoundation->release(); gFoundation = nullptr; }
}
```

### PhysX封装类

创建 `source/engine3d/PhysicsWorld.h`，封装上述代码为引擎模块：

```cpp
class PhysicsWorld {
public:
    void Init();
    void Shutdown();
    void Step(float dt);
    
    // 创建碰撞体
    void AddStaticBox(Vec3 pos, Vec3 halfSize);     // 静态障碍物
    void AddDynamicBox(Vec3 pos, Vec3 halfSize);     // 动态刚体
    void AddGroundPlane();                            // 地面
    
    // 查询
    bool Raycast(Vec3 origin, Vec3 dir, float maxDist, Vec3& hitPoint, Vec3& hitNormal);
    
    // 角色控制器（用Kinematic刚体）
    void CreateCharacterBody(Vec3 pos, float radius, float height);
    void MoveCharacter(Vec3 newPos);
    Vec3 GetCharacterPosition();
};
```

---

## B. 敌人AI系统

### 创建 `source/engine3d/EnemyAI3D.h`

```cpp
enum class AIState { Idle, Patrol, Chase, Attack, Retreat, Dead };

struct EnemyAI {
    AIState state = AIState::Patrol;
    Vec3 position;
    float hp = 100, maxHp = 100;
    float detectionRange = 10.0f;   // 发现玩家距离
    float attackRange = 2.0f;       // 攻击距离
    float moveSpeed = 3.0f;
    float attackCooldown = 1.5f;
    float attackTimer = 0;
    
    // 巡逻
    Vec3 patrolPointA, patrolPointB;
    bool patrolToB = true;
    
    // 视野检测
    float fovAngle = 120.0f;  // 视野角度（度）
    Vec3 forward = Vec3(0,0,1);
    
    void Update(float dt, Vec3 playerPos);
    bool CanSeePlayer(Vec3 playerPos);
    bool InAttackRange(Vec3 playerPos);
};
```

### AI行为逻辑

```
Patrol: 在A/B点之间往返走动
  ↓ 发现玩家（距离<detectionRange 且 在视野内）
Chase: 朝玩家移动
  ↓ 进入攻击范围
Attack: 攻击玩家（播放攻击动画，有冷却）
  ↓ 玩家跑远了
Chase: 继续追
  ↓ 玩家跑出追击范围(detectionRange*1.5)
Patrol: 返回巡逻
  ↓ HP<=0
Dead: 播放死亡动画，消失
```

---

## 第27章需要创建的文件

1. `source/engine3d/PhysicsWorld.h` — PhysX封装
2. `source/engine3d/EnemyAI3D.h` — 敌人AI
3. `examples/27_enemy_ai/main.cpp` — 演示程序
4. `examples/27_enemy_ai/README.md` — 详细文档
5. 更新 CMakeLists.txt
6. 更新 README.md

## 场景设计

- 大地面(30x30) + 砖墙 + 木箱 + 金属柱（全部注册为PhysX静态碰撞体）
- 玩家（X Bot，PhysX Kinematic刚体，用CharacterController3D移动）
- 3个敌人（用简单几何体+血条，有AI巡逻/追击/攻击）
- 敌人被打死后消失
- 角色不能穿过任何场景物体（PhysX保证）

## 按键

和第26章一样：WASD移动、Space跳跃、J攻击、F冲刺、E编辑面板

## ImGui编辑面板

- Camera: 摄像机参数
- Physics: PhysX参数（重力、步进时间、碰撞体可视化开关）
- AI: 每个敌人的状态/HP/检测范围/攻击范围/速度
- Combat: 攻击参数
- Lighting: 光照

## README要求

包含：PhysX初始化流程、刚体类型（Static/Dynamic/Kinematic）解释、射线检测原理、行为树vs状态机对比、AI状态转换图、5道思考题
