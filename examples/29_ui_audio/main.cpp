/**
 * Chapter 29: UI & Audio + FMOD
 *
 * - FMOD 3D spatial audio (replaces miniaudio)
 * - 3D positional sound effects (hit/slash from enemy position)
 * - Player HUD with HP/MP bars (color gradient)
 * - Enemy overhead health bars (world-to-screen projection)
 * - Damage float numbers (rise + fade)
 * - BGM loop, per-action SFX, 3D listener tracking
 * - All CH28 features retained: skybox, particles, scenes, triggers, combat
 *
 * Controls:
 * - WASD: Move | Space: Jump | J: Attack | Shift: Sprint | F: Dash
 * - Mouse drag: Rotate camera | Scroll: Distance | E: Editor | R: Reset
 * - 1/2: Switch scene
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
#include "engine3d/AudioSystem3D.h"
#include "engine3d/GameUI3D.h"
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
// Global state
// ============================================================================
std::unique_ptr<Shader> g_skinShader;
std::unique_ptr<Shader> g_matShader;
std::unique_ptr<Shader> g_particleShader;
Camera3D g_cam;
CharacterController3D g_player;
Skybox g_skybox;
AudioSystem3D g_audio;
GameUI3D g_gameUI;

// Skeletal model & animation
SkeletalModel g_skelModel;
std::vector<AnimClip> g_anims;
int g_currentAnim = 0;
float g_animTime = 0;
float g_animSpeed = 1.0f;
Mat4 g_boneMatrices[MAX_BONES];

// Animation blending
float g_blendDuration = 0.3f;
float g_blendTimer = 0;
Mat4 g_prevBoneMatrices[MAX_BONES];
Mat4 g_nextBoneMatrices[MAX_BONES];

// Materials & scene objects
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

// Lighting
struct DirLightData { Vec3 dir = Vec3(0.4f,0.7f,0.3f); Vec3 color = Vec3(1,0.95f,0.9f); float intensity = 1.2f; bool on = true; };
struct PtLightData { Vec3 pos = Vec3(3,4,3); Vec3 color = Vec3(1,0.8f,0.6f); float intensity = 1.5f; float lin = 0.09f, quad = 0.032f; bool on = true; };
DirLightData g_dirLight;
PtLightData g_ptLight;
float g_ambient = 0.25f;

// Combat
HitStop g_hitStop;
CombatEntity g_playerCombat;
float g_playerMP = 50.0f;
float g_playerMaxMP = 50.0f;

float g_attackDamage = 25.0f;
float g_attackKnockback = 5.0f;
float g_hitboxRadius = 1.5f;
float g_hitStopDur = 0.1f;
float g_invincibleDur = 0.5f;

enum class ComboState { None, Attack1, Attack2, Attack3 };
ComboState g_comboState = ComboState::None;
float g_attackTimer = 0;
float g_comboWindow = 0;
bool g_attackHit = false;
const float ATTACK_DURATION = 0.5f;
const float COMBO_WINDOW = 0.4f;
bool g_attackAnimDone = false;

// AI enemies
std::vector<EnemyAI3D> g_enemies;

// Particles
ParticleEmitter3D g_hitParticles;
ParticleEmitter3D g_dashParticles;
ParticleEmitter3D g_ambientParticles;

// Triggers & scenes
std::vector<Trigger3D> g_triggers;
std::vector<SceneConfig> g_sceneConfigs;
int g_currentScene = 0;
FadeTransition g_fade;
std::string g_triggerMsg;
float g_triggerMsgTimer = 0;

// Shared meshes
Mesh3D g_sphereMesh;
Mesh3D g_cubeMesh;

// Display
bool g_showEditor = true;
bool g_mouseLDown = false;
double g_lastMX = 0, g_lastMY = 0;

// 3D audio settings
float g_3dRolloff = 1.0f;
float g_3dDistanceFactor = 1.0f;

// ============================================================================
void ScrollCB(GLFWwindow*, double, double dy) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    g_cam.ProcessScroll((float)dy);
}

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
void SwitchAnim(int newIdx) {
    if (newIdx < 0 || newIdx >= (int)g_anims.size() || newIdx == g_currentAnim) return;
    for (int i = 0; i < MAX_BONES; i++) g_prevBoneMatrices[i] = g_boneMatrices[i];
    g_blendTimer = g_blendDuration;
    g_currentAnim = newIdx;
    g_animTime = 0;
}

// ============================================================================
// Scene building
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

    { SceneObj o; o.name = "Ground"; o.mesh = Mesh3D::CreatePlane(30, 30, 1, 1);
      o.matIdx = 0; o.tiling = 8; o.shininess = 8;
      g_objects.push_back(std::move(o)); }

    { SceneObj o; o.name = "Wall 1"; o.mesh = Mesh3D::CreateCube(2);
      o.pos = Vec3(-5, 1, 3); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 2);
      g_objects.push_back(std::move(o)); }

    { SceneObj o; o.name = "Wall 2"; o.mesh = Mesh3D::CreateCube(2);
      o.pos = Vec3(5, 1, -3); o.matIdx = 1; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 2);
      g_objects.push_back(std::move(o)); }

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

    { SceneObj o; o.name = "Crate 1"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(-7, 0.5f, -5); o.matIdx = 4; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 1);
      g_objects.push_back(std::move(o)); }
    { SceneObj o; o.name = "Crate 2"; o.mesh = Mesh3D::CreateCube(0.8f);
      o.pos = Vec3(3, 0.4f, 6); o.matIdx = 4; o.shininess = 16;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, Vec3(1,1,1), 0.8f);
      g_objects.push_back(std::move(o)); }

    { SceneObj o; o.name = "Pillar"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(7, 2, 7); o.scl = Vec3(1, 4, 1); o.matIdx = 3; o.shininess = 64;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }

    { SceneObj o; o.name = "Metal Ball"; o.mesh = Mesh3D::CreateSphere(0.8f, 24, 16);
      o.pos = Vec3(-3, 0.8f, 5); o.matIdx = 2; o.shininess = 128;
      g_objects.push_back(std::move(o)); }

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

    { Trigger3D t;
      t.position = Vec3(12, 1, 12); t.halfSize = Vec3(2, 2, 2);
      t.action = "goto_arena"; t.message = ">> Enter Arena >>";
      g_triggers.push_back(t); }
    { Trigger3D t;
      t.position = Vec3(0, 1, 5); t.halfSize = Vec3(3, 2, 2);
      t.action = "show_text"; t.message = "Training Ground - Practice your combos!";
      t.oneShot = false;
      g_triggers.push_back(t); }

    g_dirLight.dir = cfg.sunDir; g_dirLight.color = cfg.sunColor;
    g_dirLight.intensity = cfg.sunIntensity; g_ambient = cfg.ambient;
    g_skybox.topColor = cfg.skyTop;
    g_skybox.bottomColor = cfg.skyBottom;
    g_skybox.horizonColor = cfg.skyHorizon;
}

void BuildScene_Arena() {
    auto& cfg = g_sceneConfigs[1];

    { SceneObj o; o.name = "Arena Floor"; o.mesh = Mesh3D::CreatePlane(25, 25, 1, 1);
      o.matIdx = 3; o.tiling = 6; o.shininess = 32;
      g_objects.push_back(std::move(o)); }

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

    { SceneObj o; o.name = "Platform"; o.mesh = Mesh3D::CreateCube(1);
      o.pos = Vec3(0, 0.25f, 0); o.scl = Vec3(4, 0.5f, 4); o.matIdx = 3; o.shininess = 64;
      o.hasCollision = true; o.aabb = ComputeAABB(o.pos, o.scl, 1);
      g_objects.push_back(std::move(o)); }

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

    { Trigger3D t;
      t.position = Vec3(-10, 1, -10); t.halfSize = Vec3(2, 2, 2);
      t.action = "goto_training"; t.message = "<< Return to Training <<";
      g_triggers.push_back(t); }

    g_dirLight.dir = cfg.sunDir; g_dirLight.color = cfg.sunColor;
    g_dirLight.intensity = cfg.sunIntensity; g_ambient = cfg.ambient;
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

    g_comboState = ComboState::None;
    g_attackTimer = 0; g_comboWindow = 0;
    g_attackAnimDone = false;
    if (!g_anims.empty()) SwitchAnim(0);

    g_playerCombat.hp = g_playerCombat.maxHp;
    g_playerCombat.alive = true;
    g_playerMP = g_playerMaxMP;

    if (index == 0) BuildScene_TrainingGround();
    else BuildScene_Arena();

    g_audio.PlaySound("draw_sword", 0.6f);
    g_gameUI.ShowTip(cfg.name + " - Fight!", 3.0f);
    std::cout << "Loaded scene: " << cfg.name << std::endl;
}

// ============================================================================
// Initialize
// ============================================================================
bool Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if (!g_skybox.Init()) { std::cerr << "Failed to init skybox!" << std::endl; return false; }
    g_skybox.LoadCubeMap("data/texture/skybox");

    g_skinShader = std::make_unique<Shader>();
    const char* svs[] = {"data/shader/skinning.vs","../data/shader/skinning.vs","../../data/shader/skinning.vs"};
    const char* sfs[] = {"data/shader/skinning.fs","../data/shader/skinning.fs","../../data/shader/skinning.fs"};
    bool ok = false;
    for (int i = 0; i < 3; i++) { if (g_skinShader->LoadFromFile(svs[i], sfs[i])) { ok = true; break; } }
    if (!ok) { std::cerr << "Failed to load skinning shader!" << std::endl; return false; }

    g_matShader = std::make_unique<Shader>();
    const char* mvs[] = {"data/shader/material.vs","../data/shader/material.vs","../../data/shader/material.vs"};
    const char* mfs[] = {"data/shader/material.fs","../data/shader/material.fs","../../data/shader/material.fs"};
    ok = false;
    for (int i = 0; i < 3; i++) { if (g_matShader->LoadFromFile(mvs[i], mfs[i])) { ok = true; break; } }
    if (!ok) { std::cerr << "Failed to load material shader!" << std::endl; return false; }

    g_particleShader = std::make_unique<Shader>();
    const char* pvs[] = {"data/shader/particle3d.vs","../data/shader/particle3d.vs","../../data/shader/particle3d.vs"};
    const char* pfs[] = {"data/shader/particle3d.fs","../data/shader/particle3d.fs","../../data/shader/particle3d.fs"};
    ok = false;
    for (int i = 0; i < 3; i++) { if (g_particleShader->LoadFromFile(pvs[i], pfs[i])) { ok = true; break; } }
    if (!ok) { std::cerr << "Failed to load particle shader!" << std::endl; return false; }

    g_skinShader->Bind();
    g_skinShader->SetInt("uDiffuseMap", 0); g_skinShader->SetInt("uNormalMap", 1); g_skinShader->SetInt("uRoughnessMap", 2);
    g_matShader->Bind();
    g_matShader->SetInt("uDiffuseMap", 0); g_matShader->SetInt("uNormalMap", 1); g_matShader->SetInt("uRoughnessMap", 2);

    if (!Animator::LoadModelFallback("fbx/X Bot.fbx", g_skelModel)) {
        std::cerr << "Failed to load X Bot model!" << std::endl; return false;
    }

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

    { Material3D m; m.LoadFromDirectory("woodfloor", "Wood Floor"); m.tiling = 3; g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("bricks", "Bricks"); g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("metal", "Metal"); g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("metalplates", "Metal Plates"); g_materials.push_back(std::move(m)); }
    { Material3D m; m.LoadFromDirectory("wood", "Wood"); g_materials.push_back(std::move(m)); }

    g_sphereMesh = Mesh3D::CreateSphere(1.0f, 12, 8);
    g_cubeMesh = Mesh3D::CreateCube(1.0f);

    // Particles
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

    // Character
    g_player.params.groundHeight = 0;
    g_playerCombat.name = "Player";
    g_playerCombat.hp = 100; g_playerCombat.maxHp = 100;
    g_playerCombat.alive = true;

    // Camera
    g_cam.mode = CameraMode::TPS;
    g_cam.tpsDistance = 8; g_cam.tpsHeight = 3;
    g_cam.pitch = 20; g_cam.yaw = -30;

    for (int i = 0; i < MAX_BONES; i++) g_boneMatrices[i] = Mat4::Identity();

    // FMOD audio
    if (g_audio.Init()) {
        g_audio.LoadSound("hit", "data/audio/sfx3d/hit.mp3", true);
        g_audio.LoadSound("hurt", "data/audio/sfx3d/hurt.mp3", true);
        g_audio.LoadSound("slash", "data/audio/sfx3d/slash.wav", true);
        g_audio.LoadSound("sword_swing", "data/audio/sfx3d/sword_swing.mp3", true);
        g_audio.LoadSound("dash", "data/audio/sfx3d/dash.mp3");
        g_audio.LoadSound("jump", "data/audio/sfx3d/jump.mp3");
        g_audio.LoadSound("land", "data/audio/sfx3d/land.mp3");
        g_audio.LoadSound("draw_sword", "data/audio/sfx3d/draw_sword.mp3");
        g_audio.LoadSound("bgm_battle", "data/audio/bgm/boss_battle.mp3", false, true);

        g_audio.Set3DSettings(1.0f, g_3dDistanceFactor, g_3dRolloff);
        g_audio.PlayBGM("bgm_battle", 0.4f);
    }

    // Scene configs
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

    std::cout << "\n=== Chapter 29: UI & Audio + FMOD ===" << std::endl;
    std::cout << "WASD: Move | Space: Jump | Shift: Sprint | F: Dash" << std::endl;
    std::cout << "J: Attack (3-hit combo) | E: Editor | R: Reset" << std::endl;
    std::cout << "1/2: Switch Scene | LMB+Drag: Camera | Scroll: Distance" << std::endl;
    return true;
}

// ============================================================================
// Attack
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

            Vec3 sparkPos = (hitCenter + enemy.position + Vec3(0, 0.9f, 0)) * 0.5f;
            g_hitParticles.Burst(sparkPos, 25);

            // 3D spatial hit sound at enemy position
            g_audio.PlaySound3D("hit", enemy.position + Vec3(0, 0.9f, 0), 0.8f);

            // Damage float number
            bool isCrit = (g_comboState == ComboState::Attack3);
            g_gameUI.SpawnDamageNumber(enemy.position + Vec3(0, 2.2f, 0), dmg, isCrit);
        }
    }
    if (!g_attackHit) {
        g_audio.PlaySound3D("slash", hitCenter, 0.5f);
    }
}

// ============================================================================
// Update
// ============================================================================
void Update(float dt) {
    Engine& eng = Engine::Instance();
    GLFWwindow* w = eng.GetWindow();
    if (dt > 0.1f) dt = 0.1f;

    g_hitStop.Update(dt);
    float gameDt = dt * g_hitStop.GetTimeScale();

    // Fade transition
    g_fade.Update(dt);
    if (g_fade.ShouldSwitch() && g_fade.targetScene >= 0) {
        LoadScene(g_fade.targetScene);
        g_fade.targetScene = -1;
    }

    // E: toggle editor
    { static bool eL = false; bool e = glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS;
      if (e && !eL) g_showEditor = !g_showEditor; eL = e; }

    // R: reset scene
    { static bool rL = false; bool r = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS;
      if (r && !rL) LoadScene(g_currentScene); rL = r; }

    // 1/2: manual scene switch
    { static bool k1L = false; bool k1 = glfwGetKey(w, GLFW_KEY_1) == GLFW_PRESS;
      if (k1 && !k1L && !g_fade.active) g_fade.Start(0);  k1L = k1; }
    { static bool k2L = false; bool k2 = glfwGetKey(w, GLFW_KEY_2) == GLFW_PRESS;
      if (k2 && !k2L && !g_fade.active) g_fade.Start(1);  k2L = k2; }

    // Mouse rotation
    { bool lb = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
      double mx, my; glfwGetCursorPos(w, &mx, &my);
      if (!ImGui::GetIO().WantCaptureMouse) {
          if (lb && g_mouseLDown) g_cam.ProcessMouseMove((float)(mx - g_lastMX), (float)(my - g_lastMY));
          g_mouseLDown = lb;
      } else { g_mouseLDown = false; }
      g_lastMX = mx; g_lastMY = my; }

    // ---- Attack input ----
    { static bool jL = false; bool j = glfwGetKey(w, GLFW_KEY_J) == GLFW_PRESS;
      if (j && !jL && g_playerCombat.alive) {
          if (g_comboState == ComboState::None) {
              g_comboState = ComboState::Attack1;
              g_attackTimer = ATTACK_DURATION;
              g_attackAnimDone = false;
              if (g_anims.size() > 2) SwitchAnim(2);
              g_audio.PlaySound("sword_swing", 0.5f);
              PerformAttack();
          } else if (g_comboWindow > 0 && g_attackAnimDone) {
              if (g_comboState == ComboState::Attack1) g_comboState = ComboState::Attack2;
              else if (g_comboState == ComboState::Attack2) g_comboState = ComboState::Attack3;
              g_attackTimer = ATTACK_DURATION;
              g_attackAnimDone = false;
              if (g_anims.size() > 2) SwitchAnim(2);
              g_audio.PlaySound("sword_swing", 0.5f);
              PerformAttack();
          }
      } jL = j; }

    // Attack timer
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

    // ---- Movement (no movement during attack) ----
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

    // Jump
    static bool spL = false;
    bool wasOnGround = g_player.state.grounded;
    { bool sp = glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
      if (sp && !spL) {
          g_player.ProcessJump(true);
          g_audio.PlaySound("jump", 0.5f);
      }
      spL = sp; }

    // Dash
    static bool fL = false;
    { bool f = glfwGetKey(w, GLFW_KEY_F) == GLFW_PRESS;
      if (f && !fL) {
          g_player.ProcessDash(true);
          if (g_player.state.dashing) {
              g_audio.PlaySound("dash", 0.6f);
          }
      } fL = f; }

    // Dash trail particles
    if (g_player.state.dashing) {
        g_dashParticles.Burst(g_player.position + Vec3(0, 0.5f, 0), 5);
    }

    g_player.Update(gameDt);

    // Landing sound
    if (g_player.state.grounded && !wasOnGround) {
        g_audio.PlaySound("land", 0.4f);
    }

    // AABB collision
    for (auto& o : g_objects) {
        if (!o.hasCollision) continue;
        ResolveCollision(g_player.position, PLAYER_COLLISION_RADIUS, o.aabb);
    }

    // ---- AI enemy update ----
    for (auto& e : g_enemies) {
        e.Update(gameDt, g_player.position);

        if (e.IsAttacking() && g_playerCombat.alive) {
            float dx = g_player.position.x - e.position.x;
            float dz = g_player.position.z - e.position.z;
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist < e.attackRange + 0.5f) {
                float dmgThisFrame = e.attackDamage * gameDt;
                g_playerCombat.hp -= dmgThisFrame;
                // 3D spatial hurt sound from enemy position
                static float hurtCooldown = 0;
                hurtCooldown -= gameDt;
                if (hurtCooldown <= 0) {
                    g_audio.PlaySound3D("hurt", e.position + Vec3(0, 0.9f, 0), 0.6f);
                    hurtCooldown = 0.5f;
                }
                if (g_playerCombat.hp <= 0) {
                    g_playerCombat.hp = 0;
                    g_playerCombat.alive = false;
                    g_gameUI.ShowTip("YOU DIED - Press R to restart", 5.0f);
                }
            }
        }
    }

    // ---- Trigger detection ----
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
                    g_gameUI.ShowTip(t.message, 3.0f);
                }
            }
        } else {
            if (!t.oneShot) t.triggered = false;
        }
    }

    // Camera follow
    g_cam.tpsTarget = g_player.position;
    g_cam.orbitTarget = g_player.position + Vec3(0, 1, 0);
    g_cam.UpdateTPS(gameDt);

    // 3D listener position (camera)
    Vec3 camPos = g_cam.GetPosition();
    Vec3 camFwd = g_cam.GetForward();
    g_audio.SetListenerPosition(camPos, camFwd, Vec3(0, 1, 0));
    g_audio.Update();

    // Ambient particles
    g_ambientParticles.position = g_player.position + Vec3(
        (float)(rand() % 20 - 10), 5.0f + (float)(rand() % 4), (float)(rand() % 20 - 10));

    g_hitParticles.Update(gameDt);
    g_dashParticles.Update(gameDt);
    g_ambientParticles.Update(gameDt);

    // MP regen
    if (g_playerMP < g_playerMaxMP) {
        g_playerMP += 3.0f * gameDt;
        if (g_playerMP > g_playerMaxMP) g_playerMP = g_playerMaxMP;
    }

    // UI update
    g_gameUI.Update(gameDt);

    // Animation
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

    if (g_comboState == ComboState::None) {
        if (isMoving && g_currentAnim != 1 && g_anims.size() > 1) SwitchAnim(1);
        if (!isMoving && g_currentAnim != 0 && g_anims.size() > 0) SwitchAnim(0);
    }
}

// ============================================================================
// Render scene objects
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
// Render
// ============================================================================
void Render() {
    Engine& eng = Engine::Instance();
    int ww, wh; glfwGetFramebufferSize(eng.GetWindow(), &ww, &wh);
    float aspect = (ww > 0 && wh > 0) ? (float)ww / (float)wh : 1;

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat4 view = g_cam.GetViewMatrix();
    Mat4 proj = g_cam.GetProjectionMatrix(aspect);

    // 1. Skybox
    g_skybox.Render(view, proj);

    // 2. Scene objects
    g_matShader->Bind();
    g_matShader->SetMat4("uView", view.m);
    g_matShader->SetMat4("uProjection", proj.m);
    g_matShader->SetVec3("uViewPos", g_cam.GetPosition());
    SetLightUniforms(g_matShader.get());

    for (auto& o : g_objects) RenderSceneObj(o);

    // 3. Enemies
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

        Mat4 bodyMat = Mat4::Translate(e.position.x, e.position.y + 0.5f, e.position.z)
                     * Mat4::Scale(0.8f, 0.8f, 0.8f);
        g_matShader->SetMat4("uModel", bodyMat.m);
        g_matShader->SetMat3("uNormalMatrix", identNm.m);
        g_matShader->SetVec4("uBaseColor", bodyColor);
        g_sphereMesh.Draw();

        Mat4 headMat = Mat4::Translate(e.position.x, e.position.y + 1.4f, e.position.z)
                     * Mat4::Scale(0.4f, 0.4f, 0.4f);
        g_matShader->SetMat4("uModel", headMat.m);
        g_matShader->SetVec4("uBaseColor", headColor);
        g_sphereMesh.Draw();

        float fx = e.forward.x, fz = e.forward.z;
        Mat4 dirMat = Mat4::Translate(
            e.position.x + fx * 0.9f, e.position.y + 0.9f, e.position.z + fz * 0.9f)
            * Mat4::Scale(0.15f, 0.15f, 0.15f);
        g_matShader->SetMat4("uModel", dirMat.m);
        g_matShader->SetVec4("uBaseColor", Vec4(1, 1, 0, 1));
        g_cubeMesh.Draw();
    }

    // 4. Trigger visualization
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

    // 5. Attack hitbox visualization
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

    // 6. Skinned player model
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

    // 7. Particles
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

    // ---- Player HUD (HP + MP) ----
    g_gameUI.RenderPlayerHUD(g_playerCombat.hp, g_playerCombat.maxHp,
                             g_playerMP, g_playerMaxMP, ww, wh);

    // ---- Enemy overhead health bars ----
    for (auto& e : g_enemies) {
        if (e.state == AIState::Dead) continue;
        Vec3 barPos(e.position.x, e.position.y + 2.2f, e.position.z);
        g_gameUI.RenderEnemyBar(barPos, e.hp, e.maxHp, e.name.c_str(),
                                e.hitTimer > 0, vp, ww, wh);
    }

    // ---- Damage float numbers ----
    g_gameUI.RenderDamageNumbers(vp, ww, wh);

    // ---- Tip text ----
    g_gameUI.RenderTip(ww, wh);

    // ---- Trigger approach hints ----
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

    // ---- Fade transition overlay ----
    if (g_fade.active) {
        float a = g_fade.GetAlpha();
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(0, 0), ImVec2((float)ww, (float)wh),
            IM_COL32(0, 0, 0, (int)(a * 255)));
    }

    // ---- Info HUD ----
    ImGui::SetNextWindowPos(ImVec2(10, 75));
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##HUD", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
    ImGui::Text("Ch29: UI & Audio + FMOD");
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

    // ---- Editor panel ----
    if (!g_showEditor) return;

    ImGui::Begin("Editor [E]", &g_showEditor, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::BeginTabBar("Ch29Tabs")) {

        if (ImGui::BeginTabItem("Camera")) {
            ImGui::SliderFloat("Yaw", &g_cam.yaw, -180, 180);
            ImGui::SliderFloat("Pitch", &g_cam.pitch, -20, 89);
            ImGui::SliderFloat("Distance", &g_cam.tpsDistance, 2, 20);
            ImGui::SliderFloat("FOV", &g_cam.fov, 10, 120);
            ImGui::SliderFloat("Height", &g_cam.tpsHeight, 0, 10);
            ImGui::SliderFloat("Mouse Sens", &g_cam.mouseSensitivity, 0.01f, 0.5f);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Audio")) {
            float master = g_audio.GetMasterVolume();
            if (ImGui::SliderFloat("Master Volume", &master, 0, 1.5f)) g_audio.SetMasterVolume(master);
            float bgm = g_audio.GetBGMVolume();
            if (ImGui::SliderFloat("BGM Volume", &bgm, 0, 1.5f)) g_audio.SetBGMVolume(bgm);
            float sfx = g_audio.GetSFXVolume();
            if (ImGui::SliderFloat("SFX Volume", &sfx, 0, 1.5f)) g_audio.SetSFXVolume(sfx);
            ImGui::Separator();
            ImGui::Text("3D Audio Settings:");
            if (ImGui::SliderFloat("Rolloff Scale", &g_3dRolloff, 0.1f, 5.0f))
                g_audio.Set3DSettings(1.0f, g_3dDistanceFactor, g_3dRolloff);
            if (ImGui::SliderFloat("Distance Factor", &g_3dDistanceFactor, 0.1f, 10.0f))
                g_audio.Set3DSettings(1.0f, g_3dDistanceFactor, g_3dRolloff);
            ImGui::Separator();
            if (ImGui::Button("Play Hit SFX")) g_audio.PlaySound("hit", 0.7f);
            ImGui::SameLine();
            if (ImGui::Button("Play Slash SFX")) g_audio.PlaySound("slash", 0.5f);
            ImGui::SameLine();
            if (ImGui::Button("Stop BGM")) g_audio.StopBGM();
            if (ImGui::Button("Play BGM")) g_audio.PlayBGM("bgm_battle", 0.4f);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("UI")) {
            ImGui::Text("Player HUD:");
            ImGui::SliderFloat("Bar Width", &g_gameUI.hudBarWidth, 100, 400);
            ImGui::SliderFloat("Bar Height", &g_gameUI.hudBarHeight, 10, 40);
            ImGui::SliderFloat("HUD X", &g_gameUI.hudX, 0, 200);
            ImGui::SliderFloat("HUD Y", &g_gameUI.hudY, 0, 200);
            ImGui::Separator();
            ImGui::Text("Enemy Bars:");
            ImGui::SliderFloat("Enemy Bar W", &g_gameUI.enemyBarWidth, 30, 150);
            ImGui::SliderFloat("Enemy Bar H", &g_gameUI.enemyBarHeight, 4, 20);
            ImGui::Separator();
            ImGui::Text("Damage Numbers:");
            ImGui::SliderFloat("Float Speed", &g_gameUI.floatSpeed, 0.5f, 5.0f);
            ImGui::SliderFloat("Float Life", &g_gameUI.floatLife, 0.3f, 3.0f);
            ImGui::SliderFloat("Crit Scale", &g_gameUI.critFontScale, 1.0f, 3.0f);
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
            ImGui::ColorEdit4("Hit Start##h", &g_hitParticles.startColor.x);
            ImGui::ColorEdit4("Hit End##h", &g_hitParticles.endColor.x);
            ImGui::Separator();
            ImGui::Text("Dash Particles:");
            ImGui::SliderFloat("Dash Life##d", &g_dashParticles.particleLife, 0.1f, 2);
            ImGui::SliderFloat("Dash Size##d", &g_dashParticles.particleSize, 0.05f, 0.5f);
            ImGui::ColorEdit4("Dash Start##d", &g_dashParticles.startColor.x);
            ImGui::Separator();
            ImGui::Text("Ambient Particles:");
            ImGui::SliderFloat("Amb Rate", &g_ambientParticles.emitRate, 0, 20);
            ImGui::SliderFloat("Amb Life##a", &g_ambientParticles.particleLife, 1, 8);
            ImGui::SliderFloat("Amb Size##a", &g_ambientParticles.particleSize, 0.02f, 0.2f);
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
            ImGui::Text("Player MP: %.0f / %.0f", g_playerMP, g_playerMaxMP);
            ImGui::SliderFloat("Player MP", &g_playerMP, 0, g_playerMaxMP);
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
// Cleanup & main
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
    std::cout << "=== Example 29: UI & Audio + FMOD ===" << std::endl;
    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth = 1280; cfg.windowHeight = 720;
    cfg.windowTitle = "Mini Engine - UI & Audio + FMOD";
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
