/**
 * 第27章：敌人AI + PhysX物理集成
 *
 * - PhysX物理引擎：地面/墙壁/箱子注册为静态碰撞体，角色为Kinematic刚体
 * - 敌人AI状态机：Patrol巡逻 → Chase追击 → Attack攻击 → Dead死亡
 * - 3个敌人有独立巡逻路线，发现玩家后追击，进入攻击范围自动攻击
 * - 射线检测：角色贴地
 * - 所有CH26功能保留：骨骼动画、三段连招、Hit Stop、受击闪红血条
 *
 * 操作：
 * - WASD：移动 | Space：跳跃 | J：攻击 | Shift：冲刺 | F：闪避
 * - 鼠标左键拖动：旋转摄像机 | 滚轮：距离 | E：编辑面板 | R：重置
 */

#include "engine/Engine.h"
#include "engine/Shader.h"
#include "engine3d/Mesh3D.h"
#include "engine3d/ModelLoader.h"
#include "engine3d/Texture3D.h"
#include "engine3d/Camera3D.h"
#include "engine3d/CharacterController3D.h"
#include "engine3d/Animator.h"
#include "engine3d/CombatSystem3D.h"
#include "engine3d/PhysicsWorld.h"
#include "engine3d/EnemyAI3D.h"
#include "math/Math.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <cmath>
#include <vector>
#include <string>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 全局状态
// ============================================================================
std::unique_ptr<Shader> g_skinShader;
std::unique_ptr<Shader> g_matShader;
Camera3D g_cam;
CharacterController3D g_player;
PhysicsWorld g_physics;

// 骨骼模型 & 动画
SkeletalModel g_skelModel;
std::vector<AnimClip> g_anims; // 0=Idle, 1=Running, 2=Punching, 3=Death
int g_currentAnim = 0;
float g_animTime = 0;
float g_animSpeed = 1.0f;
Mat4 g_boneMatrices[MAX_BONES];

// 动画混合
float g_blendDuration = 0.3f;
float g_blendTimer = 0;
Mat4 g_prevBoneMatrices[MAX_BONES];
Mat4 g_nextBoneMatrices[MAX_BONES];

// 材质 & 场景
std::vector<Material3D> g_materials;
struct SceneObj {
    Mesh3D mesh;
    Vec3 pos, rot = Vec3(0,0,0), scl = Vec3(1,1,1);
    int matIdx = -1; float tiling = 1; float shininess = 32;
    bool visible = true; std::string name;
    bool hasCollision = false;
    AABB3D aabb;
};

const float PLAYER_COLLISION_RADIUS = 0.5f;

AABB3D ComputeAABB(const Vec3& pos, const Vec3& scl, float baseSize) {
    AABB3D aabb;
    float hx = baseSize * scl.x * 0.5f;
    float hy = baseSize * scl.y * 0.5f;
    float hz = baseSize * scl.z * 0.5f;
    aabb.min = Vec3(pos.x - hx, pos.y - hy, pos.z - hz);
    aabb.max = Vec3(pos.x + hx, pos.y + hy, pos.z + hz);
    return aabb;
}
std::vector<SceneObj> g_objects;

// 光照
struct { Vec3 dir = Vec3(0.4f,0.7f,0.3f); Vec3 color = Vec3(1,0.95f,0.9f); float intensity = 1.2f; bool on = true; } g_dirLight;
struct { Vec3 pos = Vec3(3,4,3); Vec3 color = Vec3(1,0.8f,0.6f); float intensity = 1.5f; float lin = 0.09f, quad = 0.032f; bool on = true; } g_ptLight;
float g_ambient = 0.25f;

// 战斗系统
HitStop g_hitStop;
CombatEntity g_playerCombat;

// 攻击参数
float g_attackDamage = 25.0f;
float g_attackKnockback = 5.0f;
float g_hitboxRadius = 1.5f;
float g_hitStopDur = 0.1f;
float g_invincibleDur = 0.5f;

// 攻击状态
enum class ComboState { None, Attack1, Attack2, Attack3 };
ComboState g_comboState = ComboState::None;
float g_attackTimer = 0;
float g_comboWindow = 0;
bool g_attackHit = false;
const float ATTACK_DURATION = 0.5f;
const float COMBO_WINDOW = 0.4f;
bool g_attackAnimDone = false;

// AI敌人
std::vector<EnemyAI3D> g_enemies;

// 显示
bool g_showEditor = true;
Vec4 g_bgColor(0.2f, 0.25f, 0.35f, 1);
bool g_mouseLDown = false;
double g_lastMX = 0, g_lastMY = 0;

// 共享网格
Mesh3D g_sphereMesh;
Mesh3D g_cubeMesh;

// PhysX调试
bool g_showPhysxDebug = false;

// ============================================================================
// 滚轮回调
// ============================================================================
void ScrollCB(GLFWwindow*, double, double dy) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    g_cam.ProcessScroll((float)dy);
}

// ============================================================================
// 光照设置
// ============================================================================
void SetLightUniforms(Shader* s) {
    s->SetFloat("uAmbientStrength", g_ambient);
    s->SetInt("uNumDirLights", g_dirLight.on ? 1 : 0);
    if (g_dirLight.on) {
        s->SetVec3("uDirLights[0].direction", g_dirLight.dir);
        s->SetVec3("uDirLights[0].color", g_dirLight.color);
        s->SetFloat("uDirLights[0].intensity", g_dirLight.intensity);
    }
    s->SetInt("uNumPointLights", g_ptLight.on ? 1 : 0);
    if (g_ptLight.on) {
        s->SetVec3("uPointLights[0].position", g_ptLight.pos);
        s->SetVec3("uPointLights[0].color", g_ptLight.color);
        s->SetFloat("uPointLights[0].intensity", g_ptLight.intensity);
        s->SetFloat("uPointLights[0].constant", 1);
        s->SetFloat("uPointLights[0].linear", g_ptLight.lin);
        s->SetFloat("uPointLights[0].quadratic", g_ptLight.quad);
    }
}

