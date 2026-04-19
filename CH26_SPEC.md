# 第26章开发规范：3D战斗系统

## 必读：先读 PROJECT_CONTEXT.md 了解项目全部规范

## 第26章需要创建的文件

1. `source/engine3d/CombatSystem3D.h` — 3D战斗系统
2. `examples/26_3d_combat/main.cpp` — 演示程序
3. `examples/26_3d_combat/README.md` — 详细文档
4. 更新 `CMakeLists.txt` — 添加26章target
5. 更新 `README.md`（项目根目录）— 添加26章到章节表

## CombatSystem3D.h 需要包含

```cpp
namespace MiniEngine {

// 碰撞体
struct SphereCollider {
    Vec3 center;
    float radius;
};

struct CapsuleCollider {
    Vec3 top, bottom;  // 胶囊两端点
    float radius;
};

// 碰撞检测函数
bool SphereSphereCollision(const SphereCollider& a, const SphereCollider& b);
bool SphereCapsuleCollision(const SphereCollider& sphere, const CapsuleCollider& capsule);
float PointToSegmentDistance(const Vec3& point, const Vec3& segA, const Vec3& segB);

// 攻击数据
struct AttackData {
    float damage = 10;
    float knockback = 5;
    float hitStopDuration = 0.1f;  // 顿帧时间
    SphereCollider hitbox;
};

// 可被攻击的实体
struct CombatEntity {
    Vec3 position;
    float hp = 100, maxHp = 100;
    SphereCollider hurtbox;
    bool alive = true;
    float hitTimer = 0;        // 受击闪烁
    float invincibleTimer = 0; // 无敌时间
    std::string name;
};

// Hit Stop效果（全局时间缩放）
struct HitStop {
    float timer = 0;
    float duration = 0;
    float timeScale = 0.05f;  // 顿帧时的时间缩放
    
    void Trigger(float dur) { timer = dur; duration = dur; }
    float GetTimeScale() const { return timer > 0 ? timeScale : 1.0f; }
    void Update(float realDt) { if(timer > 0) timer -= realDt; }
};

}
```

## main.cpp 场景设计

- 地面(30x30木地板) + 砖墙障碍 + 场景物体
- 玩家角色(X Bot, 用CharacterController3D移动)
- 3个敌人(用球体/立方体+血条表示)
- 玩家按J攻击 → 在角色前方生成Hitbox球体
- 攻击命中敌人 → Hit Stop顿帧 + 敌人闪烁 + 血量减少
- 敌人死亡后消失
- HUD显示：玩家位置、当前动画、敌人数量

## 按键映射（严格遵守）

| 按键 | 功能 |
|------|------|
| WASD | 移动角色（D=-1, A=1, W按24章的方向） |
| Space | 跳跃 |
| J | 攻击（前方Hitbox检测） |
| Shift | 冲刺跑 |
| F | 闪避冲刺 |
| 鼠标左键拖动 | 旋转摄像机 |
| 滚轮 | 调整距离 |
| E | 编辑面板 |
| R | 重置 |

## ImGui编辑面板标签页

- **Camera**: Yaw/Pitch/Distance/FOV
- **Combat**: 攻击伤害/击退/Hitbox半径/Hit Stop时长/无敌时间
- **Player**: HP/位置/速度
- **Enemies**: 每个敌人的HP/位置/状态
- **Lighting**: 方向光+点光源

## 严格遵守的API调用（从PROJECT_CONTEXT.md）

```cpp
// Shader
g_shader->LoadFromFile(vsPath, fsPath);  // 不是Load()
g_shader->Bind();                         // 不是Use()
g_shader->SetMat4("uModel", model.m);    // 传.m
g_shader->SetMat3("uNormalMatrix", nm.m);

// Camera3D
g_cam.mode = CameraMode::TPS;
g_cam.orbitDistance = 12;    // 不是distance
g_cam.orbitTarget = ...;     // 不是target
g_cam.tpsTarget = ...;
g_cam.tpsDistance = 8;
g_cam.ProcessMouseMove(dx, dy);
g_cam.ProcessScroll(delta);
g_cam.UpdateTPS(dt);
g_cam.GetViewMatrix();
g_cam.GetProjectionMatrix(aspect);
g_cam.GetPosition();

// 注意yaw/pitch是度数，不是弧度
g_cam.yaw = -30.0f;  // 度
g_cam.pitch = 25.0f;  // 度

// CharacterController3D
g_player.ProcessMovement(inputFwd, inputRt, g_cam.yaw, sprint, dt);
g_player.ProcessJump(pressed);
g_player.ProcessDash(pressed);
g_player.Update(dt);
g_player.position;
g_player.state.currentYaw;

// 材质
Material3D mat;
mat.LoadFromDirectory("bricks", "Bricks");
mat.Bind();

// glad必须在GLFW前
#include <glad/gl.h>
#include <GLFW/glfw3.h>
```

## CMakeLists.txt添加方法

参考现有的24章：
```cmake
file(GLOB example_26_cpp examples/26_3d_combat/*.cpp)

add_executable(26_3d_combat
    ${glfw_sources} ${imgui_src} ${engine_cpp} ${engine_h} ${example_26_cpp})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib")
    target_link_libraries(26_3d_combat
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/zlibstatic.lib")
endif()

# MSVC配置（加到现有列表中）
set_property(TARGET 26_3d_combat PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT 26_3d_combat)
set_target_properties(26_3d_combat PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
```

## README.md要求

包含：学习目标、碰撞检测数学（球球/球胶囊/点到线段距离公式）、Hit Stop原理、连招状态机设计、Shader解析、操作说明、ImGui说明、5道思考题、下一章预告（第27章敌人AI+PhysX）。

## 注意事项

- 不要创建Camera3D里不存在的成员（如distance, target, Update, GetFlatFront等）
- 不要限制角色移动范围
- 骨骼动画已在第25章实现（source/engine3d/Animator.h），可以集成动画播放
- 如果用蒙皮渲染角色，shader用skinning.vs/fs（第25章创建的）
