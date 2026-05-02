/**
 * Chapter 30: Complete 3D Action Game Demo (Final Chapter)
 *
 * Integrates ALL systems from chapters 1-29:
 * - Rendering: material shader, skinning shader, skybox, particles
 * - Physics: AABB collision resolution
 * - Animation: skeletal animation with blending
 * - Combat: 3-hit combo, hitbox/hurtbox, hit stop, knockback
 * - Enemy AI: patrol/chase/attack state machine
 * - Audio: FMOD 3D spatial audio + BGM
 * - UI: HUD HP/MP bars, enemy bars, damage floaters, tips
 * - Scene: 3 levels with fade transitions
 *
 * Game flow: Title -> Level 1 (Tutorial) -> Level 2 (Arena) -> Level 3 (BOSS) -> Victory
 *
 * Controls:
 * - WASD: Move | Space: Jump | J: Attack | Shift: Sprint | F: Dash
 * - Mouse drag: Rotate camera | Scroll: Distance
 * - ESC: Pause | E: Editor | R: Restart (when dead) | Enter: Start/Continue
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
// Game State
// ============================================================================
enum class GameState { Title, Playing, Paused, GameOver, Victory };
GameState g_gameState = GameState::Title;
float g_gameTime = 0;
int g_totalKills = 0;
int g_totalEnemies = 0;

// ============================================================================
// Global Systems
// ============================================================================
std::unique_ptr<Shader> g_skinShader;
std::unique_ptr<Shader> g_matShader;
std::unique_ptr<Shader> g_particleShader;
Camera3D g_cam;
CharacterController3D g_player;
Skybox g_skybox;
AudioSystem3D g_audio;
GameUI3D g_gameUI;

// Skeletal models
SkeletalModel g_playerModel;
SkeletalModel g_zombieModel;
SkeletalModel g_mutantModel;

// Player animations
std::vector<AnimClip> g_playerAnims;
int g_currentAnim = 0;
float g_animTime = 0;
float g_animSpeed = 1.0f;
Mat4 g_boneMatrices[MAX_BONES];
float g_blendDuration = 0.3f;
float g_blendTimer = 0;
Mat4 g_prevBoneMatrices[MAX_BONES];
Mat4 g_nextBoneMatrices[MAX_BONES];

// Enemy animations
std::vector<AnimClip> g_zombieAnims; // 0=Idle, 1=Run, 2=Attack, 3=Death
std::vector<AnimClip> g_mutantAnims; // 0=Idle, 1=Run, 2=Attack, 3=Death

// Materials & scene objects
std::vector<Material3D> g_materials;
struct SceneObj {
    Mesh3D mesh;
    Vec3 pos, rot = Vec3(0,0,0), scl = Vec3(1,1,1);
    int matIdx = -1; float tiling = 1; float shininess = 32;
    bool visible = true; std::string name;
    bool hasCollision = false;
    AABB3D aabb;
    float boundRadius = 1.0f;
};

const float PLAYER_COLLISION_RADIUS = 0.5f;
int g_frustumCulled = 0;

struct FrustumPlane { float a, b, c, d; };
struct Frustum { FrustumPlane planes[6]; };

Frustum ExtractFrustum(const Mat4& vp) {
    Frustum f;
    const float* m = vp.m;
    // Left
    f.planes[0] = {m[3]+m[0], m[7]+m[4], m[11]+m[8], m[15]+m[12]};
    // Right
    f.planes[1] = {m[3]-m[0], m[7]-m[4], m[11]-m[8], m[15]-m[12]};
    // Bottom
    f.planes[2] = {m[3]+m[1], m[7]+m[5], m[11]+m[9], m[15]+m[13]};
    // Top
    f.planes[3] = {m[3]-m[1], m[7]-m[5], m[11]-m[9], m[15]-m[13]};
    // Near
    f.planes[4] = {m[3]+m[2], m[7]+m[6], m[11]+m[10], m[15]+m[14]};
    // Far
    f.planes[5] = {m[3]-m[2], m[7]-m[6], m[11]-m[10], m[15]-m[14]};
    for (int i = 0; i < 6; i++) {
        float len = std::sqrt(f.planes[i].a*f.planes[i].a + f.planes[i].b*f.planes[i].b + f.planes[i].c*f.planes[i].c);
        if (len > 0.0001f) { f.planes[i].a/=len; f.planes[i].b/=len; f.planes[i].c/=len; f.planes[i].d/=len; }
    }
    return f;
}

bool SphereInFrustum(const Frustum& f, Vec3 center, float radius) {
    for (int i = 0; i < 6; i++) {
        float dist = f.planes[i].a*center.x + f.planes[i].b*center.y + f.planes[i].c*center.z + f.planes[i].d;
        if (dist < -radius) return false;
    }
    return true;
}

Frustum g_frustum;

AABB3D ComputeAABB(const Vec3& pos, const Vec3& scl, float baseSize) {
    AABB3D aabb;
    float hx = baseSize * scl.x * 0.5f;
    float hy = baseSize * scl.y * 0.5f;
    float hz = baseSize * scl.z * 0.5f;
    aabb.min = Vec3(pos.x - hx, pos.y - hy, pos.z - hz);
    aabb.max = Vec3(pos.x + hx, pos.y + hy, pos.z + hz);
    return aabb;
}

float ComputeBoundRadius(const Vec3& scl, float baseSize) {
    float hx = baseSize * scl.x * 0.5f;
    float hy = baseSize * scl.y * 0.5f;
    float hz = baseSize * scl.z * 0.5f;
    return std::sqrt(hx*hx + hy*hy + hz*hz);
}

std::vector<SceneObj> g_objects;

// Lighting
struct DirLightData { Vec3 dir = Vec3(0.4f,0.7f,0.3f); Vec3 color = Vec3(1,0.95f,0.9f); float intensity = 1.5f; bool on = true; };
struct PtLightData { Vec3 pos = Vec3(3,4,3); Vec3 color = Vec3(1,0.8f,0.6f); float intensity = 1.5f; float lin = 0.09f, quad = 0.032f; bool on = true; };
DirLightData g_dirLight;
PtLightData g_ptLight;
float g_ambient = 0.3f;

// Combat
HitStop g_hitStop;
CombatEntity g_playerCombat;
float g_attackDamage = 15.0f;
float g_attackKnockback = 5.0f;
float g_hitboxRadius = 2.0f;
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

// Enemy data
struct EnemyInstance {
    EnemyAI3D ai;
    int animIdx = 0;
    float animTime = 0;
    Mat4 bones[MAX_BONES];
    bool isMutant = false;
    bool isBoss = false;
    int bossPhase = 1;
    float baseSpeed = 3.0f;
    float baseAttackInterval = 2.0f;
};
std::vector<EnemyInstance> g_enemies;

// Particles
ParticleEmitter3D g_hitParticles;
ParticleEmitter3D g_dashParticles;
ParticleEmitter3D g_ambientParticles;
ParticleEmitter3D g_deathParticles;
ParticleEmitter3D g_portalParticles;

// Scene
std::vector<SceneConfig> g_sceneConfigs;
int g_currentLevel = 0;
FadeTransition g_fade;
Trigger3D g_exitTrigger;
bool g_exitTriggerActive = false;

// Shared meshes
Mesh3D g_sphereMesh;
Mesh3D g_cubeMesh;

// Display
bool g_showEditor = false;
bool g_mouseLDown = false;
double g_lastMX = 0, g_lastMY = 0;
float g_titleCamAngle = 0;

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

void SwitchAnim(int newIdx) {
    if (newIdx < 0 || newIdx >= (int)g_playerAnims.size() || newIdx == g_currentAnim) return;
    for (int i = 0; i < MAX_BONES; i++) g_prevBoneMatrices[i] = g_boneMatrices[i];
    g_blendTimer = g_blendDuration;
    g_currentAnim = newIdx;
    g_animTime = 0;
}

// ============================================================================
// Level Building
// ============================================================================
void ClearLevel() {
    for (auto& o : g_objects) o.mesh.Destroy();
    g_objects.clear();
    g_enemies.clear();
    g_exitTriggerActive = false;
}

void AddCube(const char* name, float size, Vec3 pos, Vec3 scl, int mat, float shin, bool col) {
    SceneObj o; o.name = name; o.mesh = Mesh3D::CreateCube(size);
    o.pos = pos; o.scl = scl; o.matIdx = mat; o.shininess = shin;
    o.boundRadius = ComputeBoundRadius(scl, size);
    if (col) { o.hasCollision = true; o.aabb = ComputeAABB(pos, scl, size); }
    g_objects.push_back(std::move(o));
}

void AddBorderWalls(float halfW, float halfD, float height, int mat) {
    AddCube("Border N", 1, Vec3(0, height/2, halfD), Vec3(halfW*2, height, 1), mat, 16, true);
    AddCube("Border S", 1, Vec3(0, height/2, -halfD), Vec3(halfW*2, height, 1), mat, 16, true);
    AddCube("Border E", 1, Vec3(halfW, height/2, 0), Vec3(1, height, halfD*2), mat, 16, true);
    AddCube("Border W", 1, Vec3(-halfW, height/2, 0), Vec3(1, height, halfD*2), mat, 16, true);
}

EnemyInstance MakeZombie(const char* name, Vec3 pos, Vec3 patA, Vec3 patB) {
    EnemyInstance ei;
    ei.ai.name = name; ei.ai.position = pos;
    ei.ai.hp = 50; ei.ai.maxHp = 50;
    ei.ai.patrolPointA = patA; ei.ai.patrolPointB = patB;
    ei.ai.detectionRange = 12; ei.ai.attackRange = 2.0f; ei.ai.moveSpeed = 3.0f;
    ei.ai.attackDamage = 8; ei.isMutant = false;
    ei.baseSpeed = 3.0f;
    for (int i = 0; i < MAX_BONES; i++) ei.bones[i] = Mat4::Identity();
    return ei;
}

void BuildLevel_Tutorial() {
    // 60x60 地面
    { SceneObj o; o.name = "Ground"; o.mesh = Mesh3D::CreatePlane(60, 60, 1, 1);
      o.matIdx = 0; o.tiling = 15; o.shininess = 8; o.boundRadius = 50;
      g_objects.push_back(std::move(o)); }

    // 围墙
    AddBorderWalls(29, 29, 3, 1);

    // 障碍物散布 — 墙壁
    AddCube("Wall 1", 2, Vec3(-10, 1, 8), Vec3(1,1,1), 1, 16, true);
    AddCube("Wall 2", 2, Vec3(10, 1, -8), Vec3(1,1,1), 1, 16, true);
    AddCube("Wall 3", 1, Vec3(-15, 1.5f, -12), Vec3(6,3,1), 1, 16, true);
    AddCube("Wall 4", 1, Vec3(18, 1.5f, 5), Vec3(1,3,8), 1, 16, true);

    // 箱子散布
    AddCube("Crate 1", 1, Vec3(-12, 0.5f, -5), Vec3(1,1,1), 4, 16, true);
    AddCube("Crate 2", 0.8f, Vec3(6, 0.4f, 12), Vec3(1,1,1), 4, 16, true);
    AddCube("Crate 3", 1.2f, Vec3(15, 0.6f, -15), Vec3(1,1,1), 4, 16, true);
    AddCube("Crate 4", 1, Vec3(-20, 0.5f, 15), Vec3(1,1,1), 4, 16, true);
    AddCube("Crate 5", 0.9f, Vec3(22, 0.45f, -20), Vec3(1,1,1), 4, 16, true);
    AddCube("Crate 6", 1.1f, Vec3(-5, 0.55f, -18), Vec3(1,1,1), 4, 16, true);

    // 金属柱子
    AddCube("Pillar 1", 1, Vec3(12, 2, 12), Vec3(1,4,1), 2, 64, true);
    AddCube("Pillar 2", 1, Vec3(-12, 2, 12), Vec3(1,4,1), 2, 64, true);
    AddCube("Pillar 3", 1, Vec3(0, 2, -10), Vec3(1,4,1), 2, 64, true);
    AddCube("Pillar 4", 1, Vec3(-20, 2, -20), Vec3(1,4,1), 2, 64, true);

    // 装饰台阶
    AddCube("Platform 1", 1, Vec3(20, 0.3f, 20), Vec3(6,0.6f,6), 3, 32, true);
    AddCube("Platform 2", 1, Vec3(-22, 0.3f, -5), Vec3(4,0.6f,4), 3, 32, true);

    // 3个Zombie分布在更大范围内
    g_enemies.push_back(MakeZombie("Zombie A", Vec3(10, 0, 10), Vec3(5, 0, 5), Vec3(15, 0, 15)));
    g_enemies.push_back(MakeZombie("Zombie B", Vec3(-10, 0, -10), Vec3(-15, 0, -5), Vec3(-5, 0, -15)));
    g_enemies.push_back(MakeZombie("Zombie C", Vec3(-15, 0, 15), Vec3(-20, 0, 10), Vec3(-10, 0, 20)));

    g_totalEnemies = (int)g_enemies.size();
    g_gameUI.ShowTip("WASD: Move | J: Attack | Space: Jump | F: Dash", 5.0f);
}

void BuildLevel_Arena() {
    // 50x50 金属地面
    { SceneObj o; o.name = "Arena Floor"; o.mesh = Mesh3D::CreatePlane(50, 50, 1, 1);
      o.matIdx = 3; o.tiling = 10; o.shininess = 32; o.boundRadius = 40;
      g_objects.push_back(std::move(o)); }

    // 围墙
    AddBorderWalls(24, 24, 4, 1);

    // 8根柱子环形排列
    for (int i = 0; i < 8; i++) {
        float ang = i * 3.14159f * 2.0f / 8.0f;
        char nm[32]; snprintf(nm, sizeof(nm), "Pillar %d", i);
        AddCube(nm, 1, Vec3(std::cos(ang)*14, 2, std::sin(ang)*14), Vec3(1.2f,4,1.2f), 2, 64, true);
    }

    // 中央平台
    AddCube("Center", 1, Vec3(0, 0.4f, 0), Vec3(6,0.8f,6), 3, 48, true);

    // 角落掩体
    AddCube("Cover NE", 1, Vec3(16, 1, 16), Vec3(4,2,1), 1, 16, true);
    AddCube("Cover NW", 1, Vec3(-16, 1, 16), Vec3(1,2,4), 1, 16, true);
    AddCube("Cover SE", 1, Vec3(16, 1, -16), Vec3(1,2,4), 1, 16, true);
    AddCube("Cover SW", 1, Vec3(-16, 1, -16), Vec3(4,2,1), 1, 16, true);

    // 散布箱子
    AddCube("Crate A1", 1, Vec3(8, 0.5f, 18), Vec3(1,1,1), 4, 16, true);
    AddCube("Crate A2", 1, Vec3(-10, 0.5f, -18), Vec3(1,1,1), 4, 16, true);
    AddCube("Crate A3", 0.8f, Vec3(20, 0.4f, -5), Vec3(1,1,1), 4, 16, true);
    AddCube("Crate A4", 1.2f, Vec3(-18, 0.6f, 8), Vec3(1,1,1), 4, 16, true);

    // 4个Zombie分散在四个方向 + 1个Mutant精英在中心附近
    g_enemies.push_back(MakeZombie("Zombie A", Vec3(12,0,12), Vec3(8,0,8), Vec3(16,0,16)));
    g_enemies.push_back(MakeZombie("Zombie B", Vec3(-12,0,12), Vec3(-16,0,8), Vec3(-8,0,16)));
    g_enemies.push_back(MakeZombie("Zombie C", Vec3(12,0,-12), Vec3(8,0,-16), Vec3(16,0,-8)));
    g_enemies.push_back(MakeZombie("Zombie D", Vec3(-12,0,-12), Vec3(-16,0,-16), Vec3(-8,0,-8)));
    {
        EnemyInstance ei;
        ei.ai.name = "Mutant Elite"; ei.ai.position = Vec3(0, 0, -10);
        ei.ai.hp = 150; ei.ai.maxHp = 150;
        ei.ai.patrolPointA = Vec3(-6, 0, -6); ei.ai.patrolPointB = Vec3(6, 0, -14);
        ei.ai.detectionRange = 15; ei.ai.attackRange = 2.5f; ei.ai.moveSpeed = 4.0f;
        ei.ai.attackDamage = 15; ei.isMutant = true;
        ei.baseSpeed = 4.0f;
        for (int i = 0; i < MAX_BONES; i++) ei.bones[i] = Mat4::Identity();
        g_enemies.push_back(ei);
    }

    g_totalEnemies = (int)g_enemies.size();
    g_gameUI.ShowTip("Battle Arena - Defeat all enemies!", 3.0f);
}

void BuildLevel_Boss() {
    // 40x40 砖地面
    { SceneObj o; o.name = "Boss Floor"; o.mesh = Mesh3D::CreatePlane(40, 40, 1, 1);
      o.matIdx = 1; o.tiling = 10; o.shininess = 16; o.boundRadius = 35;
      g_objects.push_back(std::move(o)); }

    // 高墙封闭竞技场
    AddBorderWalls(19, 19, 6, 1);

    // 4根装饰柱
    AddCube("Pillar NE", 1, Vec3(12,3,12), Vec3(1.5f,6,1.5f), 2, 64, true);
    AddCube("Pillar NW", 1, Vec3(-12,3,12), Vec3(1.5f,6,1.5f), 2, 64, true);
    AddCube("Pillar SE", 1, Vec3(12,3,-12), Vec3(1.5f,6,1.5f), 2, 64, true);
    AddCube("Pillar SW", 1, Vec3(-12,3,-12), Vec3(1.5f,6,1.5f), 2, 64, true);

    // 掩体 — 低矮障碍物可以躲避BOSS
    AddCube("Cover 1", 1, Vec3(8, 0.6f, 0), Vec3(2,1.2f,2), 3, 32, true);
    AddCube("Cover 2", 1, Vec3(-8, 0.6f, 0), Vec3(2,1.2f,2), 3, 32, true);
    AddCube("Cover 3", 1, Vec3(0, 0.6f, 8), Vec3(2,1.2f,2), 3, 32, true);
    AddCube("Cover 4", 1, Vec3(0, 0.6f, -8), Vec3(2,1.2f,2), 3, 32, true);

    // BOSS
    {
        EnemyInstance ei;
        ei.ai.name = "MUTANT BOSS"; ei.ai.position = Vec3(0, 0, -10);
        ei.ai.hp = 300; ei.ai.maxHp = 300;
        ei.ai.patrolPointA = Vec3(-8, 0, -6); ei.ai.patrolPointB = Vec3(8, 0, -14);
        ei.ai.detectionRange = 30; ei.ai.attackRange = 2.5f; ei.ai.moveSpeed = 3.5f;
        ei.ai.attackDamage = 20; ei.isMutant = true; ei.isBoss = true;
        ei.bossPhase = 1;
        ei.baseSpeed = 3.5f; ei.baseAttackInterval = 2.0f;
        for (int i = 0; i < MAX_BONES; i++) ei.bones[i] = Mat4::Identity();
        g_enemies.push_back(ei);
    }

    g_totalEnemies = 1;
    g_gameUI.ShowTip("BOSS FIGHT - Mutant Lord!", 4.0f);
}

void LoadLevel(int level) {
    ClearLevel();
    g_currentLevel = level;
    if (level < 0 || level >= (int)g_sceneConfigs.size()) return;

    auto& cfg = g_sceneConfigs[level];
    g_player.position = cfg.playerSpawn;
    g_player.state = CharacterState3D();

    g_comboState = ComboState::None;
    g_attackTimer = 0; g_comboWindow = 0;
    g_attackAnimDone = false;
    if (!g_playerAnims.empty()) SwitchAnim(0);

    g_playerCombat.hp = g_playerCombat.maxHp;
    g_playerCombat.alive = true;

    g_dirLight.dir = cfg.sunDir; g_dirLight.color = cfg.sunColor;
    g_dirLight.intensity = cfg.sunIntensity; g_ambient = cfg.ambient;
    g_skybox.topColor = cfg.skyTop;
    g_skybox.bottomColor = cfg.skyBottom;
    g_skybox.horizonColor = cfg.skyHorizon;

    if (level == 0) BuildLevel_Tutorial();
    else if (level == 1) BuildLevel_Arena();
    else if (level == 2) BuildLevel_Boss();

    // BGM - all levels use boss_battle (same as ch29)
    g_audio.StopBGM();
    g_audio.PlayBGM("bgm_battle", 0.4f);

    g_audio.PlaySound("unsheathe", 0.6f);
    std::cout << "Loaded level " << level << ": " << cfg.name << std::endl;
}

// ============================================================================
// Initialize
// ============================================================================
bool Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if (!g_skybox.Init()) { std::cerr << "Failed to init skybox!" << std::endl; return false; }
    g_skybox.LoadCubeMap("data/texture/skybox");

    // Shaders
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

    // Player model
    if (!Animator::LoadModelFallback("fbx/X Bot.fbx", g_playerModel)) {
        std::cerr << "Failed to load player model!" << std::endl; return false;
    }

    const char* pAnimFiles[] = {"fbx/Idle.fbx", "fbx/Running.fbx", "fbx/Punching.fbx", "fbx/Death.fbx"};
    const char* pAnimNames[] = {"Idle", "Running", "Punching", "Death"};
    for (int i = 0; i < 4; i++) {
        AnimClip clip;
        if (Animator::LoadAnimFallback(pAnimFiles[i], clip)) {
            clip.name = pAnimNames[i]; g_playerAnims.push_back(std::move(clip));
        }
    }
    if (g_playerAnims.empty()) { std::cerr << "No player animations!" << std::endl; return false; }

    // Zombie model
    Animator::LoadModelFallback("fbx/enemies/zombie/Zombie.fbx", g_zombieModel);
    { const char* files[] = {"fbx/enemies/zombie/Zombie_Idle.fbx","fbx/enemies/zombie/Zombie_Running.fbx",
                             "fbx/enemies/zombie/Zombie_Attack.fbx","fbx/enemies/zombie/Zombie_Death.fbx"};
      for (int i = 0; i < 4; i++) { AnimClip c; if (Animator::LoadAnimFallback(files[i], c)) g_zombieAnims.push_back(std::move(c)); } }

    // Mutant model
    Animator::LoadModelFallback("fbx/enemies/mutant/Mutant.fbx", g_mutantModel);
    { const char* files[] = {"fbx/enemies/mutant/Mutant_Idle.fbx","fbx/enemies/mutant/Mutant_Run.fbx",
                             "fbx/enemies/mutant/Mutant_Attack.fbx","fbx/enemies/mutant/Mutant_Death.fbx"};
      for (int i = 0; i < 4; i++) { AnimClip c; if (Animator::LoadAnimFallback(files[i], c)) g_mutantAnims.push_back(std::move(c)); } }

    // Materials
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

    g_deathParticles.maxParticles = 100;
    g_deathParticles.particleLife = 1.2f;
    g_deathParticles.particleSize = 0.3f;
    g_deathParticles.startColor = Vec4(0.5f, 0.5f, 0.5f, 0.8f);
    g_deathParticles.endColor = Vec4(0.3f, 0.3f, 0.3f, 0);
    g_deathParticles.startSpeed = 4.0f;
    g_deathParticles.spread = 2.0f;
    g_deathParticles.gravity = -1.0f;
    g_deathParticles.continuous = false;
    g_deathParticles.Init();

    g_portalParticles.maxParticles = 200;
    g_portalParticles.particleLife = 1.5f;
    g_portalParticles.particleSize = 0.2f;
    g_portalParticles.startColor = Vec4(0.3f, 0.8f, 1.0f, 1.0f);
    g_portalParticles.endColor = Vec4(0.6f, 1.0f, 1.0f, 0);
    g_portalParticles.startSpeed = 3.0f;
    g_portalParticles.spread = 0.8f;
    g_portalParticles.gravity = 0;
    g_portalParticles.direction = Vec3(0, 1, 0);
    g_portalParticles.emitRate = 30;
    g_portalParticles.continuous = true;
    g_portalParticles.Init();

    // Player
    g_player.params.groundHeight = 0;
    g_playerCombat.name = "Player";
    g_playerCombat.hp = 100; g_playerCombat.maxHp = 100;
    g_playerCombat.alive = true;

    // Camera
    g_cam.mode = CameraMode::TPS;
    g_cam.tpsDistance = 8; g_cam.tpsHeight = 3;
    g_cam.pitch = 20; g_cam.yaw = -30;

    for (int i = 0; i < MAX_BONES; i++) g_boneMatrices[i] = Mat4::Identity();

    // Audio
    if (g_audio.Init()) {
        g_audio.LoadSound("hit", "data/audio/sfx3d/hit.mp3", true);
        g_audio.LoadSound("hurt", "data/audio/sfx3d/hurt.mp3", true);
        g_audio.LoadSound("slash", "data/audio/sfx3d/slash.wav", true);
        g_audio.LoadSound("sword_swing", "data/audio/sfx3d/sword_swing.mp3");
        g_audio.LoadSound("dash", "data/audio/sfx3d/dash.mp3");
        g_audio.LoadSound("jump", "data/audio/sfx3d/jump.mp3");
        g_audio.LoadSound("land", "data/audio/sfx3d/land.mp3");
        g_audio.LoadSound("draw_sword", "data/audio/sfx3d/draw_sword.mp3");
        g_audio.LoadSound("unsheathe", "data/audio/sfx3d/unsheathe.mp3");
        g_audio.LoadSound("bgm_battle", "data/audio/bgm/boss_battle.mp3", false, true);
        g_audio.Set3DSettings(1.0f, 1.0f, 1.0f);
    }

    // Level configs
    g_sceneConfigs.clear();
    {
        SceneConfig sc;
        sc.name = "Tutorial - Training Grounds";
        sc.playerSpawn = Vec3(0, 0, 0);
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
        sc.name = "Battle Arena";
        sc.playerSpawn = Vec3(0, 0, 18);
        sc.skyTop = Vec3(0.1f, 0.1f, 0.3f);
        sc.skyBottom = Vec3(0.9f, 0.4f, 0.2f);
        sc.skyHorizon = Vec3(1.0f, 0.6f, 0.3f);
        sc.ambient = 0.3f;
        sc.sunDir = Vec3(0.3f, 0.5f, 0.4f);
        sc.sunColor = Vec3(1, 0.7f, 0.5f);
        sc.sunIntensity = 1.3f;
        g_sceneConfigs.push_back(sc);
    }
    {
        SceneConfig sc;
        sc.name = "BOSS - Mutant Lord";
        sc.playerSpawn = Vec3(0, 0, 15);
        sc.skyTop = Vec3(0.15f, 0.02f, 0.02f);
        sc.skyBottom = Vec3(0.3f, 0.05f, 0.05f);
        sc.skyHorizon = Vec3(0.4f, 0.1f, 0.08f);
        sc.ambient = 0.2f;
        sc.sunDir = Vec3(0.2f, 0.8f, 0.1f);
        sc.sunColor = Vec3(0.8f, 0.3f, 0.2f);
        sc.sunIntensity = 1.0f;
        g_sceneConfigs.push_back(sc);
    }

    g_gameState = GameState::Title;
    g_audio.PlayBGM("bgm_battle", 0.3f);

    std::cout << "\n=== Chapter 30: Complete 3D Action Game Demo ===" << std::endl;
    std::cout << "Press ENTER to start!" << std::endl;
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
    for (auto& ei : g_enemies) {
        auto& enemy = ei.ai;
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
            enemy.hitStunTimer = 0.4f;
            enemy.ReanchorPatrol();

            if (enemy.hp <= 0) {
                enemy.hp = 0; enemy.state = AIState::Dead;
                g_totalKills++;
                g_deathParticles.Burst(enemy.position + Vec3(0, 1, 0), 20);
                g_audio.PlaySound3D("slash", enemy.position, 0.7f);
            }

            g_hitStop.Trigger(g_hitStopDur);
            g_attackHit = true;

            Vec3 sparkPos = (hitCenter + enemy.position + Vec3(0, 0.9f, 0)) * 0.5f;
            g_hitParticles.Burst(sparkPos, 25);
            g_audio.PlaySound3D("hit", enemy.position + Vec3(0, 0.9f, 0), 0.8f);

            bool isCrit = (g_comboState == ComboState::Attack3);
            g_gameUI.SpawnDamageNumber(enemy.position + Vec3(0, 2.2f, 0), dmg, isCrit);

            // BOSS phase transition
            if (ei.isBoss) {
                float hpRatio = enemy.hp / enemy.maxHp;
                int newPhase = (hpRatio > 0.6f) ? 1 : (hpRatio > 0.3f) ? 2 : 3;
                if (newPhase != ei.bossPhase) {
                    ei.bossPhase = newPhase;
                    g_audio.PlaySound("draw_sword", 0.8f);
                    if (newPhase == 2) {
                        enemy.moveSpeed = ei.baseSpeed * 1.5f;
                        g_gameUI.ShowTip("BOSS enraged! Speed increased!", 3.0f);
                    } else if (newPhase == 3) {
                        enemy.moveSpeed = ei.baseSpeed * 2.0f;
                        g_gameUI.ShowTip("BOSS is BERSERK!", 3.0f);
                    }
                    g_hitParticles.startColor = Vec4(1, 0.2f, 0.1f, 1);
                    g_hitParticles.Burst(enemy.position + Vec3(0, 1.5f, 0), 40);
                    g_hitParticles.startColor = Vec4(1, 0.9f, 0.4f, 1);
                }
            }
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

    // Title screen
    if (g_gameState == GameState::Title) {
        g_titleCamAngle += dt * 15.0f;
        g_cam.yaw = g_titleCamAngle;
        g_cam.tpsTarget = Vec3(0, 0, 0);
        g_cam.orbitTarget = Vec3(0, 1, 0);
        g_cam.UpdateTPS(dt);
        g_audio.Update();

        static bool enterL = false;
        bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (enter && !enterL) {
            g_gameState = GameState::Playing;
            g_gameTime = 0; g_totalKills = 0;
            LoadLevel(0);
        }
        enterL = enter;
        return;
    }

    // Victory/GameOver
    if (g_gameState == GameState::Victory || g_gameState == GameState::GameOver) {
        g_audio.Update();
        static bool enterL2 = false;
        bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (enter && !enterL2) {
            g_gameState = GameState::Title;
            g_audio.PlayBGM("bgm_battle", 0.3f);
        }
        enterL2 = enter;

        if (g_gameState == GameState::GameOver) {
            static bool rL = false;
            bool r = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS;
            if (r && !rL) {
                g_gameState = GameState::Playing;
                LoadLevel(g_currentLevel);
            }
            rL = r;
        }
        return;
    }

    // Pause
    { static bool escL = false; bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
      if (esc && !escL) {
          if (g_gameState == GameState::Paused) g_gameState = GameState::Playing;
          else if (g_gameState == GameState::Playing) g_gameState = GameState::Paused;
      }
      escL = esc; }

    if (g_gameState == GameState::Paused) { g_audio.Update(); return; }

    // Playing
    g_gameTime += dt;
    g_hitStop.Update(dt);
    float gameDt = dt * g_hitStop.GetTimeScale();

    // Fade transition
    g_fade.Update(dt);
    if (g_fade.ShouldSwitch() && g_fade.targetScene >= 0) {
        if (g_fade.targetScene >= (int)g_sceneConfigs.size()) {
            g_gameState = GameState::Victory;
            g_audio.StopBGM();
        } else {
            LoadLevel(g_fade.targetScene);
        }
        g_fade.targetScene = -1;
    }

    // E: editor
    { static bool eL = false; bool e = glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS;
      if (e && !eL) g_showEditor = !g_showEditor; eL = e; }

    // Mouse rotation
    { bool lb = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
      double mx, my; glfwGetCursorPos(w, &mx, &my);
      if (!ImGui::GetIO().WantCaptureMouse) {
          if (lb && g_mouseLDown) g_cam.ProcessMouseMove((float)(mx - g_lastMX), (float)(my - g_lastMY));
          g_mouseLDown = lb;
      } else { g_mouseLDown = false; }
      g_lastMX = mx; g_lastMY = my; }

    // Attack
    { static bool jL = false; bool j = glfwGetKey(w, GLFW_KEY_J) == GLFW_PRESS;
      if (j && !jL && g_playerCombat.alive) {
          if (g_comboState == ComboState::None) {
              g_comboState = ComboState::Attack1;
              g_attackTimer = ATTACK_DURATION;
              g_attackAnimDone = false;
              if (g_playerAnims.size() > 2) SwitchAnim(2);
              g_audio.PlaySound("sword_swing", 0.5f);
              PerformAttack();
          } else if (g_comboWindow > 0 && g_attackAnimDone) {
              if (g_comboState == ComboState::Attack1) g_comboState = ComboState::Attack2;
              else if (g_comboState == ComboState::Attack2) g_comboState = ComboState::Attack3;
              g_attackTimer = ATTACK_DURATION;
              g_attackAnimDone = false;
              if (g_playerAnims.size() > 2) SwitchAnim(2);
              g_audio.PlaySound("sword_swing", 0.5f);
              PerformAttack();
          }
      } jL = j; }

    if (g_attackTimer > 0) {
        g_attackTimer -= gameDt;
        if (g_attackTimer <= 0) { g_attackAnimDone = true; g_comboWindow = COMBO_WINDOW; }
    }
    if (g_comboWindow > 0) {
        g_comboWindow -= gameDt;
        if (g_comboWindow <= 0) {
            g_comboState = ComboState::None; g_comboWindow = 0; g_attackAnimDone = false;
            if (g_playerAnims.size() > 0) SwitchAnim(0);
        }
    }

    // Movement
    float inputFwd = 0, inputRt = 0;
    bool isMoving = false;
    if (g_comboState == ComboState::None && g_playerCombat.alive) {
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
      if (sp && !spL && g_playerCombat.alive) {
          g_player.ProcessJump(true);
          g_audio.PlaySound("jump", 0.5f);
      }
      spL = sp; }

    // Dash
    static bool fL = false;
    { bool f = glfwGetKey(w, GLFW_KEY_F) == GLFW_PRESS;
      if (f && !fL && g_playerCombat.alive) {
          g_player.ProcessDash(true);
          if (g_player.state.dashing) g_audio.PlaySound("dash", 0.6f);
      } fL = f; }

    if (g_player.state.dashing) g_dashParticles.Burst(g_player.position + Vec3(0, 0.5f, 0), 5);

    g_player.Update(gameDt);

    if (g_player.state.grounded && !wasOnGround) g_audio.PlaySound("land", 0.4f);

    // AABB collision
    for (auto& o : g_objects) {
        if (!o.hasCollision) continue;
        ResolveCollision(g_player.position, PLAYER_COLLISION_RADIUS, o.aabb);
    }

    // Enemy update
    static float hurtCooldown = 0;
    hurtCooldown -= gameDt;
    for (auto& ei : g_enemies) {
        auto& e = ei.ai;
        e.Update(gameDt, g_player.position);

        // Enemy attack player
        if (e.IsAttacking() && g_playerCombat.alive) {
            float dx = g_player.position.x - e.position.x;
            float dz = g_player.position.z - e.position.z;
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist < e.attackRange + 0.5f) {
                float dmgThisFrame = e.attackDamage * gameDt;
                g_playerCombat.hp -= dmgThisFrame;
                if (hurtCooldown <= 0) {
                    g_audio.PlaySound3D("hurt", e.position + Vec3(0, 0.9f, 0), 0.6f);
                    hurtCooldown = 0.5f;
                }
                if (g_playerCombat.hp <= 0) {
                    g_playerCombat.hp = 0;
                    g_playerCombat.alive = false;
                    g_gameState = GameState::GameOver;
                    if (g_playerAnims.size() > 3) SwitchAnim(3);
                }
            }
        }

        // Enemy animation state
        int prevAnim = ei.animIdx;
        if (e.state == AIState::Dead) ei.animIdx = 3;
        else if (e.IsAttacking()) ei.animIdx = 2;
        else if (e.state == AIState::Chase) ei.animIdx = 1;
        else ei.animIdx = 0;
        if (ei.animIdx != prevAnim) ei.animTime = 0;

        auto& anims = ei.isMutant ? g_mutantAnims : g_zombieAnims;
        auto& model = ei.isMutant ? g_mutantModel : g_zombieModel;
        if (!anims.empty() && model.IsValid() && ei.animIdx < (int)anims.size()) {
            ei.animTime += gameDt;
            bool isDead = (e.state == AIState::Dead);
            Animator::ComputeBoneMatrices(model, anims[ei.animIdx], ei.animTime, ei.bones, !isDead);
        }
    }

    // Check level completion
    bool allDead = true;
    for (auto& ei : g_enemies) { if (ei.ai.state != AIState::Dead) { allDead = false; break; } }

    if (allDead && !g_exitTriggerActive && g_playerCombat.alive) {
        if (g_currentLevel < 2) {
            g_exitTriggerActive = true;
            g_exitTrigger.position = Vec3(0, 0, g_currentLevel == 0 ? 26.0f : 21.0f);
            g_exitTrigger.halfSize = Vec3(4, 3, 3);
            g_audio.PlaySound("draw_sword", 0.8f);
            g_portalParticles.Burst(g_exitTrigger.position + Vec3(0, 2, 0), 50);
            g_gameUI.ShowTip(">>> PORTAL OPENED! Walk into the light! <<<", 8.0f);
        } else {
            if (!g_fade.active) {
                g_fade.Start((int)g_sceneConfigs.size());
            }
        }
    }

    // Portal particles
    if (g_exitTriggerActive) {
        g_portalParticles.position = g_exitTrigger.position + Vec3(
            (float)(rand() % 8 - 4) * 0.5f, 0, (float)(rand() % 4 - 2) * 0.5f);
    }

    // Exit trigger check
    if (g_exitTriggerActive && g_exitTrigger.Contains(g_player.position) && !g_fade.active) {
        g_fade.Start(g_currentLevel + 1);
        g_exitTriggerActive = false;
        g_audio.PlaySound("unsheathe", 0.7f);
    }

    // Camera
    g_cam.tpsTarget = g_player.position;
    g_cam.orbitTarget = g_player.position + Vec3(0, 1, 0);
    g_cam.UpdateTPS(gameDt);

    // 3D listener
    Vec3 camPos = g_cam.GetPosition();
    Vec3 camFwd = g_cam.GetForward();
    g_audio.SetListenerPosition(camPos, camFwd, Vec3(0, 1, 0));
    g_audio.Update();

    // Particles
    g_ambientParticles.position = g_player.position + Vec3(
        (float)(rand() % 20 - 10), 5.0f + (float)(rand() % 4), (float)(rand() % 20 - 10));
    g_hitParticles.Update(gameDt);
    g_dashParticles.Update(gameDt);
    g_ambientParticles.Update(gameDt);
    g_deathParticles.Update(gameDt);
    g_portalParticles.Update(gameDt);

    // UI
    g_gameUI.Update(gameDt);

    // Player animation
    if (!g_playerAnims.empty()) {
        g_animTime += gameDt * g_animSpeed;
        Animator::ComputeBoneMatrices(g_playerModel, g_playerAnims[g_currentAnim], g_animTime, g_boneMatrices);
        if (g_blendTimer > 0) {
            g_blendTimer -= gameDt;
            float blendT = 1.0f - std::max(0.0f, g_blendTimer / g_blendDuration);
            Animator::ComputeBoneMatrices(g_playerModel, g_playerAnims[g_currentAnim], g_animTime, g_nextBoneMatrices);
            Animator::BlendBoneMatrices(g_prevBoneMatrices, g_nextBoneMatrices, blendT, g_boneMatrices);
            if (g_blendTimer <= 0) g_blendTimer = 0;
        }
    }

    if (g_comboState == ComboState::None && g_playerCombat.alive) {
        if (isMoving && g_currentAnim != 1 && g_playerAnims.size() > 1) SwitchAnim(1);
        if (!isMoving && g_currentAnim != 0 && g_playerAnims.size() > 0) SwitchAnim(0);
    }
}

// ============================================================================
// Render
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

void RenderSkinnedModel(SkeletalModel& model, Mat4* bones, Mat4 transform, Vec4 color) {
    if (!model.IsValid()) return;
    g_skinShader->Bind();
    int ww, wh; glfwGetFramebufferSize(Engine::Instance().GetWindow(), &ww, &wh);
    float aspect = (ww > 0 && wh > 0) ? (float)ww / (float)wh : 1;
    g_skinShader->SetMat4("uView", g_cam.GetViewMatrix().m);
    g_skinShader->SetMat4("uProjection", g_cam.GetProjectionMatrix(aspect).m);
    g_skinShader->SetVec3("uViewPos", g_cam.GetPosition());
    SetLightUniforms(g_skinShader.get());
    g_skinShader->SetMat4("uModel", transform.m);
    Mat3 nm; nm.m[0] = nm.m[4] = nm.m[8] = 1;
    nm.m[1] = nm.m[2] = nm.m[3] = nm.m[5] = nm.m[6] = nm.m[7] = 0;
    g_skinShader->SetMat3("uNormalMatrix", nm.m);
    g_skinShader->SetBool("uUseSkinning", true);
    for (int i = 0; i < MAX_BONES && i < (int)model.bones.size(); i++) {
        g_skinShader->SetMat4("uBones[" + std::to_string(i) + "]", bones[i].m);
    }
    g_skinShader->SetBool("uUseDiffuseMap", false);
    g_skinShader->SetBool("uUseNormalMap", false);
    g_skinShader->SetBool("uUseRoughnessMap", false);
    g_skinShader->SetBool("uUseVertexColor", true);
    g_skinShader->SetFloat("uShininess", 32);
    g_skinShader->SetFloat("uTexTiling", 1);
    g_skinShader->SetVec4("uBaseColor", color);
    model.Draw();
}

void Render() {
    Engine& eng = Engine::Instance();
    int ww, wh; glfwGetFramebufferSize(eng.GetWindow(), &ww, &wh);
    float aspect = (ww > 0 && wh > 0) ? (float)ww / (float)wh : 1;

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat4 view = g_cam.GetViewMatrix();
    Mat4 proj = g_cam.GetProjectionMatrix(aspect);

    g_skybox.Render(view, proj);

    Mat4 vp = proj * view;
    g_frustum = ExtractFrustum(vp);
    g_frustumCulled = 0;

    g_matShader->Bind();
    g_matShader->SetMat4("uView", view.m);
    g_matShader->SetMat4("uProjection", proj.m);
    g_matShader->SetVec3("uViewPos", g_cam.GetPosition());
    SetLightUniforms(g_matShader.get());

    for (auto& o : g_objects) {
        if (!SphereInFrustum(g_frustum, o.pos, o.boundRadius)) { g_frustumCulled++; continue; }
        RenderSceneObj(o);
    }

    // Enemies (skinned or fallback spheres)
    for (auto& ei : g_enemies) {
        auto& e = ei.ai;
        if (e.state == AIState::Dead && e.hitTimer <= -3.0f) continue;
        if (!SphereInFrustum(g_frustum, e.position + Vec3(0,1,0), 3.0f)) { g_frustumCulled++; continue; }

        auto& model = ei.isMutant ? g_mutantModel : g_zombieModel;
        float scale = ei.isMutant ? 0.015f : 0.012f;
        if (ei.isBoss) scale = 0.02f;

        Vec4 color = ei.isMutant ? Vec4(0.4f, 0.6f, 0.3f, 1) : Vec4(0.5f, 0.4f, 0.35f, 1);
        if (e.hitTimer > 0) color = Vec4(1.0f, 0.2f, 0.2f, 1);
        if (ei.isBoss && ei.bossPhase >= 3) color = Vec4(0.8f, 0.1f, 0.1f, 1);

        if (model.IsValid()) {
            float yaw = std::atan2(e.forward.x, e.forward.z);
            Mat4 transform = Mat4::Translate(e.position.x, e.position.y, e.position.z)
                           * Mat4::RotateY(yaw) * Mat4::Scale(scale, scale, scale);
            RenderSkinnedModel(model, ei.bones, transform, color);
        } else {
            // Fallback: sphere rendering
            g_matShader->Bind();
            g_matShader->SetBool("uUseDiffuseMap", false);
            g_matShader->SetBool("uUseNormalMap", false);
            g_matShader->SetBool("uUseRoughnessMap", false);
            g_matShader->SetBool("uUseVertexColor", false);
            g_matShader->SetFloat("uShininess", 32);
            g_matShader->SetFloat("uTexTiling", 1);

            float bodyScale = ei.isBoss ? 1.2f : 0.8f;
            Mat4 bodyMat = Mat4::Translate(e.position.x, e.position.y + bodyScale * 0.6f, e.position.z)
                         * Mat4::Scale(bodyScale, bodyScale, bodyScale);
            Mat3 identNm; identNm.m[0] = identNm.m[4] = identNm.m[8] = 1;
            identNm.m[1] = identNm.m[2] = identNm.m[3] = identNm.m[5] = identNm.m[6] = identNm.m[7] = 0;
            g_matShader->SetMat4("uModel", bodyMat.m);
            g_matShader->SetMat3("uNormalMatrix", identNm.m);
            g_matShader->SetVec4("uBaseColor", color);
            g_sphereMesh.Draw();

            Mat4 headMat = Mat4::Translate(e.position.x, e.position.y + bodyScale * 1.5f, e.position.z)
                         * Mat4::Scale(bodyScale * 0.5f, bodyScale * 0.5f, bodyScale * 0.5f);
            g_matShader->SetMat4("uModel", headMat.m);
            g_sphereMesh.Draw();
        }
    }

    // Exit trigger visualization — big glowing portal
    if (g_exitTriggerActive) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        g_matShader->Bind();
        g_matShader->SetBool("uUseDiffuseMap", false);
        g_matShader->SetBool("uUseNormalMap", false);
        g_matShader->SetBool("uUseRoughnessMap", false);
        g_matShader->SetBool("uUseVertexColor", false);
        g_matShader->SetFloat("uShininess", 4);
        g_matShader->SetFloat("uTexTiling", 1);
        Mat3 identNm3; identNm3.m[0]=identNm3.m[4]=identNm3.m[8]=1;
        identNm3.m[1]=identNm3.m[2]=identNm3.m[3]=identNm3.m[5]=identNm3.m[6]=identNm3.m[7]=0;
        g_matShader->SetMat3("uNormalMatrix", identNm3.m);

        float pulse = std::sin(g_gameTime * 3.0f) * 0.15f + 0.85f;
        Vec3 pp = g_exitTrigger.position;

        // Light pillar
        float pillarH = 12.0f * pulse;
        Mat4 pillarMat = Mat4::Translate(pp.x, pillarH * 0.5f, pp.z) * Mat4::Scale(1.5f, pillarH, 1.5f);
        g_matShader->SetMat4("uModel", pillarMat.m);
        g_matShader->SetVec4("uBaseColor", Vec4(0.2f, 0.7f, 1.0f, 0.25f * pulse));
        g_cubeMesh.Draw();

        // Main portal cube (pulsing)
        float sz = 2.5f * pulse;
        Mat4 portalMat = Mat4::Translate(pp.x, 2.0f, pp.z) * Mat4::RotateY(g_gameTime * 60.0f) * Mat4::Scale(sz, sz, sz);
        g_matShader->SetMat4("uModel", portalMat.m);
        g_matShader->SetVec4("uBaseColor", Vec4(0.3f, 0.9f, 1.0f, 0.5f * pulse));
        g_cubeMesh.Draw();

        // Inner spinning cube
        float sz2 = 1.5f * pulse;
        Mat4 innerMat = Mat4::Translate(pp.x, 2.0f, pp.z) * Mat4::RotateY(-g_gameTime * 120.0f) * Mat4::RotateX(g_gameTime * 45.0f) * Mat4::Scale(sz2, sz2, sz2);
        g_matShader->SetMat4("uModel", innerMat.m);
        g_matShader->SetVec4("uBaseColor", Vec4(0.5f, 1.0f, 1.0f, 0.7f * pulse));
        g_cubeMesh.Draw();

        // Ground glow ring (flat disc)
        Mat4 ringMat = Mat4::Translate(pp.x, 0.05f, pp.z) * Mat4::Scale(5.0f * pulse, 0.1f, 5.0f * pulse);
        g_matShader->SetMat4("uModel", ringMat.m);
        g_matShader->SetVec4("uBaseColor", Vec4(0.2f, 0.6f, 1.0f, 0.3f * pulse));
        g_cubeMesh.Draw();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // Player
    if (g_gameState == GameState::Playing || g_gameState == GameState::GameOver) {
        if (g_playerModel.IsValid()) {
            Mat4 playerTransform = Mat4::Translate(g_player.position.x,
                                                   g_player.position.y - g_player.params.height * 0.5f,
                                                   g_player.position.z)
                                 * Mat4::RotateY(g_player.state.currentYaw)
                                 * Mat4::Scale(0.02f, 0.02f, 0.02f);
            RenderSkinnedModel(g_playerModel, g_boneMatrices, playerTransform, Vec4(0.5f, 0.5f, 0.6f, 1));
        }
    }

    // Particles
    g_hitParticles.Render(g_particleShader.get(), view, proj);
    g_dashParticles.Render(g_particleShader.get(), view, proj);
    g_ambientParticles.Render(g_particleShader.get(), view, proj);
    g_deathParticles.Render(g_particleShader.get(), view, proj);
    g_portalParticles.Render(g_particleShader.get(), view, proj);
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

    // Title screen
    if (g_gameState == GameState::Title) {
        auto* dl = ImGui::GetForegroundDrawList();
        const char* title = "Mini Engine 3D Action Demo";
        ImVec2 tsz = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2((ww - tsz.x) * 0.5f, wh * 0.35f), IM_COL32(255, 255, 255, 255), title);
        const char* start = "Press ENTER to Start";
        ImVec2 ssz = ImGui::CalcTextSize(start);
        float alpha = (std::sin(g_titleCamAngle * 0.1f) * 0.5f + 0.5f) * 255;
        dl->AddText(ImVec2((ww - ssz.x) * 0.5f, wh * 0.55f), IM_COL32(200, 200, 255, (int)alpha), start);
        return;
    }

    // Victory
    if (g_gameState == GameState::Victory) {
        auto* dl = ImGui::GetForegroundDrawList();
        const char* vic = "VICTORY!";
        ImVec2 vsz = ImGui::CalcTextSize(vic);
        dl->AddText(ImVec2((ww - vsz.x) * 0.5f, wh * 0.3f), IM_COL32(255, 220, 50, 255), vic);
        char stats[128];
        snprintf(stats, sizeof(stats), "Kills: %d | Time: %.1fs", g_totalKills, g_gameTime);
        ImVec2 stsz = ImGui::CalcTextSize(stats);
        dl->AddText(ImVec2((ww - stsz.x) * 0.5f, wh * 0.45f), IM_COL32(255, 255, 255, 220), stats);
        const char* ret = "Press ENTER to Return to Title";
        ImVec2 rsz = ImGui::CalcTextSize(ret);
        dl->AddText(ImVec2((ww - rsz.x) * 0.5f, wh * 0.6f), IM_COL32(200, 200, 200, 200), ret);
        return;
    }

    // Game Over
    if (g_gameState == GameState::GameOver) {
        auto* dl = ImGui::GetForegroundDrawList();
        const char* go = "GAME OVER";
        ImVec2 gsz = ImGui::CalcTextSize(go);
        dl->AddText(ImVec2((ww - gsz.x) * 0.5f, wh * 0.35f), IM_COL32(255, 50, 50, 255), go);
        const char* restart = "Press R to Restart | ENTER for Title";
        ImVec2 rsz = ImGui::CalcTextSize(restart);
        dl->AddText(ImVec2((ww - rsz.x) * 0.5f, wh * 0.5f), IM_COL32(200, 200, 200, 200), restart);
        return;
    }

    // Paused
    if (g_gameState == GameState::Paused) {
        auto* dl = ImGui::GetForegroundDrawList();
        dl->AddRectFilled(ImVec2(0, 0), ImVec2((float)ww, (float)wh), IM_COL32(0, 0, 0, 128));
        const char* p = "PAUSED - Press ESC to continue";
        ImVec2 psz = ImGui::CalcTextSize(p);
        dl->AddText(ImVec2((ww - psz.x) * 0.5f, wh * 0.45f), IM_COL32(255, 255, 255, 255), p);
        return;
    }

    // --- Playing: HUD ---
    g_gameUI.RenderPlayerHUD(g_playerCombat.hp, g_playerCombat.maxHp, 0, 0, ww, wh);

    // Level name
    if (g_currentLevel < (int)g_sceneConfigs.size()) {
        ImGui::GetForegroundDrawList()->AddText(ImVec2(15, 65),
            IM_COL32(200, 200, 255, 200), g_sceneConfigs[g_currentLevel].name.c_str());
    }

    // Kill counter (top right)
    {
        int alive = 0;
        for (auto& ei : g_enemies) if (ei.ai.state != AIState::Dead) alive++;
        char killText[64];
        snprintf(killText, sizeof(killText), "Defeated: %d / %d", g_totalEnemies - alive, g_totalEnemies);
        ImVec2 ksz = ImGui::CalcTextSize(killText);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(ww - ksz.x - 15, 15),
            IM_COL32(255, 255, 255, 220), killText);
    }

    // Enemy bars
    for (auto& ei : g_enemies) {
        auto& e = ei.ai;
        if (e.state == AIState::Dead) continue;

        // BOSS bar at top of screen
        if (ei.isBoss) {
            float barW = 400, barH = 20;
            float bx = (ww - barW) * 0.5f, by = 30;
            float hpRatio = e.hp / e.maxHp;
            auto* dl = ImGui::GetForegroundDrawList();
            dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + barW, by + barH), IM_COL32(40, 40, 40, 220), 3);
            ImU32 bossCol = (ei.bossPhase >= 3) ? IM_COL32(200, 30, 30, 255) :
                            (ei.bossPhase >= 2) ? IM_COL32(220, 140, 30, 255) : IM_COL32(180, 50, 50, 230);
            dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + barW * hpRatio, by + barH), bossCol, 3);
            dl->AddRect(ImVec2(bx, by), ImVec2(bx + barW, by + barH), IM_COL32(200, 200, 200, 200), 3);
            char bossText[64];
            snprintf(bossText, sizeof(bossText), "%s [Phase %d] %.0f/%.0f", e.name.c_str(), ei.bossPhase, e.hp, e.maxHp);
            ImVec2 btsz = ImGui::CalcTextSize(bossText);
            dl->AddText(ImVec2((ww - btsz.x) * 0.5f, by + 3), IM_COL32(255, 255, 255, 240), bossText);
        } else {
            Vec3 barPos(e.position.x, e.position.y + 2.2f, e.position.z);
            g_gameUI.RenderEnemyBar(barPos, e.hp, e.maxHp, e.name.c_str(), e.hitTimer > 0, vp, ww, wh);
        }
    }

    // Damage numbers + tips
    g_gameUI.RenderDamageNumbers(vp, ww, wh);
    g_gameUI.RenderTip(ww, wh);

    // Exit trigger hint — large marker + edge arrow when off-screen
    if (g_exitTriggerActive) {
        auto* dl = ImGui::GetForegroundDrawList();
        float pulse = std::sin(g_gameTime * 4.0f) * 0.3f + 0.7f;
        int pAlpha = (int)(pulse * 255);

        // Distance to portal
        float pdx = g_exitTrigger.position.x - g_player.position.x;
        float pdz = g_exitTrigger.position.z - g_player.position.z;
        float pDist = std::sqrt(pdx*pdx + pdz*pdz);

        Vec3 wp = g_exitTrigger.position + Vec3(0, 4, 0);
        Vec4 clip4(vp.m[0]*wp.x+vp.m[4]*wp.y+vp.m[8]*wp.z+vp.m[12],
                   vp.m[1]*wp.x+vp.m[5]*wp.y+vp.m[9]*wp.z+vp.m[13],
                   vp.m[2]*wp.x+vp.m[6]*wp.y+vp.m[10]*wp.z+vp.m[14],
                   vp.m[3]*wp.x+vp.m[7]*wp.y+vp.m[11]*wp.z+vp.m[15]);

        float ndcX = clip4.x / (clip4.w > 0.01f ? clip4.w : 0.01f);
        float ndcY = clip4.y / (clip4.w > 0.01f ? clip4.w : 0.01f);
        bool onScreen = clip4.w > 0.01f && ndcX > -1 && ndcX < 1 && ndcY > -1 && ndcY < 1;

        if (onScreen) {
            float sx = (ndcX * 0.5f + 0.5f) * ww;
            float sy = (1.0f - (ndcY * 0.5f + 0.5f)) * wh;

            // Large diamond marker
            float mSz = 20.0f * pulse;
            ImVec2 pts[4] = {ImVec2(sx, sy-mSz), ImVec2(sx+mSz, sy), ImVec2(sx, sy+mSz), ImVec2(sx-mSz, sy)};
            dl->AddConvexPolyFilled(pts, 4, IM_COL32(30, 180, 255, pAlpha/2));
            dl->AddPolyline(pts, 4, IM_COL32(100, 220, 255, pAlpha), ImDrawFlags_Closed, 2.5f);

            // Label
            char portalMsg[64];
            snprintf(portalMsg, sizeof(portalMsg), ">>> PORTAL [%.0fm] <<<", pDist);
            ImVec2 tsz = ImGui::CalcTextSize(portalMsg);
            float lx = sx - tsz.x/2, ly = sy - mSz - tsz.y - 10;
            dl->AddRectFilled(ImVec2(lx-10, ly-4), ImVec2(lx+tsz.x+10, ly+tsz.y+6),
                IM_COL32(0, 80, 160, (int)(180*pulse)), 6);
            dl->AddRect(ImVec2(lx-10, ly-4), ImVec2(lx+tsz.x+10, ly+tsz.y+6),
                IM_COL32(100, 200, 255, pAlpha), 6, 0, 2);
            dl->AddText(ImVec2(lx, ly), IM_COL32(255, 255, 255, pAlpha), portalMsg);
        } else {
            // Off-screen: arrow at screen edge pointing toward portal
            float angle = std::atan2(-ndcY, ndcX);
            if (clip4.w <= 0.01f) angle += 3.14159f;
            float margin = 50.0f;
            float cx = ww * 0.5f, cy = wh * 0.5f;
            float cosA = std::cos(angle), sinA = std::sin(angle);
            float maxR = std::min(cx - margin, cy - margin);
            float ax = cx + cosA * maxR;
            float ay = cy - sinA * maxR;
            ax = std::max(margin, std::min(ax, (float)ww - margin));
            ay = std::max(margin, std::min(ay, (float)wh - margin));

            // Arrow triangle
            float arrSz = 18.0f * pulse;
            ImVec2 arrPts[3] = {
                ImVec2(ax + cosA*arrSz, ay - sinA*arrSz),
                ImVec2(ax - sinA*arrSz*0.5f - cosA*arrSz*0.3f, ay - cosA*arrSz*0.5f + sinA*arrSz*0.3f),
                ImVec2(ax + sinA*arrSz*0.5f - cosA*arrSz*0.3f, ay + cosA*arrSz*0.5f + sinA*arrSz*0.3f)
            };
            dl->AddConvexPolyFilled(arrPts, 3, IM_COL32(30, 180, 255, pAlpha));
            dl->AddPolyline(arrPts, 3, IM_COL32(200, 240, 255, pAlpha), ImDrawFlags_Closed, 2);

            char distMsg[32];
            snprintf(distMsg, sizeof(distMsg), "PORTAL %.0fm", pDist);
            ImVec2 dsz = ImGui::CalcTextSize(distMsg);
            dl->AddRectFilled(ImVec2(ax-dsz.x/2-6, ay+arrSz), ImVec2(ax+dsz.x/2+6, ay+arrSz+dsz.y+6),
                IM_COL32(0, 60, 140, (int)(160*pulse)), 4);
            dl->AddText(ImVec2(ax-dsz.x/2, ay+arrSz+2), IM_COL32(255, 255, 255, pAlpha), distMsg);
        }

        // Bottom banner reminder
        const char* banner = ">> Walk to the PORTAL to enter the next level! <<";
        ImVec2 bsz = ImGui::CalcTextSize(banner);
        float bx2 = (ww - bsz.x) * 0.5f, by2 = (float)wh - 50;
        dl->AddRectFilled(ImVec2(bx2-15, by2-6), ImVec2(bx2+bsz.x+15, by2+bsz.y+6),
            IM_COL32(0, 80, 160, (int)(140*pulse)), 8);
        dl->AddText(ImVec2(bx2, by2), IM_COL32(100, 220, 255, pAlpha), banner);
    }

    // Fade
    if (g_fade.active) {
        float a = g_fade.GetAlpha();
        ImGui::GetForegroundDrawList()->AddRectFilled(
            ImVec2(0, 0), ImVec2((float)ww, (float)wh), IM_COL32(0, 0, 0, (int)(a * 255)));
    }

    // Editor panel
    if (!g_showEditor) return;
    ImGui::Begin("Editor [E]", &g_showEditor, ImGuiWindowFlags_AlwaysAutoResize);
    if (ImGui::BeginTabBar("Ch30Tabs")) {
        if (ImGui::BeginTabItem("Game")) {
            ImGui::Text("State: %s", g_gameState == GameState::Playing ? "Playing" :
                        g_gameState == GameState::Paused ? "Paused" : "Other");
            ImGui::Text("Level: %d - %s", g_currentLevel,
                        g_currentLevel < (int)g_sceneConfigs.size() ? g_sceneConfigs[g_currentLevel].name.c_str() : "?");
            ImGui::Text("Time: %.1fs | Kills: %d", g_gameTime, g_totalKills);
            int alive = 0; for (auto& ei : g_enemies) if (ei.ai.state != AIState::Dead) alive++;
            ImGui::Text("Enemies alive: %d / %d", alive, (int)g_enemies.size());
            ImGui::Text("FPS: %.0f", Engine::Instance().GetFPS());
            ImGui::Text("Objects: %d | Frustum Culled: %d", (int)g_objects.size() + (int)g_enemies.size(), g_frustumCulled);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Player")) {
            ImGui::Text("HP: %.0f / %.0f", g_playerCombat.hp, g_playerCombat.maxHp);
            ImGui::SliderFloat("HP", &g_playerCombat.hp, 0, g_playerCombat.maxHp);
            ImGui::DragFloat3("Pos", &g_player.position.x, 0.1f);
            ImGui::SliderFloat("Attack Damage", &g_attackDamage, 1, 100);
            ImGui::SliderFloat("Hitbox Radius", &g_hitboxRadius, 0.5f, 5);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Enemies")) {
            for (int i = 0; i < (int)g_enemies.size(); i++) {
                auto& ei = g_enemies[i];
                ImGui::PushID(i);
                if (ImGui::TreeNode("##e", "[%d] %s [%s] HP:%.0f", i, ei.ai.name.c_str(),
                    ei.ai.StateName(), ei.ai.hp)) {
                    ImGui::SliderFloat("HP", &ei.ai.hp, 0, ei.ai.maxHp);
                    ImGui::DragFloat3("Pos", &ei.ai.position.x, 0.1f);
                    if (ei.isBoss) ImGui::Text("Boss Phase: %d", ei.bossPhase);
                    if (ei.ai.state == AIState::Dead && ImGui::Button("Revive")) {
                        ei.ai.state = AIState::Patrol; ei.ai.hp = ei.ai.maxHp;
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Camera")) {
            ImGui::SliderFloat("Yaw", &g_cam.yaw, -180, 180);
            ImGui::SliderFloat("Pitch", &g_cam.pitch, -20, 89);
            ImGui::SliderFloat("Distance", &g_cam.tpsDistance, 2, 20);
            ImGui::SliderFloat("FOV", &g_cam.fov, 10, 120);
            ImGui::SliderFloat("Height", &g_cam.tpsHeight, 0, 10);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Audio")) {
            float master = g_audio.GetMasterVolume();
            if (ImGui::SliderFloat("Master", &master, 0, 1.5f)) g_audio.SetMasterVolume(master);
            float bgm = g_audio.GetBGMVolume();
            if (ImGui::SliderFloat("BGM", &bgm, 0, 1.5f)) g_audio.SetBGMVolume(bgm);
            float sfx = g_audio.GetSFXVolume();
            if (ImGui::SliderFloat("SFX", &sfx, 0, 1.5f)) g_audio.SetSFXVolume(sfx);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Lighting")) {
            ImGui::SliderFloat("Ambient", &g_ambient, 0, 0.5f);
            ImGui::DragFloat3("Sun Dir", &g_dirLight.dir.x, 0.01f);
            ImGui::ColorEdit3("Sun Color", &g_dirLight.color.x);
            ImGui::SliderFloat("Sun Intensity", &g_dirLight.intensity, 0, 3);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Combat")) {
            ImGui::SliderFloat("Damage", &g_attackDamage, 1, 100);
            ImGui::SliderFloat("Knockback", &g_attackKnockback, 0, 20);
            ImGui::SliderFloat("Hit Stop", &g_hitStopDur, 0.01f, 0.5f);
            ImGui::SliderFloat("Invincible", &g_invincibleDur, 0.1f, 2);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

// ============================================================================
void Cleanup() {
    g_audio.Shutdown();
    g_playerModel.Destroy();
    g_zombieModel.Destroy();
    g_mutantModel.Destroy();
    for (auto& o : g_objects) o.mesh.Destroy();
    for (auto& m : g_materials) m.Destroy();
    g_sphereMesh.Destroy();
    g_cubeMesh.Destroy();
    g_hitParticles.Destroy();
    g_dashParticles.Destroy();
    g_ambientParticles.Destroy();
    g_deathParticles.Destroy();
    g_portalParticles.Destroy();
    g_skybox.Destroy();
    g_skinShader.reset();
    g_matShader.reset();
    g_particleShader.reset();
}

int main() {
    std::cout << "=== Chapter 30: Complete 3D Action Game Demo ===" << std::endl;
    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth = 1280; cfg.windowHeight = 720;
    cfg.windowTitle = "Mini Engine - 3D Action Game Demo (Final)";
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