// ============================================================================
// 动画切换（带混合）
// ============================================================================
void SwitchAnim(int newIdx) {
    if (newIdx < 0 || newIdx >= (int)g_anims.size() || newIdx == g_currentAnim) return;
    for (int i = 0; i < MAX_BONES; i++) g_prevBoneMatrices[i] = g_boneMatrices[i];
    g_blendTimer = g_blendDuration;
    g_currentAnim = newIdx;
    g_animTime = 0;
}

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // PhysX初始化
    g_physics.Init();
    if (!g_physics.IsValid()) {
        std::cerr << "PhysX initialization failed!" << std::endl;
        return false;
    }
    g_physics.AddGroundPlane();

    // 蒙皮着色器
    g_skinShader = std::make_unique<Shader>();
    const char* svs[] = {"data/shader/skinning.vs","../data/shader/skinning.vs","../../data/shader/skinning.vs"};
    const char* sfs[] = {"data/shader/skinning.fs","../data/shader/skinning.fs","../../data/shader/skinning.fs"};
    bool ok = false;
    for (int i = 0; i < 3; i++) { if (g_skinShader->LoadFromFile(svs[i], sfs[i])) { ok = true; break; } }
    if (!ok) { std::cerr << "Failed to load skinning shader!" << std::endl; return false; }

    // 材质着色器
    g_matShader = std::make_unique<Shader>();
    const char* mvs[] = {"data/shader/material.vs","../data/shader/material.vs","../../data/shader/material.vs"};
    const char* mfs[] = {"data/shader/material.fs","../data/shader/material.fs","../../data/shader/material.fs"};
    ok = false;
    for (int i = 0; i < 3; i++) { if (g_matShader->LoadFromFile(mvs[i], mfs[i])) { ok = true; break; } }
    if (!ok) { std::cerr << "Failed to load material shader!" << std::endl; return false; }

    // 纹理单元
    g_skinShader->Bind();
    g_skinShader->SetInt("uDiffuseMap", 0); g_skinShader->SetInt("uNormalMap", 1); g_skinShader->SetInt("uRoughnessMap", 2);
    g_matShader->Bind();
    g_matShader->SetInt("uDiffuseMap", 0); g_matShader->SetInt("uNormalMap", 1); g_matShader->SetInt("uRoughnessMap", 2);

    // 骨骼模型
    if (!Animator::LoadModelFallback("fbx/X Bot.fbx", g_skelModel)) {
        std::cerr << "Failed to load X Bot model!" << std::endl;
        return false;
    }

    // 动画：0=Idle, 1=Running, 2=Punching, 3=Death
    const char* animFiles[] = {"fbx/Idle.fbx", "fbx/Running.fbx", "fbx/Punching.fbx", "fbx/Death.fbx"};
    const char* animNames[] = {"Idle", "Running", "Punching", "Death"};
    for (int i = 0; i < 4; i++) {
        AnimClip clip;
        if (Animator::LoadAnimFallback(animFiles[i], clip)) {
            clip.name = animNames[i];
            g_anims.push_back(std::move(clip));
        }
    }
    if (g_anims.empty()) { std::cerr << "No animations loaded!" << std::endl; return false; }

    // 材质
    { Material3D m; m.LoadFromDirectory("woodfloor", "Wood Floor"); m.tiling = 3; g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("bricks", "Bricks"); g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("metal", "Metal"); g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("metalplates", "Metal Plates"); g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("wood", "Wood"); g_materials.push_back(std::move(m)); }

    // ---- 场景物体 + PhysX静态碰撞体 ----

    // 地面(30x30)
    { SceneObj o; o.name = "Ground"; o.mesh = Mesh3D::CreatePlane(30, 30, 1, 1);
      o.matIdx = 0; o.tiling = 8; o.shininess = 8;
      g_objects.push_back(std::move(o)); }

    // 砖墙1
    { SceneObj o; o.name = "Wall 1"; o.mesh = Mesh3D::CreateCube(2);
      o.pos = Vec3(-5, 1, 3); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 2);
      g_objects.push_back(std::move(o));
      g_physics.AddStaticBox(Vec3(-5, 1, 3), Vec3(1, 1, 1)); }

    // 砖墙2
    { SceneObj o; o.name = "Wall 2"; o.mesh = Mesh3D::CreateCube(2);
      o.pos = Vec3(5, 1, -3); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 2);
      g_objects.push_back(std::move(o));
      g_physics.AddStaticBox(Vec3(5, 1, -3), Vec3(1, 1, 1)); }

    // 砖墙3
    { SceneObj o; o.name = "Wall 3"; o.mesh = Mesh3D::CreateCube(2);
      o.pos = Vec3(0, 1, -8); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 2);
      g_objects.push_back(std::move(o));
      g_physics.AddStaticBox(Vec3(0, 1, -8), Vec3(1, 1, 1)); }

    // 长墙（竞技场边界）
    { SceneObj o; o.name = "Arena Wall N"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(0, 1, 14); o.scl = Vec3(30, 2, 1); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o));
      g_physics.AddStaticBox(Vec3(0, 1, 14), Vec3(15, 1, 0.5f)); }

    { SceneObj o; o.name = "Arena Wall S"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(0, 1, -14); o.scl = Vec3(30, 2, 1); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o));
      g_physics.AddStaticBox(Vec3(0, 1, -14), Vec3(15, 1, 0.5f)); }

    { SceneObj o; o.name = "Arena Wall E"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(14, 1, 0); o.scl = Vec3(1, 2, 30); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o));
      g_physics.AddStaticBox(Vec3(14, 1, 0), Vec3(0.5f, 1, 15)); }

    { SceneObj o; o.name = "Arena Wall W"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(-14, 1, 0); o.scl = Vec3(1, 2, 30); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o));
      g_physics.AddStaticBox(Vec3(-14, 1, 0), Vec3(0.5f, 1, 15)); }

    // 金属柱
    { SceneObj o; o.name = "Pillar"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(7, 2, 7); o.scl = Vec3(1, 4, 1); o.matIdx = 3; o.shininess = 64;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o));
      g_physics.AddStaticBox(Vec3(7, 2, 7), Vec3(0.5f, 2, 0.5f)); }

    // 木箱1
    { SceneObj o; o.name = "Crate 1"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(-7, 0.5f, -5); o.matIdx = 4; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 1);
      g_objects.push_back(std::move(o));
      g_physics.AddStaticBox(Vec3(-7, 0.5f, -5), Vec3(0.5f, 0.5f, 0.5f)); }

    // 木箱2
    { SceneObj o; o.name = "Crate 2"; o.mesh = Mesh3D::CreateCube(0.8f);
      o.pos = Vec3(3, 0.4f, 6); o.matIdx = 4; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 0.8f);
      g_objects.push_back(std::move(o));
      g_physics.AddStaticBox(Vec3(3, 0.4f, 6), Vec3(0.4f, 0.4f, 0.4f)); }

    // 金属球（装饰）
    { SceneObj o; o.name = "Metal Ball"; o.mesh = Mesh3D::CreateSphere(0.8f, 24, 16);
      o.pos = Vec3(-3, 0.8f, 5); o.matIdx = 2; o.shininess = 128;
      g_objects.push_back(std::move(o)); }

    // 角色控制器 + PhysX Kinematic体
    g_player.position = Vec3(0, 0.9f, 0);
    g_player.params.groundHeight = 0;
    g_physics.CreateCharacterBody(Vec3(0, 0.9f, 0), 0.4f, 0.5f);

    // 摄像机
    g_cam.mode = CameraMode::TPS;
    g_cam.tpsDistance = 8; g_cam.tpsHeight = 3;
    g_cam.pitch = 20; g_cam.yaw = -30;

    // 玩家战斗数据
    g_playerCombat.name = "Player";
    g_playerCombat.hp = 100; g_playerCombat.maxHp = 100;
    g_playerCombat.alive = true;

    // 3个AI敌人（各有巡逻路线）
    g_enemies.clear();
    {
        EnemyAI3D e;
        e.name = "Enemy A"; e.position = Vec3(4, 0.9f, 4);
        e.hp = 100; e.maxHp = 100;
        e.patrolPointA = Vec3(2, 0.9f, 2); e.patrolPointB = Vec3(8, 0.9f, 8);
        e.detectionRange = 10; e.attackRange = 2.0f; e.moveSpeed = 3.0f;
        e.attackDamage = 10;
        g_enemies.push_back(e);
    }
    {
        EnemyAI3D e;
        e.name = "Enemy B"; e.position = Vec3(-4, 0.9f, -4);
        e.hp = 80; e.maxHp = 80;
        e.patrolPointA = Vec3(-8, 0.9f, -2); e.patrolPointB = Vec3(-2, 0.9f, -8);
        e.detectionRange = 8; e.attackRange = 2.5f; e.moveSpeed = 3.5f;
        e.attackDamage = 15;
        g_enemies.push_back(e);
    }
    {
        EnemyAI3D e;
        e.name = "Enemy C"; e.position = Vec3(6, 0.9f, -6);
        e.hp = 120; e.maxHp = 120;
        e.patrolPointA = Vec3(4, 0.9f, -4); e.patrolPointB = Vec3(10, 0.9f, -10);
        e.detectionRange = 12; e.attackRange = 1.8f; e.moveSpeed = 2.5f;
        e.attackDamage = 20;
        g_enemies.push_back(e);
    }

    // 共享网格
    g_sphereMesh = Mesh3D::CreateSphere(1.0f, 12, 8);
    g_cubeMesh = Mesh3D::CreateCube(1.0f);

    // 骨骼矩阵初始化
    for (int i = 0; i < MAX_BONES; i++) g_boneMatrices[i] = Mat4::Identity();

    std::cout << "\n=== Chapter 27: Enemy AI + PhysX ===" << std::endl;
    std::cout << "WASD: Move | Space: Jump | Shift: Sprint | F: Dash" << std::endl;
    std::cout << "J: Attack (3-hit combo) | E: Editor | R: Reset" << std::endl;
    std::cout << "LMB+Drag: Camera | Scroll: Distance" << std::endl;
    return true;
}

