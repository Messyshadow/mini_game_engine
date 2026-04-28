/**
 * 第28章：场景与关卡
 *
 * - 程序化渐变天空盒（日光/黄昏/夜晚预设）
 * - 3D粒子系统：攻击火花、冲刺拖尾、环境粒子
 * - 场景管理器：训练场 + 竞技场两个场景可切换
 * - 触发器系统：走到区域自动切换场景
 * - 音效：攻击命中、冲刺、场景切换（miniaudio）
 * - 所有CH27功能保留：骨骼动画、AI敌人、三段连招、Hit Stop、AABB碰撞
 *
 * 操作：
 * - WASD：移动 | Space：跳跃 | J：攻击 | Shift：冲刺 | F：闪避
 * - 鼠标左键拖动：旋转摄像机 | 滚轮：距离 | E：编辑面板 | R：重置
 * - 1/2：手动切换场景
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
#include "engine3d/EnemyAI3D.h"
#include "engine3d/Skybox.h"
#include "engine3d/ParticleSystem3D.h"
#include "engine3d/SceneManager.h"
#include "audio/AudioSystem.h"
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
std::unique_ptr<Shader> g_particleShader;
Camera3D g_cam;
CharacterController3D g_player;
Skybox g_skybox;
AudioSystem g_audio;

// 骨骼模型 & 动画
SkeletalModel g_skelModel;
std::vector<AnimClip> g_anims;
int g_currentAnim = 0;
float g_animTime = 0;
float g_animSpeed = 1.0f;
Mat4 g_boneMatrices[MAX_BONES];

// 动画混合
float g_blendDuration = 0.3f;
float g_blendTimer = 0;
Mat4 g_prevBoneMatrices[MAX_BONES];
Mat4 g_nextBoneMatrices[MAX_BONES];

// 材质 & 场景物体
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
struct DirLightData { Vec3 dir = Vec3(0.4f,0.7f,0.3f); Vec3 color = Vec3(1,0.95f,0.9f); float intensity = 1.2f; bool on = true; };
struct PtLightData { Vec3 pos = Vec3(3,4,3); Vec3 color = Vec3(1,0.8f,0.6f); float intensity = 1.5f; float lin = 0.09f, quad = 0.032f; bool on = true; };
DirLightData g_dirLight;
PtLightData g_ptLight;
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

// 粒子
ParticleEmitter3D g_hitParticles;
ParticleEmitter3D g_dashParticles;
ParticleEmitter3D g_ambientParticles;

// 触发器 & 场景
std::vector<Trigger3D> g_triggers;
std::vector<SceneConfig> g_sceneConfigs;
int g_currentScene = 0;
FadeTransition g_fade;
std::string g_triggerMsg;
float g_triggerMsgTimer = 0;

// 共享网格
Mesh3D g_sphereMesh;
Mesh3D g_cubeMesh;

// 显示
bool g_showEditor = true;
bool g_mouseLDown = false;
double g_lastMX = 0, g_lastMY = 0;

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
// 场景构建
// ============================================================================
void ClearScene() {
    for (auto& o : g_objects) o.mesh.Destroy();
    g_objects.clear();
    g_enemies.clear();
    g_triggers.clear();
    g_triggerMsg.clear();
    g_triggerMsgTimer = 0;
}

void BuildScene_TrainingGround() {
    auto& cfg = g_sceneConfigs[0];

    // 地面(30x30)
    { SceneObj o; o.name = "Ground"; o.mesh = Mesh3D::CreatePlane(30, 30, 1, 1);
      o.matIdx = 0; o.tiling = 8; o.shininess = 8;
      g_objects.push_back(std::move(o)); }

    // 砖墙
    { SceneObj o; o.name = "Wall 1"; o.mesh = Mesh3D::CreateCube(2);
      o.pos = Vec3(-5, 1, 3); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 2);
      g_objects.push_back(std::move(o)); }

    { SceneObj o; o.name = "Wall 2"; o.mesh = Mesh3D::CreateCube(2);
      o.pos = Vec3(5, 1, -3); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 2);
      g_objects.push_back(std::move(o)); }

    // 边界墙
    { SceneObj o; o.name = "Border N"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(0, 1, 14); o.scl = Vec3(30, 2, 1); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }
    { SceneObj o; o.name = "Border S"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(0, 1, -14); o.scl = Vec3(30, 2, 1); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }
    { SceneObj o; o.name = "Border E"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(14, 1, 0); o.scl = Vec3(1, 2, 30); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }
    { SceneObj o; o.name = "Border W"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(-14, 1, 0); o.scl = Vec3(1, 2, 30); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }

    // 木箱
    { SceneObj o; o.name = "Crate 1"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(-7, 0.5f, -5); o.matIdx = 4; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 1);
      g_objects.push_back(std::move(o)); }
    { SceneObj o; o.name = "Crate 2"; o.mesh = Mesh3D::CreateCube(0.8f);
      o.pos = Vec3(3, 0.4f, 6); o.matIdx = 4; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 0.8f);
      g_objects.push_back(std::move(o)); }

    // 金属柱
    { SceneObj o; o.name = "Pillar"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(7, 2, 7); o.scl = Vec3(1, 4, 1); o.matIdx = 3; o.shininess = 64;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }

    // 装饰球
    { SceneObj o; o.name = "Metal Ball"; o.mesh = Mesh3D::CreateSphere(0.8f, 24, 16);
      o.pos = Vec3(-3, 0.8f, 5); o.matIdx = 2; o.shininess = 128;
      g_objects.push_back(std::move(o)); }

    // 2个巡逻敌人
    {
        EnemyAI3D e;
        e.name = "Guard A"; e.position = Vec3(4, 0.9f, 4);
        e.hp = 100; e.maxHp = 100;
        e.patrolPointA = Vec3(2, 0.9f, 2); e.patrolPointB = Vec3(8, 0.9f, 8);
        e.detectionRange = 10; e.attackRange = 2.0f; e.moveSpeed = 3.0f;
        e.attackDamage = 10;
        g_enemies.push_back(e);
    }
    {
        EnemyAI3D e;
        e.name = "Guard B"; e.position = Vec3(-4, 0.9f, -4);
        e.hp = 80; e.maxHp = 80;
        e.patrolPointA = Vec3(-8, 0.9f, -2); e.patrolPointB = Vec3(-2, 0.9f, -8);
        e.detectionRange = 8; e.attackRange = 2.5f; e.moveSpeed = 3.5f;
        e.attackDamage = 15;
        g_enemies.push_back(e);
    }

    // 触发器：传送到竞技场
    { Trigger3D t;
      t.position = Vec3(12, 1, 12); t.halfSize = Vec3(2, 2, 2);
      t.action = "goto_arena"; t.message = ">> Enter Arena >>";
      g_triggers.push_back(t); }

    // 触发器：文字提示
    { Trigger3D t;
      t.position = Vec3(0, 1, 5); t.halfSize = Vec3(3, 2, 2);
      t.action = "show_text"; t.message = "Training Ground - Practice your combos!";
      t.oneShot = false;
      g_triggers.push_back(t); }

    // 光照
    g_dirLight.dir = cfg.sunDir; g_dirLight.color = cfg.sunColor;
    g_dirLight.intensity = cfg.sunIntensity; g_ambient = cfg.ambient;

    // 天空
    g_skybox.topColor = cfg.skyTop;
    g_skybox.bottomColor = cfg.skyBottom;
    g_skybox.horizonColor = cfg.skyHorizon;
}

void BuildScene_Arena() {
    auto& cfg = g_sceneConfigs[1];

    // 竞技场地面(25x25)
    { SceneObj o; o.name = "Arena Floor"; o.mesh = Mesh3D::CreatePlane(25, 25, 1, 1);
      o.matIdx = 3; o.tiling = 6; o.shininess = 32;
      g_objects.push_back(std::move(o)); }

    // 圆形柱子阵列
    const float arenaR = 10.0f;
    for (int i = 0; i < 8; i++) {
        float ang = i * 3.14159f * 2.0f / 8.0f;
        Vec3 ppos(std::cos(ang) * arenaR, 2, std::sin(ang) * arenaR);
        char nm[32]; snprintf(nm, sizeof(nm), "Pillar %d", i);
        SceneObj o; o.name = nm; o.mesh = Mesh3D::CreateCube(1);
        o.pos = ppos; o.scl = Vec3(1.2f, 4, 1.2f); o.matIdx = 2; o.shininess = 64;
        o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
        g_objects.push_back(std::move(o));
    }

    // 边界墙
    { SceneObj o; o.name = "Wall N"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(0, 1.5f, 12); o.scl = Vec3(25, 3, 1); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }
    { SceneObj o; o.name = "Wall S"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(0, 1.5f, -12); o.scl = Vec3(25, 3, 1); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }
    { SceneObj o; o.name = "Wall E"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(12, 1.5f, 0); o.scl = Vec3(1, 3, 25); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }
    { SceneObj o; o.name = "Wall W"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(-12, 1.5f, 0); o.scl = Vec3(1, 3, 25); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }

    // 中央装饰台
    { SceneObj o; o.name = "Platform"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(0, 0.25f, 0); o.scl = Vec3(4, 0.5f, 4); o.matIdx = 3; o.shininess = 64;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }

    // 3个敌人
    {
        EnemyAI3D e;
        e.name = "Warrior A"; e.position = Vec3(5, 0.9f, 5);
        e.hp = 120; e.maxHp = 120;
        e.patrolPointA = Vec3(3, 0.9f, 3); e.patrolPointB = Vec3(8, 0.9f, 8);
        e.detectionRange = 12; e.attackRange = 2.0f; e.moveSpeed = 3.5f;
        e.attackDamage = 15;
        g_enemies.push_back(e);
    }
    {
        EnemyAI3D e;
        e.name = "Warrior B"; e.position = Vec3(-5, 0.9f, -5);
        e.hp = 100; e.maxHp = 100;
        e.patrolPointA = Vec3(-8, 0.9f, -3); e.patrolPointB = Vec3(-3, 0.9f, -8);
        e.detectionRange = 10; e.attackRange = 2.5f; e.moveSpeed = 4.0f;
        e.attackDamage = 12;
        g_enemies.push_back(e);
    }
    {
        EnemyAI3D e;
        e.name = "Warrior C"; e.position = Vec3(6, 0.9f, -6);
        e.hp = 150; e.maxHp = 150;
        e.patrolPointA = Vec3(4, 0.9f, -4); e.patrolPointB = Vec3(9, 0.9f, -9);
        e.detectionRange = 14; e.attackRange = 2.0f; e.moveSpeed = 3.0f;
        e.attackDamage = 20;
        g_enemies.push_back(e);
    }

    // 触发器：返回训练场
    { Trigger3D t;
      t.position = Vec3(-10, 1, -10); t.halfSize = Vec3(2, 2, 2);
      t.action = "goto_training"; t.message = "<< Return to Training <<";
      g_triggers.push_back(t); }

    // 光照（黄昏）
    g_dirLight.dir = cfg.sunDir; g_dirLight.color = cfg.sunColor;
    g_dirLight.intensity = cfg.sunIntensity; g_ambient = cfg.ambient;

    // 天空（黄昏）
    g_skybox.topColor = cfg.skyTop;
    g_skybox.bottomColor = cfg.skyBottom;
    g_skybox.horizonColor = cfg.skyHorizon;
}

void LoadScene(int index) {
    ClearScene();
    if (index < 0 || index >= (int)g_sceneConfigs.size()) return;
    g_currentScene = index;

    auto& cfg = g_sceneConfigs[index];
    g_player.position = cfg.playerSpawn;
    g_player.state = CharacterState3D();

    // 重置战斗状态
    g_comboState = ComboState::None;
    g_attackTimer = 0; g_comboWindow = 0;
    g_attackAnimDone = false;
    if (!g_anims.empty()) SwitchAnim(0);

    // 重置玩家HP
    g_playerCombat.hp = g_playerCombat.maxHp;
    g_playerCombat.alive = true;

    if (index == 0) BuildScene_TrainingGround();
    else BuildScene_Arena();

    g_audio.PlaySFX("transition", 0.6f);
    std::cout << "Loaded scene: " << cfg.name << std::endl;
}

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // 天空盒
    if (!g_skybox.Init()) {
        std::cerr << "Failed to init skybox!" << std::endl;
        return false;
    }
    g_skybox.LoadCubeMap("data/texture/skybox");

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

    // 粒子着色器
    g_particleShader = std::make_unique<Shader>();
    const char* pvs[] = {"data/shader/particle3d.vs","../data/shader/particle3d.vs","../../data/shader/particle3d.vs"};
    const char* pfs[] = {"data/shader/particle3d.fs","../data/shader/particle3d.fs","../../data/shader/particle3d.fs"};
    ok = false;
    for (int i = 0; i < 3; i++) { if (g_particleShader->LoadFromFile(pvs[i], pfs[i])) { ok = true; break; } }
    if (!ok) { std::cerr << "Failed to load particle shader!" << std::endl; return false; }

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

    // 动画
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

    // 材质: 0=woodfloor, 1=bricks, 2=metal, 3=metalplates, 4=wood
    { Material3D m; m.LoadFromDirectory("woodfloor", "Wood Floor"); m.tiling = 3; g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("bricks", "Bricks"); g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("metal", "Metal"); g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("metalplates", "Metal Plates"); g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("wood", "Wood"); g_materials.push_back(std::move(m)); }

    // 共享网格
    g_sphereMesh = Mesh3D::CreateSphere(1.0f, 12, 8);
    g_cubeMesh = Mesh3D::CreateCube(1.0f);

    // 粒子初始化
    g_hitParticles.maxParticles = 150;
    g_hitParticles.particleLife = 0.6f;
    g_hitParticles.particleSize = 0.35f;
    g_hitParticles.startColor = Vec4(1, 0.9f, 0.4f, 1);
    g_hitParticles.endColor = Vec4(1, 0.3f, 0.05f, 0);
    g_hitParticles.startSpeed = 10.0f;
    g_hitParticles.spread = 1.2f;
    g_hitParticles.gravity = -3.0f;
    g_hitParticles.continuous = false;
    g_hitParticles.Init();

    g_dashParticles.maxParticles = 100;
    g_dashParticles.particleLife = 0.7f;
    g_dashParticles.particleSize = 0.25f;
    g_dashParticles.startColor = Vec4(0.4f, 0.7f, 1, 1);
    g_dashParticles.endColor = Vec4(0.2f, 0.4f, 0.9f, 0);
    g_dashParticles.startSpeed = 3.0f;
    g_dashParticles.spread = 0.6f;
    g_dashParticles.gravity = 0.3f;
    g_dashParticles.continuous = false;
    g_dashParticles.Init();

    g_ambientParticles.maxParticles = 80;
    g_ambientParticles.particleLife = 5.0f;
    g_ambientParticles.particleSize = 0.1f;
    g_ambientParticles.startColor = Vec4(1, 0.95f, 0.8f, 0.5f);
    g_ambientParticles.endColor = Vec4(0.9f, 0.85f, 0.7f, 0);
    g_ambientParticles.startSpeed = 0.4f;
    g_ambientParticles.spread = 3.14f;
    g_ambientParticles.gravity = -0.2f;
    g_ambientParticles.emitRate = 8;
    g_ambientParticles.direction = Vec3(0, -1, 0);
    g_ambientParticles.continuous = true;
    g_ambientParticles.Init();

    // 角色
    g_player.params.groundHeight = 0;
    g_playerCombat.name = "Player";
    g_playerCombat.hp = 100; g_playerCombat.maxHp = 100;
    g_playerCombat.alive = true;

    // 摄像机
    g_cam.mode = CameraMode::TPS;
    g_cam.tpsDistance = 8; g_cam.tpsHeight = 3;
    g_cam.pitch = 20; g_cam.yaw = -30;

    // 骨骼矩阵初始化
    for (int i = 0; i < MAX_BONES; i++) g_boneMatrices[i] = Mat4::Identity();

    // 音频
    if (g_audio.Initialize()) {
        const char* sfxNames[]  = {"hit", "slash", "dash", "jump", "land", "transition"};
        const char* sfxFiles[]  = {"data/audio/sfx3d/hit.mp3", "data/audio/sfx3d/slash.wav",
                                   "data/audio/sfx3d/dash.mp3", "data/audio/sfx3d/jump.mp3",
                                   "data/audio/sfx3d/land.mp3", "data/audio/sfx3d/unsheathe.mp3"};
        for (int i = 0; i < 6; i++) g_audio.LoadSound(sfxNames[i], sfxFiles[i]);
        g_audio.LoadMusic("bgm", "data/audio/bgm/windaswarriors.mp3");
        g_audio.PlayMusic("bgm", 0.3f);
    }

    // 场景配置
    g_sceneConfigs.clear();
    {
        SceneConfig sc;
        sc.name = "Training Ground";
        sc.playerSpawn = Vec3(0, 0.9f, 0);
        sc.skyTop = Vec3(0.2f, 0.4f, 0.8f);
        sc.skyBottom = Vec3(0.8f, 0.85f, 0.9f);
        sc.skyHorizon = Vec3(0.9f, 0.9f, 0.95f);
        sc.ambient = 0.35f;
        sc.sunDir = Vec3(0.4f, 0.7f, 0.3f);
        sc.sunColor = Vec3(1, 0.95f, 0.9f);
        sc.sunIntensity = 1.5f;
        g_sceneConfigs.push_back(sc);
    }
    {
        SceneConfig sc;
        sc.name = "Arena";
        sc.playerSpawn = Vec3(0, 0.9f, 8);
        sc.skyTop = Vec3(0.1f, 0.1f, 0.3f);
        sc.skyBottom = Vec3(0.9f, 0.4f, 0.2f);
        sc.skyHorizon = Vec3(1.0f, 0.6f, 0.3f);
        sc.ambient = 0.3f;
        sc.sunDir = Vec3(0.3f, 0.5f, 0.4f);
        sc.sunColor = Vec3(1, 0.7f, 0.5f);
        sc.sunIntensity = 1.3f;
        g_sceneConfigs.push_back(sc);
    }

    LoadScene(0);

    std::cout << "\n=== Chapter 28: Scene & Level ===" << std::endl;
    std::cout << "WASD: Move | Space: Jump | Shift: Sprint | F: Dash" << std::endl;
    std::cout << "J: Attack (3-hit combo) | E: Editor | R: Reset" << std::endl;
    std::cout << "1/2: Switch Scene | LMB+Drag: Camera | Scroll: Distance" << std::endl;
    return true;
}

// ============================================================================
// 攻击
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

            // 命中火花粒子
            Vec3 sparkPos = (hitCenter + enemy.position + Vec3(0, 0.9f, 0)) * 0.5f;
            g_hitParticles.Burst(sparkPos, 25);
            g_audio.PlaySFX("hit", 0.7f);
        }
    }
    if (!g_attackHit) g_audio.PlaySFX("slash", 0.4f);
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

    // 场景过渡
    g_fade.Update(dt);
    if (g_fade.ShouldSwitch() && g_fade.targetScene >= 0) {
        LoadScene(g_fade.targetScene);
        g_fade.targetScene = -1;
    }

    // E键：编辑面板
    { static bool eL = false; bool e = glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS;
      if (e && !eL) g_showEditor = !g_showEditor; eL = e; }

    // R键：重置场景
    { static bool rL = false; bool r = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS;
      if (r && !rL) LoadScene(g_currentScene); rL = r; }

    // 1/2键：手动切换场景
    { static bool k1L = false; bool k1 = glfwGetKey(w, GLFW_KEY_1) == GLFW_PRESS;
      if (k1 && !k1L && !g_fade.active) g_fade.Start(0);  k1L = k1; }
    { static bool k2L = false; bool k2 = glfwGetKey(w, GLFW_KEY_2) == GLFW_PRESS;
      if (k2 && !k2L && !g_fade.active) g_fade.Start(1);  k2L = k2; }

    // 鼠标旋转
    { bool lb = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
      double mx, my; glfwGetCursorPos(w, &mx, &my);
      if (!ImGui::GetIO().WantCaptureMouse) {
          if (lb && g_mouseLDown) g_cam.ProcessMouseMove((float)(mx - g_lastMX), (float)(my - g_lastMY));
          g_mouseLDown = lb;
      } else { g_mouseLDown = false; }
      g_lastMX = mx; g_lastMY = my; }

    // ---- 攻击输入 ----
    { static bool jL = false; bool j = glfwGetKey(w, GLFW_KEY_J) == GLFW_PRESS;
      if (j && !jL && g_playerCombat.alive) {
          if (g_comboState == ComboState::None) {
              g_comboState = ComboState::Attack1;
              g_attackTimer = ATTACK_DURATION;
              g_attackAnimDone = false;
              if (g_anims.size() > 2) SwitchAnim(2);
              PerformAttack();
          } else if (g_comboWindow > 0 && g_attackAnimDone) {
              if (g_comboState == ComboState::Attack1) g_comboState = ComboState::Attack2;
              else if (g_comboState == ComboState::Attack2) g_comboState = ComboState::Attack3;
              g_attackTimer = ATTACK_DURATION;
              g_attackAnimDone = false;
              if (g_anims.size() > 2) SwitchAnim(2);
              PerformAttack();
          }
      } jL = j; }

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
    { bool sp = glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
      if (sp && !spL) { g_player.ProcessJump(true); g_audio.PlaySFX("jump", 0.5f); }
      spL = sp; }

    // 冲刺
    static bool fL = false;
    { bool f = glfwGetKey(w, GLFW_KEY_F) == GLFW_PRESS;
      if (f && !fL) {
          g_player.ProcessDash(true);
          if (g_player.state.dashing) {
              g_audio.PlaySFX("dash", 0.6f);
          }
      } fL = f; }

    // 冲刺拖尾粒子
    if (g_player.state.dashing) {
        g_dashParticles.Burst(g_player.position + Vec3(0, 0.5f, 0), 5);
    }

    g_player.Update(gameDt);

    // AABB碰撞检测
    for (auto& o : g_objects) {
        if (!o.hasCollision) continue;
        ResolveCollision(g_player.position, PLAYER_COLLISION_RADIUS, o.aabb);
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

    // ---- 触发器检测 ----
    for (auto& t : g_triggers) {
        if (t.oneShot && t.triggered) continue;
        if (t.Contains(g_player.position)) {
            if (!t.triggered) {
                t.triggered = true;
                if (t.action == "goto_arena" && !g_fade.active) {
                    g_fade.Start(1);
                } else if (t.action == "goto_training" && !g_fade.active) {
                    g_fade.Start(0);
                } else if (t.action == "show_text") {
                    g_triggerMsg = t.message;
                    g_triggerMsgTimer = 3.0f;
                }
            }
        } else {
            if (!t.oneShot) t.triggered = false;
        }
    }
    if (g_triggerMsgTimer > 0) g_triggerMsgTimer -= dt;

    // 摄像机跟随
    g_cam.tpsTarget = g_player.position;
    g_cam.orbitTarget = g_player.position + Vec3(0, 1, 0);
    g_cam.UpdateTPS(gameDt);

    // 环境粒子（围绕玩家生成）
    g_ambientParticles.position = g_player.position + Vec3(
        (float)(rand() % 20 - 10), 5.0f + (float)(rand() % 4), (float)(rand() % 20 - 10));

    // 粒子更新
    g_hitParticles.Update(gameDt);
    g_dashParticles.Update(gameDt);
    g_ambientParticles.Update(gameDt);

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

    // 动画自动切换（非攻击状态下）
    if (g_comboState == ComboState::None) {
        if (isMoving && g_currentAnim != 1 && g_anims.size() > 1) SwitchAnim(1);
        if (!isMoving && g_currentAnim != 0 && g_anims.size() > 0) SwitchAnim(0);
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

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat4 view = g_cam.GetViewMatrix();
    Mat4 proj = g_cam.GetProjectionMatrix(aspect);

    // ---- 1. 天空盒（最先渲染，关闭深度写入） ----
    g_skybox.Render(view, proj);

    // ---- 2. 材质着色器：场景物体 ----
    g_matShader->Bind();
    g_matShader->SetMat4("uView", view.m);
    g_matShader->SetMat4("uProjection", proj.m);
    g_matShader->SetVec3("uViewPos", g_cam.GetPosition());
    SetLightUniforms(g_matShader.get());

    for (auto& o : g_objects) RenderSceneObj(o);

    // ---- 3. 敌人渲染 ----
    for (auto& e : g_enemies) {
        if (e.state == AIState::Dead) continue;

        bool isHit = (e.hitTimer > 0);
        Vec4 bodyColor = isHit ? Vec4(1.0f, 0.15f, 0.15f, 1) : Vec4(0.8f, 0.2f, 0.2f, 1);
        Vec4 headColor = isHit ? Vec4(1.0f, 0.3f, 0.3f, 1)  : Vec4(0.9f, 0.3f, 0.3f, 1);

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

        // 朝向指示
        float fx = e.forward.x, fz = e.forward.z;
        Mat4 dirMat = Mat4::Translate(
            e.position.x + fx * 0.9f, e.position.y + 0.9f, e.position.z + fz * 0.9f)
            * Mat4::Scale(0.15f, 0.15f, 0.15f);
        g_matShader->SetMat4("uModel", dirMat.m);
        g_matShader->SetVec4("uBaseColor", Vec4(1, 1, 0, 1));
        g_cubeMesh.Draw();
    }

    // ---- 4. 触发器可视化 ----
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (auto& t : g_triggers) {
        Vec4 trgColor(0.2f, 0.8f, 0.3f, 0.15f);
        if (t.action.find("goto") != std::string::npos) trgColor = Vec4(0.3f, 0.5f, 1.0f, 0.15f);
        Mat4 tm = Mat4::Translate(t.position.x, t.position.y, t.position.z)
                 * Mat4::Scale(t.halfSize.x * 2, t.halfSize.y * 2, t.halfSize.z * 2);
        g_matShader->SetMat4("uModel", tm.m);
        g_matShader->SetBool("uUseDiffuseMap", false);
        g_matShader->SetBool("uUseNormalMap", false);
        g_matShader->SetBool("uUseRoughnessMap", false);
        g_matShader->SetBool("uUseVertexColor", false);
        g_matShader->SetVec4("uBaseColor", trgColor);
        g_matShader->SetFloat("uShininess", 8);
        g_matShader->SetFloat("uTexTiling", 1);
        g_cubeMesh.Draw();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // ---- 5. 攻击hitbox可视化 ----
    if (g_attackTimer > 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        Vec3 fwd = g_player.GetForward();
        Vec3 hitCenter = g_player.position + fwd * 1.5f + Vec3(0, 0.9f, 0);
        Mat4 hm = Mat4::Translate(hitCenter.x, hitCenter.y, hitCenter.z)
                 * Mat4::Scale(g_hitboxRadius, g_hitboxRadius, g_hitboxRadius);
        g_matShader->SetMat4("uModel", hm.m);
        g_matShader->SetVec4("uBaseColor", Vec4(1, 0.8f, 0.2f, 0.3f));
        g_sphereMesh.Draw();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // ---- 6. 蒙皮着色器：玩家X Bot ----
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

    // ---- 7. 粒子（最后渲染，半透明） ----
    g_hitParticles.Render(g_particleShader.get(), view, proj);
    g_dashParticles.Render(g_particleShader.get(), view, proj);
    g_ambientParticles.Render(g_particleShader.get(), view, proj);
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui() {
    int ww, wh; glfwGetFramebufferSize(Engine::Instance().GetWindow(), &ww, &wh);
    float aspect = (ww > 0 && wh > 0) ? (float)ww / (float)wh : 1;
    Mat4 view = g_cam.GetViewMatrix();
    Mat4 proj = g_cam.GetProjectionMatrix(aspect);
    Mat4 vp = proj * view;

    // ---- 敌人头顶血条 ----
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
            ImVec2(sx - barW/2, sy), ImVec2(sx + barW/2, sy + barH), IM_COL32(40, 40, 40, 200));
        ImU32 barCol = (e.hitTimer > 0) ? IM_COL32(255, 100, 100, 255) : IM_COL32(220, 50, 50, 230);
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(sx - barW/2, sy), ImVec2(sx - barW/2 + barW * hpRatio, sy + barH), barCol);
        ImGui::GetForegroundDrawList()->AddRect(
            ImVec2(sx - barW/2, sy), ImVec2(sx + barW/2, sy + barH), IM_COL32(200, 200, 200, 180));

        char hpText[64];
        snprintf(hpText, sizeof(hpText), "%s [%s] %.0f/%.0f",
                 e.name.c_str(), e.StateName(), e.hp, e.maxHp);
        ImGui::GetForegroundDrawList()->AddText(
            ImVec2(sx - 50, sy - 16), IM_COL32(255, 255, 255, 220), hpText);
    }

    // ---- 玩家血条 ----
    {
        float barW = 200, barH = 20;
        float x = 10, y2 = (float)wh - 40;
        float hpRatio = g_playerCombat.hp / g_playerCombat.maxHp;
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(x, y2), ImVec2(x + barW, y2 + barH), IM_COL32(40, 40, 40, 200));
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(x, y2), ImVec2(x + barW * hpRatio, y2 + barH), IM_COL32(50, 200, 50, 230));
        ImGui::GetForegroundDrawList()->AddRect(
            ImVec2(x, y2), ImVec2(x + barW, y2 + barH), IM_COL32(200, 200, 200, 180));
        char hpText[64];
        snprintf(hpText, sizeof(hpText), "Player HP: %.0f/%.0f", g_playerCombat.hp, g_playerCombat.maxHp);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(x + 5, y2 + 3), IM_COL32(255, 255, 255, 220), hpText);
    }

    // ---- 触发器消息 ----
    if (g_triggerMsgTimer > 0 && !g_triggerMsg.empty()) {
        ImVec2 tsz = ImGui::CalcTextSize(g_triggerMsg.c_str());
        float tx = ((float)ww - tsz.x) * 0.5f;
        float ty = (float)wh * 0.3f;
        float alpha = std::min(1.0f, g_triggerMsgTimer) * 255;
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(tx - 10, ty - 5), ImVec2(tx + tsz.x + 10, ty + tsz.y + 5),
            IM_COL32(0, 0, 0, (int)(alpha * 0.6f)), 5);
        ImGui::GetForegroundDrawList()->AddText(
            ImVec2(tx, ty), IM_COL32(255, 255, 200, (int)alpha), g_triggerMsg.c_str());
    }

    // ---- 触发器提示：靠近时显示 ----
    for (auto& t : g_triggers) {
        if (t.action.find("goto") == std::string::npos) continue;
        float dx = g_player.position.x - t.position.x;
        float dz = g_player.position.z - t.position.z;
        float dist = std::sqrt(dx*dx + dz*dz);
        if (dist < t.halfSize.x + 3.0f) {
            Vec3 wp(t.position.x, t.position.y + 3, t.position.z);
            Vec4 clip(
                vp.m[0]*wp.x + vp.m[4]*wp.y + vp.m[8]*wp.z + vp.m[12],
                vp.m[1]*wp.x + vp.m[5]*wp.y + vp.m[9]*wp.z + vp.m[13],
                vp.m[2]*wp.x + vp.m[6]*wp.y + vp.m[10]*wp.z + vp.m[14],
                vp.m[3]*wp.x + vp.m[7]*wp.y + vp.m[11]*wp.z + vp.m[15]
            );
            if (clip.w > 0.01f) {
                float sx = (clip.x/clip.w * 0.5f + 0.5f) * ww;
                float sy = (1.0f - (clip.y/clip.w * 0.5f + 0.5f)) * wh;
                ImVec2 tsz = ImGui::CalcTextSize(t.message.c_str());
                ImGui::GetForegroundDrawList()->AddRectFilled(
                    ImVec2(sx - tsz.x/2 - 8, sy - 4), ImVec2(sx + tsz.x/2 + 8, sy + tsz.y + 4),
                    IM_COL32(30, 80, 180, 180), 4);
                ImGui::GetForegroundDrawList()->AddText(
                    ImVec2(sx - tsz.x/2, sy), IM_COL32(255, 255, 255, 230), t.message.c_str());
            }
        }
    }

    // ---- 场景过渡黑幕 ----
    if (g_fade.active) {
        float a = g_fade.GetAlpha();
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(0, 0), ImVec2((float)ww, (float)wh),
            IM_COL32(0, 0, 0, (int)(a * 255)));
    }

    // ---- HUD ----
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##HUD", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Ch28: Scene & Level");
    ImGui::Separator();
    if (g_currentScene < (int)g_sceneConfigs.size())
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1), "Scene: %s", g_sceneConfigs[g_currentScene].name.c_str());
    ImGui::Text("Pos: (%.1f, %.1f, %.1f)", g_player.position.x, g_player.position.y, g_player.position.z);

    if (!g_anims.empty())
        ImGui::Text("Anim: %s", g_anims[g_currentAnim].name.c_str());

    int aliveCount = 0;
    for (auto& e : g_enemies) if (e.state != AIState::Dead) aliveCount++;
    ImGui::Text("Enemies: %d / %d", aliveCount, (int)g_enemies.size());

    const char* comboNames[] = {"None", "Attack 1", "Attack 2", "Attack 3"};
    ImGui::Text("Combo: %s", comboNames[(int)g_comboState]);
    if (g_hitStop.timer > 0) ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "HIT STOP!");
    ImGui::Text("Particles: %d+%d+%d",
        g_hitParticles.ActiveCount(), g_dashParticles.ActiveCount(), g_ambientParticles.ActiveCount());
    ImGui::Text("FPS: %.0f", Engine::Instance().GetFPS());
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "WASD:Move J:Attack Space:Jump");
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "Shift:Sprint F:Dash E:Editor");
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1, 1), "1/2:Switch Scene R:Reset");
    ImGui::End();

    // ---- 编辑面板 ----
    if (!g_showEditor) return;

    ImGui::Begin("Scene Editor [E]", &g_showEditor, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::BeginTabBar("Ch28Tabs")) {

        if (ImGui::BeginTabItem("Camera")) {
            ImGui::SliderFloat("Yaw", &g_cam.yaw, -180, 180);
            ImGui::SliderFloat("Pitch", &g_cam.pitch, -20, 89);
            ImGui::SliderFloat("Distance", &g_cam.tpsDistance, 2, 20);
            ImGui::SliderFloat("FOV", &g_cam.fov, 10, 120);
            ImGui::SliderFloat("Height", &g_cam.tpsHeight, 0, 10);
            ImGui::SliderFloat("Mouse Sens", &g_cam.mouseSensitivity, 0.01f, 0.5f);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Scene")) {
            ImGui::Text("Current: %s", g_sceneConfigs[g_currentScene].name.c_str());
            for (int i = 0; i < (int)g_sceneConfigs.size(); i++) {
                char btn[64]; snprintf(btn, sizeof(btn), "Load: %s", g_sceneConfigs[i].name.c_str());
                if (ImGui::Button(btn) && !g_fade.active) g_fade.Start(i);
            }
            ImGui::Separator();
            ImGui::Text("Triggers (%d):", (int)g_triggers.size());
            for (int i = 0; i < (int)g_triggers.size(); i++) {
                auto& t = g_triggers[i];
                ImGui::Text("  [%d] %s (%.0f,%.0f,%.0f) %s",
                    i, t.action.c_str(), t.position.x, t.position.y, t.position.z,
                    t.triggered ? "[triggered]" : "");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Particles")) {
            ImGui::Text("Hit Particles:");
            ImGui::SliderFloat("Hit Life##h", &g_hitParticles.particleLife, 0.1f, 2);
            ImGui::SliderFloat("Hit Size##h", &g_hitParticles.particleSize, 0.05f, 0.5f);
            ImGui::SliderFloat("Hit Speed##h", &g_hitParticles.startSpeed, 1, 20);
            ImGui::SliderFloat("Hit Spread##h", &g_hitParticles.spread, 0.1f, 3.14f);
            ImGui::ColorEdit4("Hit Start##h", &g_hitParticles.startColor.x);
            ImGui::ColorEdit4("Hit End##h", &g_hitParticles.endColor.x);
            ImGui::Separator();
            ImGui::Text("Dash Particles:");
            ImGui::SliderFloat("Dash Life##d", &g_dashParticles.particleLife, 0.1f, 2);
            ImGui::SliderFloat("Dash Size##d", &g_dashParticles.particleSize, 0.05f, 0.5f);
            ImGui::ColorEdit4("Dash Start##d", &g_dashParticles.startColor.x);
            ImGui::ColorEdit4("Dash End##d", &g_dashParticles.endColor.x);
            ImGui::Separator();
            ImGui::Text("Ambient Particles:");
            ImGui::SliderFloat("Amb Rate", &g_ambientParticles.emitRate, 0, 20);
            ImGui::SliderFloat("Amb Life##a", &g_ambientParticles.particleLife, 1, 8);
            ImGui::SliderFloat("Amb Size##a", &g_ambientParticles.particleSize, 0.02f, 0.2f);
            ImGui::SliderFloat("Amb Gravity##a", &g_ambientParticles.gravity, -2, 2);
            ImGui::ColorEdit4("Amb Start##a", &g_ambientParticles.startColor.x);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Skybox")) {
            ImGui::Checkbox("Use Texture CubeMap", &g_skybox.useTexture);
            ImGui::Separator();
            ImGui::Text("Procedural Colors:");
            ImGui::ColorEdit3("Top", &g_skybox.topColor.x);
            ImGui::ColorEdit3("Bottom", &g_skybox.bottomColor.x);
            ImGui::ColorEdit3("Horizon", &g_skybox.horizonColor.x);
            ImGui::Separator();
            if (ImGui::Button("Day")) g_skybox.SetDayPreset();
            ImGui::SameLine();
            if (ImGui::Button("Dusk")) g_skybox.SetDuskPreset();
            ImGui::SameLine();
            if (ImGui::Button("Night")) g_skybox.SetNightPreset();
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
            ImGui::Separator();
            if (ImGui::Button("Day Light")) {
                g_dirLight.dir = Vec3(0.4f,0.7f,0.3f); g_dirLight.color = Vec3(1,0.95f,0.9f);
                g_dirLight.intensity = 1.5f; g_ambient = 0.35f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Dusk Light")) {
                g_dirLight.dir = Vec3(0.3f,0.5f,0.4f); g_dirLight.color = Vec3(1,0.7f,0.5f);
                g_dirLight.intensity = 1.3f; g_ambient = 0.3f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Night Light")) {
                g_dirLight.dir = Vec3(0.1f,0.8f,0.2f); g_dirLight.color = Vec3(0.4f,0.4f,0.6f);
                g_dirLight.intensity = 0.7f; g_ambient = 0.15f;
            }
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
                    ImGui::Text("Dist to player: %.1f", e.DistanceTo(g_player.position));
                    if (e.state == AIState::Dead && ImGui::Button("Revive")) {
                        e.state = AIState::Patrol; e.hp = e.maxHp;
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
            ImGui::Text("Player HP: %.0f / %.0f", g_playerCombat.hp, g_playerCombat.maxHp);
            ImGui::SliderFloat("Player HP", &g_playerCombat.hp, 0, g_playerCombat.maxHp);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Objects")) {
            ImGui::Text("Scene objects: %d", (int)g_objects.size());
            for (int i = 0; i < (int)g_objects.size(); i++) {
                auto& o = g_objects[i];
                ImGui::PushID(i + 1000);
                if (ImGui::TreeNode("##o", "[%d] %s", i, o.name.c_str())) {
                    ImGui::DragFloat3("Pos", &o.pos.x, 0.1f);
                    ImGui::DragFloat3("Scl", &o.scl.x, 0.1f);
                    ImGui::Checkbox("Visible", &o.visible);
                    ImGui::Checkbox("Collision", &o.hasCollision);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
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
    g_audio.Shutdown();
    g_skelModel.Destroy();
    for (auto& o : g_objects) o.mesh.Destroy();
    for (auto& m : g_materials) m.Destroy();
    g_sphereMesh.Destroy();
    g_cubeMesh.Destroy();
    g_hitParticles.Destroy();
    g_dashParticles.Destroy();
    g_ambientParticles.Destroy();
    g_skybox.Destroy();
    g_skinShader.reset();
    g_matShader.reset();
    g_particleShader.reset();
}

int main() {
    std::cout << "=== Example 28: Scene & Level ===" << std::endl;
    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth = 1280; cfg.windowHeight = 720;
    cfg.windowTitle = "Mini Engine - Scene & Level";
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
