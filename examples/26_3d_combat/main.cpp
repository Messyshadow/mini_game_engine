/**
 * 第26章：3D战斗系统 (3D Combat System)
 *
 * 在3D场景中实现：
 * - AABB碰撞检测：角色不能穿过墙壁和箱子
 * - Hitbox/Hurtbox攻击判定：球球碰撞检测
 * - 骨骼动画攻击：按J播放Punching动画，攻击中不可移动，结束后自动回Idle
 * - Hit Stop顿帧效果：命中时全局时间缩放
 * - 敌人受击反馈：闪红+击退+无敌时间+血条显示
 * - 三段连招状态机：伤害递增
 *
 * 操作：
 * - WASD：移动角色（相对摄像机方向）
 * - Space：跳跃（支持二段跳）
 * - J：攻击（前方Hitbox检测）
 * - Shift：冲刺跑
 * - F：闪避冲刺（有冷却）
 * - 鼠标左键拖动：旋转摄像机视角
 * - 滚轮：调整跟随距离
 * - E：编辑面板 | R：重置
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

// 骨骼模型 & 动画
SkeletalModel g_skelModel;
std::vector<AnimClip> g_anims;  // 0=Idle, 1=Running, 2=Punching, 3=Death
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
std::vector<SceneObj> g_objects;

// 光照
struct { Vec3 dir = Vec3(0.4f,0.7f,0.3f); Vec3 color = Vec3(1,0.95f,0.9f); float intensity = 0.8f; bool on = true; } g_dirLight;
struct { Vec3 pos = Vec3(3,4,3); Vec3 color = Vec3(1,0.8f,0.6f); float intensity = 1.5f; float lin = 0.09f, quad = 0.032f; bool on = true; } g_ptLight;
float g_ambient = 0.15f;

// 战斗系统
HitStop g_hitStop;
std::vector<CombatEntity> g_enemies;
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

// 显示
bool g_showEditor = true;
Vec4 g_bgColor(0.15f, 0.18f, 0.25f, 1);
bool g_mouseLDown = false;
double g_lastMX = 0, g_lastMY = 0;

// 共享网格
Mesh3D g_sphereMesh;
Mesh3D g_cubeMesh;

// 角色碰撞半径
const float PLAYER_COLLISION_RADIUS = 0.5f;

// ============================================================================
// 计算场景物体的AABB
// ============================================================================
AABB3D ComputeAABB(const Vec3& pos, const Vec3& scl, float baseSize) {
    AABB3D aabb;
    float hx = baseSize * scl.x * 0.5f;
    float hy = baseSize * scl.y * 0.5f;
    float hz = baseSize * scl.z * 0.5f;
    aabb.min = Vec3(pos.x - hx, pos.y - hy, pos.z - hz);
    aabb.max = Vec3(pos.x + hx, pos.y + hy, pos.z + hz);
    return aabb;
}

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

    // ---- 场景物体（带AABB碰撞） ----

    // 地面(30x30) — 无碰撞（角色靠groundHeight检测）
    { SceneObj o; o.name = "Ground"; o.mesh = Mesh3D::CreatePlane(30, 30, 1, 1);
      o.matIdx = 0; o.tiling = 8; o.shininess = 8; o.hasCollision = false;
      g_objects.push_back(std::move(o)); }

    // 砖墙1 — 有碰撞
    { SceneObj o; o.name = "Wall 1"; o.mesh = Mesh3D::CreateCube(2);
      o.pos = Vec3(-5, 1, 3); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 2);
      g_objects.push_back(std::move(o)); }

    // 砖墙2 — 有碰撞
    { SceneObj o; o.name = "Wall 2"; o.mesh = Mesh3D::CreateCube(2);
      o.pos = Vec3(5, 1, -3); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 2);
      g_objects.push_back(std::move(o)); }

    // 砖墙3 — 有碰撞
    { SceneObj o; o.name = "Wall 3"; o.mesh = Mesh3D::CreateCube(2);
      o.pos = Vec3(0, 1, -8); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 2);
      g_objects.push_back(std::move(o)); }

    // 金属柱 — 有碰撞（scl=1,4,1）
    { SceneObj o; o.name = "Pillar"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(7, 2, 7); o.scl = Vec3(1, 4, 1); o.matIdx = 3; o.shininess = 64;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }

    // 木箱1 — 有碰撞
    { SceneObj o; o.name = "Crate 1"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(-7, 0.5f, -5); o.matIdx = 4; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 1);
      g_objects.push_back(std::move(o)); }

    // 木箱2 — 有碰撞
    { SceneObj o; o.name = "Crate 2"; o.mesh = Mesh3D::CreateCube(0.8f);
      o.pos = Vec3(3, 0.4f, 6); o.matIdx = 4; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 0.8f);
      g_objects.push_back(std::move(o)); }

    // 金属球 — 装饰，无碰撞
    { SceneObj o; o.name = "Metal Ball"; o.mesh = Mesh3D::CreateSphere(0.8f, 24, 16);
      o.pos = Vec3(-3, 0.8f, 5); o.matIdx = 2; o.shininess = 128; o.hasCollision = false;
      g_objects.push_back(std::move(o)); }

    // 角色控制器
    g_player.position = Vec3(0, 0.9f, 0);
    g_player.params.groundHeight = 0;

    // 摄像机
    g_cam.mode = CameraMode::TPS;
    g_cam.tpsDistance = 8; g_cam.tpsHeight = 3;
    g_cam.pitch = 20; g_cam.yaw = -30;

    // 玩家战斗数据
    g_playerCombat.name = "Player";
    g_playerCombat.hp = 100; g_playerCombat.maxHp = 100;
    g_playerCombat.alive = true;

    // 3个敌人
    g_enemies.clear();
    { CombatEntity e; e.name = "Enemy A"; e.position = Vec3(4, 0.9f, 4);
      e.hp = 100; e.maxHp = 100; e.hurtbox = {e.position + Vec3(0,0.9f,0), 0.8f};
      g_enemies.push_back(e); }
    { CombatEntity e; e.name = "Enemy B"; e.position = Vec3(-4, 0.9f, -4);
      e.hp = 80; e.maxHp = 80; e.hurtbox = {e.position + Vec3(0,0.9f,0), 0.8f};
      g_enemies.push_back(e); }
    { CombatEntity e; e.name = "Enemy C"; e.position = Vec3(6, 0.9f, -2);
      e.hp = 120; e.maxHp = 120; e.hurtbox = {e.position + Vec3(0,0.9f,0), 0.8f};
      g_enemies.push_back(e); }

    // 共享网格
    g_sphereMesh = Mesh3D::CreateSphere(1.0f, 12, 8);
    g_cubeMesh = Mesh3D::CreateCube(1.0f);

    // 骨骼矩阵初始化
    for (int i = 0; i < MAX_BONES; i++) g_boneMatrices[i] = Mat4::Identity();

    std::cout << "\n=== Chapter 26: 3D Combat System ===" << std::endl;
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

    AttackData atk;
    float dmgMult = 1.0f;
    switch (g_comboState) {
        case ComboState::Attack1: dmgMult = 1.0f; break;
        case ComboState::Attack2: dmgMult = 1.2f; break;
        case ComboState::Attack3: dmgMult = 1.8f; break;
        default: break;
    }
    atk.damage = g_attackDamage * dmgMult;
    atk.knockback = g_attackKnockback * dmgMult;
    atk.hitStopDuration = g_hitStopDur;
    atk.hitbox = {hitCenter, g_hitboxRadius};

    g_attackHit = false;
    for (auto& enemy : g_enemies) {
        if (!enemy.alive || enemy.invincibleTimer > 0) continue;
        enemy.hurtbox.center = enemy.position + Vec3(0, 0.9f, 0);

        if (SphereSphereCollision(atk.hitbox, enemy.hurtbox)) {
            enemy.hp -= atk.damage;
            enemy.hitTimer = 0.3f;
            enemy.invincibleTimer = g_invincibleDur;

            // 击退方向
            Vec3 knockDir(enemy.position.x - g_player.position.x, 0,
                          enemy.position.z - g_player.position.z);
            float kl = std::sqrt(knockDir.x * knockDir.x + knockDir.z * knockDir.z);
            if (kl > 0.001f) { knockDir.x /= kl; knockDir.z /= kl; }
            else { knockDir = fwd; }
            enemy.position = enemy.position + knockDir * atk.knockback;

            if (enemy.hp <= 0) { enemy.hp = 0; enemy.alive = false; }

            g_hitStop.Trigger(atk.hitStopDuration);
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
            g_playerCombat.hp = g_playerCombat.maxHp;
            g_playerCombat.alive = true;
            g_comboState = ComboState::None;
            g_attackTimer = 0; g_comboWindow = 0;
            g_attackAnimDone = false;
            Vec3 epos[] = {Vec3(4,0.9f,4), Vec3(-4,0.9f,-4), Vec3(6,0.9f,-2)};
            float ehp[] = {100, 80, 120};
            for (int i = 0; i < (int)g_enemies.size(); i++) {
                g_enemies[i].position = epos[i];
                g_enemies[i].hp = ehp[i]; g_enemies[i].maxHp = ehp[i];
                g_enemies[i].alive = true;
                g_enemies[i].hitTimer = 0; g_enemies[i].invincibleTimer = 0;
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
                if (g_anims.size() > 2) SwitchAnim(2); // Punching
            } else if (g_comboWindow > 0) {
                if (g_comboState == ComboState::Attack1) g_comboState = ComboState::Attack2;
                else if (g_comboState == ComboState::Attack2) g_comboState = ComboState::Attack3;
                g_attackTimer = ATTACK_DURATION;
                g_comboWindow = 0;
                g_attackAnimDone = false;
                PerformAttack();
                g_animTime = 0; // 重新播放Punching
            }
        }
        jL = j;
    }

    // 攻击计时器 — 攻击动画播放完毕检测
    bool isAttacking = (g_comboState != ComboState::None);
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
            // 连招窗口超时，回到Idle
            g_comboState = ComboState::None;
            g_comboWindow = 0;
            g_attackAnimDone = false;
            if (g_anims.size() > 0) SwitchAnim(0); // 回到Idle
        }
    }

    // ---- 移动（攻击动画播放中不可移动） ----
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

    // ---- AABB碰撞检测：角色不能穿过场景物体 ----
    for (auto& o : g_objects) {
        if (!o.hasCollision) continue;
        ResolveCollision(g_player.position, PLAYER_COLLISION_RADIUS, o.aabb);
    }

    // 动画自动切换（非攻击状态下）
    if (g_comboState == ComboState::None) {
        if (isMoving && g_currentAnim != 1 && g_anims.size() > 1) SwitchAnim(1); // Running
        if (!isMoving && g_currentAnim != 0 && g_anims.size() > 0) SwitchAnim(0); // Idle
    }

    // 敌人计时器
    for (auto& e : g_enemies) {
        if (e.hitTimer > 0) e.hitTimer -= dt;
        if (e.invincibleTimer > 0) e.invincibleTimer -= dt;
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

    // ---- 敌人渲染（球体+头部，受击闪红） ----
    for (auto& e : g_enemies) {
        if (!e.alive) continue;

        // 受击闪红：hitTimer>0时uBaseColor变红
        bool isHit = (e.hitTimer > 0);
        Vec4 bodyColor = isHit ? Vec4(1.0f, 0.15f, 0.15f, 1) : Vec4(0.8f, 0.2f, 0.2f, 1);
        Vec4 headColor = isHit ? Vec4(1.0f, 0.3f, 0.3f, 1)  : Vec4(0.9f, 0.3f, 0.3f, 1);

        Mat3 identNm;
        identNm.m[0] = identNm.m[4] = identNm.m[8] = 1;
        identNm.m[1] = identNm.m[2] = identNm.m[3] = identNm.m[5] = identNm.m[6] = identNm.m[7] = 0;

        g_matShader->SetBool("uUseDiffuseMap", false);
        g_matShader->SetBool("uUseNormalMap", false);
        g_matShader->SetBool("uUseRoughnessMap", false);
        g_matShader->SetBool("uUseVertexColor", false);
        g_matShader->SetFloat("uShininess", 32);
        g_matShader->SetFloat("uTexTiling", 1);

        // 身体（球体）
        Mat4 bodyMat = Mat4::Translate(e.position.x, e.position.y + 0.5f, e.position.z)
                     * Mat4::Scale(0.8f, 0.8f, 0.8f);
        g_matShader->SetMat4("uModel", bodyMat.m);
        g_matShader->SetMat3("uNormalMatrix", identNm.m);
        g_matShader->SetVec4("uBaseColor", bodyColor);
        g_sphereMesh.Draw();

        // 头部（小球）
        Mat4 headMat = Mat4::Translate(e.position.x, e.position.y + 1.4f, e.position.z)
                     * Mat4::Scale(0.4f, 0.4f, 0.4f);
        g_matShader->SetMat4("uModel", headMat.m);
        g_matShader->SetVec4("uBaseColor", headColor);
        g_sphereMesh.Draw();
    }

    // ---- 攻击hitbox可视化（攻击中半透明球） ----
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

    // ---- 蒙皮着色器：玩家角色（X Bot骨骼动画） ----
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
            std::string name = "uBones[" + std::to_string(i) + "]";
            g_skinShader->SetMat4(name, g_boneMatrices[i].m);
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
    // ---- 敌人头顶血条（世界坐标→屏幕坐标） ----
    {
        int ww, wh; glfwGetFramebufferSize(Engine::Instance().GetWindow(), &ww, &wh);
        float aspect = (ww > 0 && wh > 0) ? (float)ww / (float)wh : 1;
        Mat4 view = g_cam.GetViewMatrix();
        Mat4 proj = g_cam.GetProjectionMatrix(aspect);
        Mat4 vp = proj * view;

        for (auto& e : g_enemies) {
            if (!e.alive) continue;
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

            // 血条背景
            ImGui::GetForegroundDrawList()->AddRectFilled(
                ImVec2(sx - barW/2, sy), ImVec2(sx + barW/2, sy + barH),
                IM_COL32(40, 40, 40, 200));
            // 血条（受击闪白）
            ImU32 barCol = (e.hitTimer > 0) ? IM_COL32(255, 100, 100, 255) : IM_COL32(220, 50, 50, 230);
            ImGui::GetForegroundDrawList()->AddRectFilled(
                ImVec2(sx - barW/2, sy), ImVec2(sx - barW/2 + barW * hpRatio, sy + barH),
                barCol);
            // 血条边框
            ImGui::GetForegroundDrawList()->AddRect(
                ImVec2(sx - barW/2, sy), ImVec2(sx + barW/2, sy + barH),
                IM_COL32(200, 200, 200, 180));
            // 名字和血量
            char hpText[64];
            snprintf(hpText, sizeof(hpText), "%s %.0f/%.0f", e.name.c_str(), e.hp, e.maxHp);
            ImGui::GetForegroundDrawList()->AddText(
                ImVec2(sx - 40, sy - 16), IM_COL32(255, 255, 255, 220), hpText);
        }
    }

    // ---- HUD ----
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##HUD", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Ch26: 3D Combat System");
    ImGui::Separator();
    ImGui::Text("Pos: (%.1f, %.1f, %.1f)", g_player.position.x, g_player.position.y, g_player.position.z);

    if (!g_anims.empty())
        ImGui::Text("Anim: %s", g_anims[g_currentAnim].name.c_str());

    int aliveCount = 0;
    for (auto& e : g_enemies) if (e.alive) aliveCount++;
    ImGui::Text("Enemies: %d / %d", aliveCount, (int)g_enemies.size());

    const char* comboNames[] = {"None", "Attack 1", "Attack 2", "Attack 3"};
    ImGui::Text("Combo: %s", comboNames[(int)g_comboState]);
    if (g_hitStop.timer > 0) ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "HIT STOP!");

    ImGui::Text("Ground: %s  Sprint: %s  Dash: %s",
        g_player.state.grounded ? "Y" : "N",
        g_player.state.sprinting ? "Y" : "N",
        g_player.state.dashing ? "Y" : "N");
    ImGui::Text("FPS: %.0f", Engine::Instance().GetFPS());
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "WASD:Move J:Attack Space:Jump");
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "Shift:Sprint F:Dash E:Editor R:Reset");
    ImGui::End();

    // ---- 编辑面板 ----
    if (!g_showEditor) return;

    ImGui::Begin("Combat Editor [E]", &g_showEditor, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::BeginTabBar("Ch26Tabs")) {

        if (ImGui::BeginTabItem("Camera")) {
            ImGui::SliderFloat("Yaw", &g_cam.yaw, -180, 180);
            ImGui::SliderFloat("Pitch", &g_cam.pitch, -20, 89);
            ImGui::SliderFloat("Distance", &g_cam.tpsDistance, 2, 20);
            ImGui::SliderFloat("FOV", &g_cam.fov, 10, 120);
            ImGui::SliderFloat("Height", &g_cam.tpsHeight, 0, 10);
            ImGui::SliderFloat("Mouse Sens", &g_cam.mouseSensitivity, 0.01f, 0.5f);
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
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Player")) {
            ImGui::Text("HP: %.0f / %.0f", g_playerCombat.hp, g_playerCombat.maxHp);
            ImGui::SliderFloat("Player HP", &g_playerCombat.hp, 0, g_playerCombat.maxHp);
            ImGui::DragFloat3("Position", &g_player.position.x, 0.1f);
            ImGui::Separator();
            ImGui::SliderFloat("Move Speed", &g_player.params.moveSpeed, 1, 20);
            ImGui::SliderFloat("Sprint Speed", &g_player.params.sprintSpeed, 5, 30);
            ImGui::SliderFloat("Jump Force", &g_player.params.jumpForce, 3, 20);
            ImGui::SliderFloat("Dash Speed", &g_player.params.dashSpeed, 10, 40);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Enemies")) {
            for (int i = 0; i < (int)g_enemies.size(); i++) {
                auto& e = g_enemies[i];
                ImGui::PushID(i);
                if (ImGui::TreeNode("##e", "[%d] %s %s", i, e.name.c_str(), e.alive ? "" : "(DEAD)")) {
                    ImGui::Text("HP: %.0f / %.0f", e.hp, e.maxHp);
                    ImGui::SliderFloat("HP", &e.hp, 0, e.maxHp);
                    ImGui::DragFloat3("Pos", &e.position.x, 0.1f);
                    ImGui::Text("Alive: %s  Hit: %.2f  Invincible: %.2f",
                        e.alive ? "Yes" : "No", e.hitTimer, e.invincibleTimer);
                    if (!e.alive && ImGui::Button("Revive")) {
                        e.alive = true; e.hp = e.maxHp;
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
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
    g_skelModel.Destroy();
    for (auto& o : g_objects) o.mesh.Destroy();
    for (auto& m : g_materials) m.Destroy();
    g_sphereMesh.Destroy();
    g_cubeMesh.Destroy();
    g_skinShader.reset();
    g_matShader.reset();
}

int main() {
    std::cout << "=== Example 26: 3D Combat System ===" << std::endl;
    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth = 1280; cfg.windowHeight = 720;
    cfg.windowTitle = "Mini Engine - 3D Combat System";
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