// ============================================================================
// 攻击处理
// ============================================================================
void PerformAttack() {
    Vec3 fwd = g_player.GetForward();
    Vec3 hitCenter = g_player.position + fwd * 1.5f + Vec3(0, 0.9f, 0);

    float dmgMult = 1.0f;
    switch (g_comboState) {
        case ComboState::Attack1: dmgMult = 1.0f; break;
        case ComboState::Attack2: dmgMult = 1.2f; break;
        case ComboState::Attack3: dmgMult = 1.8f; break;
        default: break;
    }

    SphereCollider hitbox{hitCenter, g_hitboxRadius};

    g_attackHit = false;
    for (auto& enemy : g_enemies) {
        if (enemy.state == AIState::Dead || enemy.invincibleTimer > 0) continue;

        SphereCollider hurtbox{enemy.position + Vec3(0, 0.9f, 0), enemy.hurtRadius};

        if (SphereSphereCollision(hitbox, hurtbox)) {
            float dmg = g_attackDamage * dmgMult;
            enemy.hp -= dmg;
            enemy.hitTimer = 0.3f;
            enemy.invincibleTimer = g_invincibleDur;

            Vec3 knockDir(enemy.position.x - g_player.position.x, 0,
                          enemy.position.z - g_player.position.z);
            float kl = std::sqrt(knockDir.x * knockDir.x + knockDir.z * knockDir.z);
            if (kl > 0.001f) { knockDir.x /= kl; knockDir.z /= kl; }
            else { knockDir = fwd; }
            enemy.position = enemy.position + knockDir * (g_attackKnockback * dmgMult);

            if (enemy.hp <= 0) { enemy.hp = 0; enemy.state = AIState::Dead; }

            g_hitStop.Trigger(g_hitStopDur);
            g_attackHit = true;
        }
    }
}

// ============================================================================
// 更新
// ============================================================================
void Update(float dt) {
    Engine& eng = Engine::Instance();
    GLFWwindow* w = eng.GetWindow();
    if (dt > 0.1f) dt = 0.1f;

    // Hit Stop时间缩放
    g_hitStop.Update(dt);
    float gameDt = dt * g_hitStop.GetTimeScale();

    // PhysX步进
    g_physics.Step(gameDt);

    // E编辑面板
    static bool eL = false;
    { bool d = glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS; if (d && !eL) g_showEditor = !g_showEditor; eL = d; }

    // R重置
    static bool rL = false;
    {
        bool d = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS;
        if (d && !rL) {
            g_player.Reset();
            g_player.position = Vec3(0, 0.9f, 0);
            g_physics.MoveCharacter(Vec3(0, 0.9f, 0));
            g_playerCombat.hp = g_playerCombat.maxHp;
            g_playerCombat.alive = true;
            g_comboState = ComboState::None;
            g_attackTimer = 0; g_comboWindow = 0;
            g_attackAnimDone = false;

            Vec3 epos[] = {Vec3(4,0.9f,4), Vec3(-4,0.9f,-4), Vec3(6,0.9f,-6)};
            float ehp[] = {100, 80, 120};
            Vec3 patA[] = {Vec3(2,0.9f,2), Vec3(-8,0.9f,-2), Vec3(4,0.9f,-4)};
            Vec3 patB[] = {Vec3(8,0.9f,8), Vec3(-2,0.9f,-8), Vec3(10,0.9f,-10)};
            for (int i = 0; i < (int)g_enemies.size(); i++) {
                g_enemies[i].position = epos[i];
                g_enemies[i].hp = ehp[i]; g_enemies[i].maxHp = ehp[i];
                g_enemies[i].state = AIState::Patrol;
                g_enemies[i].hitTimer = 0; g_enemies[i].invincibleTimer = 0;
                g_enemies[i].patrolPointA = patA[i]; g_enemies[i].patrolPointB = patB[i];
                g_enemies[i].patrolToB = true;
            }
            SwitchAnim(0);
        }
        rL = d;
    }

    // 鼠标左键旋转摄像机
    double mx, my; glfwGetCursorPos(w, &mx, &my);
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
        if (!g_mouseLDown) { g_mouseLDown = true; g_lastMX = mx; g_lastMY = my; }
        g_cam.ProcessMouseMove((float)(mx - g_lastMX), (float)(my - g_lastMY));
        g_lastMX = mx; g_lastMY = my;
    } else g_mouseLDown = false;

    // ---- J键攻击 ----
    static bool jL = false;
    {
        bool j = glfwGetKey(w, GLFW_KEY_J) == GLFW_PRESS;
        if (j && !jL) {
            if (g_comboState == ComboState::None) {
                g_comboState = ComboState::Attack1;
                g_attackTimer = ATTACK_DURATION;
                g_comboWindow = 0;
                g_attackAnimDone = false;
                PerformAttack();
                if (g_anims.size() > 2) SwitchAnim(2);
            } else if (g_comboWindow > 0) {
                if (g_comboState == ComboState::Attack1) g_comboState = ComboState::Attack2;
                else if (g_comboState == ComboState::Attack2) g_comboState = ComboState::Attack3;
                g_attackTimer = ATTACK_DURATION;
                g_comboWindow = 0;
                g_attackAnimDone = false;
                PerformAttack();
                g_animTime = 0;
            }
        }
        jL = j;
    }

    // 攻击计时器
    if (g_attackTimer > 0) {
        g_attackTimer -= gameDt;
        if (g_attackTimer <= 0) {
            g_attackAnimDone = true;
            g_comboWindow = COMBO_WINDOW;
        }
    }
    if (g_comboWindow > 0) {
        g_comboWindow -= gameDt;
        if (g_comboWindow <= 0) {
            g_comboState = ComboState::None;
            g_comboWindow = 0;
            g_attackAnimDone = false;
            if (g_anims.size() > 0) SwitchAnim(0);
        }
    }

    // ---- 移动（攻击中不可移动） ----
    float inputFwd = 0, inputRt = 0;
    bool isMoving = false;
    if (g_comboState == ComboState::None) {
        if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) inputFwd = 1;
        if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) inputFwd = -1;
        if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) inputRt = -1;
        if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) inputRt = 1;
        isMoving = (inputFwd != 0 || inputRt != 0);
    }
    bool sprint = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    g_player.ProcessMovement(inputFwd, inputRt, g_cam.yaw, sprint, gameDt);

    // 跳跃
    static bool spL = false;
    { bool sp = glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS; if (sp && !spL) g_player.ProcessJump(true); spL = sp; }

    // 冲刺
    static bool fL = false;
    { bool f = glfwGetKey(w, GLFW_KEY_F) == GLFW_PRESS; if (f && !fL) g_player.ProcessDash(true); fL = f; }

    g_player.Update(gameDt);

    // AABB碰撞检测：角色不能穿过场景物体
    for (auto& o : g_objects) {
        if (!o.hasCollision) continue;
        ResolveCollision(g_player.position, PLAYER_COLLISION_RADIUS, o.aabb);
    }

    // 同步角色位置到PhysX Kinematic体
    g_physics.MoveCharacter(g_player.position);

    // PhysX射线检测贴地（可选增强）
    Vec3 rayHit, rayNorm;
    if (g_physics.Raycast(
            Vec3(g_player.position.x, g_player.position.y + 1.0f, g_player.position.z),
            Vec3(0, -1, 0), 5.0f, rayHit, rayNorm)) {
        if (g_player.state.grounded && std::abs(g_player.position.y - rayHit.y) < 0.5f) {
            g_player.position.y = rayHit.y + g_player.params.height * 0.5f;
        }
    }

    // ---- AI敌人更新 ----
    for (auto& e : g_enemies) {
        e.Update(gameDt, g_player.position);

        // 敌人攻击玩家
        if (e.IsAttacking() && g_playerCombat.alive) {
            float dx = g_player.position.x - e.position.x;
            float dz = g_player.position.z - e.position.z;
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist < e.attackRange + 0.5f) {
                g_playerCombat.hp -= e.attackDamage * gameDt;
                if (g_playerCombat.hp <= 0) {
                    g_playerCombat.hp = 0;
                    g_playerCombat.alive = false;
                }
            }
        }
    }

    // 动画自动切换（非攻击状态下）
    if (g_comboState == ComboState::None) {
        if (isMoving && g_currentAnim != 1 && g_anims.size() > 1) SwitchAnim(1);
        if (!isMoving && g_currentAnim != 0 && g_anims.size() > 0) SwitchAnim(0);
    }

    // 摄像机跟随
    g_cam.tpsTarget = g_player.position;
    g_cam.orbitTarget = g_player.position + Vec3(0, 1, 0);
    g_cam.UpdateTPS(gameDt);

    // 动画更新
    if (!g_anims.empty()) {
        g_animTime += gameDt * g_animSpeed;
        Animator::ComputeBoneMatrices(g_skelModel, g_anims[g_currentAnim], g_animTime, g_boneMatrices);

        if (g_blendTimer > 0) {
            g_blendTimer -= gameDt;
            float blendT = 1.0f - std::max(0.0f, g_blendTimer / g_blendDuration);
            Animator::ComputeBoneMatrices(g_skelModel, g_anims[g_currentAnim], g_animTime, g_nextBoneMatrices);
            Animator::BlendBoneMatrices(g_prevBoneMatrices, g_nextBoneMatrices, blendT, g_boneMatrices);
            if (g_blendTimer <= 0) g_blendTimer = 0;
        }
    }
}

// ============================================================================
// 渲染场景物体
// ============================================================================
void RenderSceneObj(SceneObj& o) {
    if (!o.visible) return;
    Mat4 model = Mat4::Translate(o.pos.x, o.pos.y, o.pos.z)
               * Mat4::RotateY(o.rot.y) * Mat4::RotateX(o.rot.x) * Mat4::RotateZ(o.rot.z)
               * Mat4::Scale(o.scl.x, o.scl.y, o.scl.z);
    g_matShader->SetMat4("uModel", model.m);
    Mat3 nm = model.GetRotationMatrix().Inverse().Transposed();
    g_matShader->SetMat3("uNormalMatrix", nm.m);

    bool hasMat = (o.matIdx >= 0 && o.matIdx < (int)g_materials.size());
    if (hasMat) {
        auto& m = g_materials[o.matIdx]; m.Bind();
        g_matShader->SetBool("uUseDiffuseMap", m.hasDiffuse);
        g_matShader->SetBool("uUseNormalMap", m.hasNormal);
        g_matShader->SetBool("uUseRoughnessMap", m.hasRoughness);
        g_matShader->SetFloat("uTexTiling", o.tiling);
    } else {
        g_matShader->SetBool("uUseDiffuseMap", false);
        g_matShader->SetBool("uUseNormalMap", false);
        g_matShader->SetBool("uUseRoughnessMap", false);
        g_matShader->SetFloat("uTexTiling", 1);
    }
    g_matShader->SetBool("uUseVertexColor", false);
    g_matShader->SetVec4("uBaseColor", Vec4(0.7f, 0.7f, 0.7f, 1));
    g_matShader->SetFloat("uShininess", o.shininess);
    o.mesh.Draw();
}

// ============================================================================
// 渲染
// ============================================================================
void Render() {
    Engine& eng = Engine::Instance();
    int ww, wh; glfwGetFramebufferSize(eng.GetWindow(), &ww, &wh);
    float aspect = (ww > 0 && wh > 0) ? (float)ww / (float)wh : 1;

    glClearColor(g_bgColor.x, g_bgColor.y, g_bgColor.z, g_bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat4 view = g_cam.GetViewMatrix();
    Mat4 proj = g_cam.GetProjectionMatrix(aspect);

    // ---- 材质着色器：场景物体 ----
    g_matShader->Bind();
    g_matShader->SetMat4("uView", view.m);
    g_matShader->SetMat4("uProjection", proj.m);
    g_matShader->SetVec3("uViewPos", g_cam.GetPosition());
    SetLightUniforms(g_matShader.get());

    for (auto& o : g_objects) RenderSceneObj(o);

    // ---- 敌人渲染（球体+头部，受击闪红，死亡不渲染） ----
    for (auto& e : g_enemies) {
        if (e.state == AIState::Dead) continue;

        bool isHit = (e.hitTimer > 0);
        Vec4 bodyColor = isHit ? Vec4(1.0f, 0.15f, 0.15f, 1) : Vec4(0.8f, 0.2f, 0.2f, 1);
        Vec4 headColor = isHit ? Vec4(1.0f, 0.3f, 0.3f, 1)  : Vec4(0.9f, 0.3f, 0.3f, 1);

        // 追击/攻击状态染色
        if (e.state == AIState::Chase) {
            bodyColor = isHit ? Vec4(1,0.15f,0.15f,1) : Vec4(0.9f, 0.5f, 0.1f, 1);
        } else if (e.state == AIState::Attack) {
            bodyColor = isHit ? Vec4(1,0.15f,0.15f,1) : Vec4(1.0f, 0.2f, 0.5f, 1);
        }

        Mat3 identNm;
        identNm.m[0] = identNm.m[4] = identNm.m[8] = 1;
        identNm.m[1] = identNm.m[2] = identNm.m[3] = identNm.m[5] = identNm.m[6] = identNm.m[7] = 0;

        g_matShader->SetBool("uUseDiffuseMap", false);
        g_matShader->SetBool("uUseNormalMap", false);
        g_matShader->SetBool("uUseRoughnessMap", false);
        g_matShader->SetBool("uUseVertexColor", false);
        g_matShader->SetFloat("uShininess", 32);
        g_matShader->SetFloat("uTexTiling", 1);

        // 身体
        Mat4 bodyMat = Mat4::Translate(e.position.x, e.position.y + 0.5f, e.position.z)
                     * Mat4::Scale(0.8f, 0.8f, 0.8f);
        g_matShader->SetMat4("uModel", bodyMat.m);
        g_matShader->SetMat3("uNormalMatrix", identNm.m);
        g_matShader->SetVec4("uBaseColor", bodyColor);
        g_sphereMesh.Draw();

        // 头部
        Mat4 headMat = Mat4::Translate(e.position.x, e.position.y + 1.4f, e.position.z)
                     * Mat4::Scale(0.4f, 0.4f, 0.4f);
        g_matShader->SetMat4("uModel", headMat.m);
        g_matShader->SetVec4("uBaseColor", headColor);
        g_sphereMesh.Draw();

        // 朝向指示（小方块）
        float fx = e.forward.x, fz = e.forward.z;
        Mat4 dirMat = Mat4::Translate(
            e.position.x + fx * 0.9f, e.position.y + 0.9f, e.position.z + fz * 0.9f)
            * Mat4::Scale(0.15f, 0.15f, 0.15f);
        g_matShader->SetMat4("uModel", dirMat.m);
        g_matShader->SetVec4("uBaseColor", Vec4(1, 1, 0, 1));
        g_cubeMesh.Draw();
    }

    // ---- 攻击hitbox可视化 ----
    if (g_attackTimer > 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        Vec3 fwd = g_player.GetForward();
        Vec3 hitCenter = g_player.position + fwd * 1.5f + Vec3(0, 0.9f, 0);
        Mat4 hm = Mat4::Translate(hitCenter.x, hitCenter.y, hitCenter.z)
                 * Mat4::Scale(g_hitboxRadius, g_hitboxRadius, g_hitboxRadius);
        g_matShader->SetMat4("uModel", hm.m);
        g_matShader->SetBool("uUseDiffuseMap", false);
        g_matShader->SetBool("uUseNormalMap", false);
        g_matShader->SetBool("uUseRoughnessMap", false);
        g_matShader->SetBool("uUseVertexColor", false);
        g_matShader->SetVec4("uBaseColor", Vec4(1, 0.8f, 0.2f, 0.3f));
        g_matShader->SetFloat("uShininess", 8);
        g_matShader->SetFloat("uTexTiling", 1);
        g_sphereMesh.Draw();

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // ---- 蒙皮着色器：玩家X Bot ----
    if (g_skelModel.IsValid()) {
        g_skinShader->Bind();
        g_skinShader->SetMat4("uView", view.m);
        g_skinShader->SetMat4("uProjection", proj.m);
        g_skinShader->SetVec3("uViewPos", g_cam.GetPosition());
        SetLightUniforms(g_skinShader.get());

        Mat4 model = Mat4::Translate(g_player.position.x,
                                     g_player.position.y - g_player.params.height * 0.5f,
                                     g_player.position.z)
                   * Mat4::RotateY(g_player.state.currentYaw)
                   * Mat4::Scale(0.02f, 0.02f, 0.02f);
        g_skinShader->SetMat4("uModel", model.m);

        Mat3 nm;
        nm.m[0] = nm.m[4] = nm.m[8] = 1;
        nm.m[1] = nm.m[2] = nm.m[3] = nm.m[5] = nm.m[6] = nm.m[7] = 0;
        g_skinShader->SetMat3("uNormalMatrix", nm.m);

        g_skinShader->SetBool("uUseSkinning", true);
        for (int i = 0; i < MAX_BONES && i < (int)g_skelModel.bones.size(); i++) {
            std::string bname = "uBones[" + std::to_string(i) + "]";
            g_skinShader->SetMat4(bname, g_boneMatrices[i].m);
        }

        g_skinShader->SetBool("uUseDiffuseMap", false);
        g_skinShader->SetBool("uUseNormalMap", false);
        g_skinShader->SetBool("uUseRoughnessMap", false);
        g_skinShader->SetBool("uUseVertexColor", true);
        g_skinShader->SetFloat("uShininess", 32);
        g_skinShader->SetFloat("uTexTiling", 1);
        g_skinShader->SetVec4("uBaseColor", Vec4(0.5f, 0.5f, 0.6f, 1));

        g_skelModel.Draw();
    }
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui() {
    // ---- 敌人头顶血条+状态 ----
    {
        int ww, wh; glfwGetFramebufferSize(Engine::Instance().GetWindow(), &ww, &wh);
        float aspect = (ww > 0 && wh > 0) ? (float)ww / (float)wh : 1;
        Mat4 view = g_cam.GetViewMatrix();
        Mat4 proj = g_cam.GetProjectionMatrix(aspect);
        Mat4 vp = proj * view;

        for (auto& e : g_enemies) {
            if (e.state == AIState::Dead) continue;
            Vec3 wp(e.position.x, e.position.y + 2.2f, e.position.z);
            Vec4 clip(
                vp.m[0]*wp.x + vp.m[4]*wp.y + vp.m[8]*wp.z  + vp.m[12],
                vp.m[1]*wp.x + vp.m[5]*wp.y + vp.m[9]*wp.z  + vp.m[13],
                vp.m[2]*wp.x + vp.m[6]*wp.y + vp.m[10]*wp.z + vp.m[14],
                vp.m[3]*wp.x + vp.m[7]*wp.y + vp.m[11]*wp.z + vp.m[15]
            );
            if (clip.w < 0.01f) continue;
            float ndcX = clip.x / clip.w, ndcY = clip.y / clip.w;
            float sx = (ndcX * 0.5f + 0.5f) * ww;
            float sy = (1.0f - (ndcY * 0.5f + 0.5f)) * wh;

            float barW = 60, barH = 8;
            float hpRatio = e.hp / e.maxHp;

            ImGui::GetForegroundDrawList()->AddRectFilled(
                ImVec2(sx - barW/2, sy), ImVec2(sx + barW/2, sy + barH),
                IM_COL32(40, 40, 40, 200));
            ImU32 barCol = (e.hitTimer > 0) ? IM_COL32(255, 100, 100, 255) : IM_COL32(220, 50, 50, 230);
            ImGui::GetForegroundDrawList()->AddRectFilled(
                ImVec2(sx - barW/2, sy), ImVec2(sx - barW/2 + barW * hpRatio, sy + barH),
                barCol);
            ImGui::GetForegroundDrawList()->AddRect(
                ImVec2(sx - barW/2, sy), ImVec2(sx + barW/2, sy + barH),
                IM_COL32(200, 200, 200, 180));

            char hpText[64];
            snprintf(hpText, sizeof(hpText), "%s [%s] %.0f/%.0f",
                     e.name.c_str(), e.StateName(), e.hp, e.maxHp);
            ImGui::GetForegroundDrawList()->AddText(
                ImVec2(sx - 50, sy - 16), IM_COL32(255, 255, 255, 220), hpText);
        }
    }

    // ---- 玩家血条 ----
    {
        float barW = 200, barH = 20;
        float x = 10, y2 = 0;
        int ww, wh; glfwGetFramebufferSize(Engine::Instance().GetWindow(), &ww, &wh);
        y2 = (float)wh - 40;
        float hpRatio = g_playerCombat.hp / g_playerCombat.maxHp;
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(x, y2), ImVec2(x + barW, y2 + barH), IM_COL32(40, 40, 40, 200));
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(x, y2), ImVec2(x + barW * hpRatio, y2 + barH),
            IM_COL32(50, 200, 50, 230));
        ImGui::GetForegroundDrawList()->AddRect(
            ImVec2(x, y2), ImVec2(x + barW, y2 + barH), IM_COL32(200, 200, 200, 180));
        char hpText[64];
        snprintf(hpText, sizeof(hpText), "Player HP: %.0f/%.0f", g_playerCombat.hp, g_playerCombat.maxHp);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(x + 5, y2 + 3), IM_COL32(255, 255, 255, 220), hpText);
    }

    // ---- HUD ----
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##HUD", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Ch27: Enemy AI + PhysX");
    ImGui::Separator();
    ImGui::Text("Pos: (%.1f, %.1f, %.1f)", g_player.position.x, g_player.position.y, g_player.position.z);

    if (!g_anims.empty())
        ImGui::Text("Anim: %s", g_anims[g_currentAnim].name.c_str());

    int aliveCount = 0;
    for (auto& e : g_enemies) if (e.state != AIState::Dead) aliveCount++;
    ImGui::Text("Enemies: %d / %d", aliveCount, (int)g_enemies.size());

    const char* comboNames[] = {"None", "Attack 1", "Attack 2", "Attack 3"};
    ImGui::Text("Combo: %s", comboNames[(int)g_comboState]);
    if (g_hitStop.timer > 0) ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "HIT STOP!");

    ImGui::Text("PhysX: %s", g_physics.IsValid() ? "OK" : "OFF");
    ImGui::Text("FPS: %.0f", Engine::Instance().GetFPS());
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "WASD:Move J:Attack Space:Jump");
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "Shift:Sprint F:Dash E:Editor R:Reset");
    ImGui::End();

    // ---- 编辑面板 ----
    if (!g_showEditor) return;

    ImGui::Begin("AI + Physics Editor [E]", &g_showEditor, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::BeginTabBar("Ch27Tabs")) {

        if (ImGui::BeginTabItem("Camera")) {
            ImGui::SliderFloat("Yaw", &g_cam.yaw, -180, 180);
            ImGui::SliderFloat("Pitch", &g_cam.pitch, -20, 89);
            ImGui::SliderFloat("Distance", &g_cam.tpsDistance, 2, 20);
            ImGui::SliderFloat("FOV", &g_cam.fov, 10, 120);
            ImGui::SliderFloat("Height", &g_cam.tpsHeight, 0, 10);
            ImGui::SliderFloat("Mouse Sens", &g_cam.mouseSensitivity, 0.01f, 0.5f);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Physics")) {
            ImGui::Text("PhysX Status: %s", g_physics.IsValid() ? "Active" : "Inactive");
            ImGui::SliderFloat("Step Size", &g_physics.m_stepSize, 1.0f/120, 1.0f/30, "%.4f");
            ImGui::Separator();
            ImGui::Text("Static bodies: walls + crates + arena border");
            ImGui::Text("Kinematic body: player character");
            ImGui::Checkbox("Show Debug", &g_showPhysxDebug);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("AI")) {
            for (int i = 0; i < (int)g_enemies.size(); i++) {
                auto& e = g_enemies[i];
                ImGui::PushID(i);
                if (ImGui::TreeNode("##e", "[%d] %s [%s]", i, e.name.c_str(), e.StateName())) {
                    ImGui::Text("HP: %.0f / %.0f", e.hp, e.maxHp);
                    ImGui::SliderFloat("HP", &e.hp, 0, e.maxHp);
                    ImGui::DragFloat3("Pos", &e.position.x, 0.1f);
                    ImGui::SliderFloat("Detection", &e.detectionRange, 1, 25);
                    ImGui::SliderFloat("Attack Range", &e.attackRange, 0.5f, 5);
                    ImGui::SliderFloat("Speed", &e.moveSpeed, 0.5f, 10);
                    ImGui::SliderFloat("FOV", &e.fovAngle, 30, 360);
                    ImGui::SliderFloat("Atk Cooldown", &e.attackCooldown, 0.3f, 5);
                    ImGui::SliderFloat("Atk Damage", &e.attackDamage, 1, 50);
                    ImGui::Text("Forward: (%.2f, %.2f)", e.forward.x, e.forward.z);
                    ImGui::Text("Dist to player: %.1f", e.DistanceTo(g_player.position));
                    if (e.state == AIState::Dead && ImGui::Button("Revive")) {
                        e.state = AIState::Patrol;
                        e.hp = e.maxHp;
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Combat")) {
            ImGui::SliderFloat("Attack Damage", &g_attackDamage, 1, 100);
            ImGui::SliderFloat("Knockback", &g_attackKnockback, 0, 20);
            ImGui::SliderFloat("Hitbox Radius", &g_hitboxRadius, 0.5f, 5);
            ImGui::SliderFloat("Hit Stop Duration", &g_hitStopDur, 0.01f, 0.5f);
            ImGui::SliderFloat("Invincible Time", &g_invincibleDur, 0.1f, 2);
            ImGui::Separator();
            ImGui::Text("Hit Stop TimeScale: %.2f", g_hitStop.GetTimeScale());
            ImGui::SliderFloat("TimeScale", &g_hitStop.timeScale, 0.01f, 0.5f);
            ImGui::Separator();
            ImGui::Text("Player HP: %.0f / %.0f", g_playerCombat.hp, g_playerCombat.maxHp);
            ImGui::SliderFloat("Player HP", &g_playerCombat.hp, 0, g_playerCombat.maxHp);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lighting")) {
            ImGui::SliderFloat("Ambient", &g_ambient, 0, 0.5f);
            ImGui::Separator();
            ImGui::Checkbox("Dir Light", &g_dirLight.on);
            ImGui::DragFloat3("Dir", &g_dirLight.dir.x, 0.01f, -1, 1);
            ImGui::ColorEdit3("Dir Color", &g_dirLight.color.x);
            ImGui::SliderFloat("Dir Intensity", &g_dirLight.intensity, 0, 3);
            ImGui::Separator();
            ImGui::Checkbox("Point Light", &g_ptLight.on);
            ImGui::DragFloat3("Pt Pos", &g_ptLight.pos.x, 0.1f);
            ImGui::ColorEdit3("Pt Color", &g_ptLight.color.x);
            ImGui::SliderFloat("Pt Intensity", &g_ptLight.intensity, 0, 5);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

// ============================================================================
// 清理 & main
// ============================================================================
void Cleanup() {
    g_physics.Shutdown();
    g_skelModel.Destroy();
    for (auto& o : g_objects) o.mesh.Destroy();
    for (auto& m : g_materials) m.Destroy();
    g_sphereMesh.Destroy();
    g_cubeMesh.Destroy();
    g_skinShader.reset();
    g_matShader.reset();
}

int main() {
    std::cout << "=== Example 27: Enemy AI + PhysX ===" << std::endl;
    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth = 1280; cfg.windowHeight = 720;
    cfg.windowTitle = "Mini Engine - Enemy AI + PhysX";
    if (!engine.Initialize(cfg)) return -1;
    glfwSetScrollCallback(engine.GetWindow(), ScrollCB);
    if (!Initialize()) { engine.Shutdown(); return -1; }
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    Cleanup();
    engine.Shutdown();
    return 0;
}
