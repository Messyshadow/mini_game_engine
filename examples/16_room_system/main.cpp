/**
 * 第16章：房间与关卡系统 (Room & Level System)
 *
 * 学习内容：
 * 1. 房间数据定义（代码定义关卡）
 * 2. 房间切换与淡入淡出过渡效果
 * 3. 传送门触发器系统
 * 4. 存档点（重生位置）
 *
 * 操作：
 * - A/D: 移动 | Space: 跳跃 | J: 攻击
 * - W/上箭头: 进入传送门
 * - Tab: 显示调试信息
 * - R: 从存档点重生
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "engine/SpriteRenderer.h"
#include "engine/Texture.h"
#include "engine/Sprite.h"
#include "math/Math.h"
#include "physics/AABB.h"
#include "tilemap/Tilemap.h"
#include "combat/CombatSystem.h"
#include "audio/AudioSystem.h"
#include "ai/EnemyAI.h"
#include "scene/RoomSystem.h"
#include "camera/Camera2D.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 动画与敌人结构（复用第15章）
// ============================================================================
struct SheetInfo { int cols, rows, totalFrames; float frameTime; };

struct AnimCtrl {
    std::string cur = "idle";
    int frame = 0; float timer = 0;
    void Play(const std::string& n) { if (cur != n) { cur = n; frame = 0; timer = 0; } }
    void Update(float dt, int max, float ft) {
        timer += dt; if (timer >= ft) { timer = 0; frame = (frame + 1) % max; }
    }
    void UpdateOnce(float dt, int max, float ft) {
        timer += dt; if (timer >= ft) { timer = 0; frame++; if (frame >= max) frame = max - 1; }
    }
};

struct Enemy {
    Vec2 pos, vel = Vec2(0,0);
    bool faceRight = false, grounded = false, alive = true;
    EnemyType type; EnemyAI ai; CombatComponent combat; AnimCtrl anim;
    std::shared_ptr<Texture> texIdle, texRun, texAtk, texDead;
    SheetInfo siIdle, siRun, siAtk, siDead;
    float deathTimer = 0;
};

// ============================================================================
// 全局变量
// ============================================================================
std::unique_ptr<Renderer2D> g_renderer;
std::unique_ptr<SpriteRenderer> g_spriteRenderer;
std::unique_ptr<Tilemap> g_tilemap;
std::unique_ptr<AudioSystem> g_audio;
RoomManager g_roomMgr;

// 纹理
std::shared_ptr<Texture> g_tilesetTex;
std::shared_ptr<Texture> g_pIdleTex, g_pRunTex, g_pPunchTex;
std::shared_ptr<Texture> g_r4Idle, g_r4Run, g_r4Punch, g_r4Dead;
std::shared_ptr<Texture> g_r8Idle, g_r8Run, g_r8Shoot, g_r8Dead;

// 玩家
struct {
    Vec2 pos = Vec2(100, 86), vel = Vec2(0, 0);
    bool faceRight = true, grounded = false;
    CombatComponent combat; AttackData punch; AnimCtrl anim;
    float speed = 250, jump = 500;
} g_player;

std::vector<Enemy> g_enemies;
bool g_showDebug = false;
Trigger* g_nearTrigger = nullptr;
bool g_enterPressed = false;
float g_doorCooldown = 0;           // 传送门冷却（防止连续触发）
std::unique_ptr<Camera2D> g_camera;

const float GRAVITY = -1200.f;

// ============================================================================
// 辅助函数
// ============================================================================
void CalcUV(int idx, int c, int r, Vec2& mn, Vec2& mx) {
    int col = idx % c, row = idx / c;
    mn = Vec2(col/(float)c, 1.f-(row+1)/(float)r);
    mx = Vec2((col+1)/(float)c, 1.f-row/(float)r);
}

bool LoadTex(std::shared_ptr<Texture>& t, const char* n) {
    t = std::make_shared<Texture>();
    std::string p[] = {std::string("data/texture/")+n, std::string("../data/texture/")+n,
                       std::string("../../data/texture/")+n};
    for (auto& s : p) if (t->Load(s)) return true;
    t->CreateSolidColor(Vec4(1,0,1,1), 64, 64);
    return false;
}

struct ColResult { bool ground=0, ceil=0, wL=0, wR=0; };
ColResult MoveCol(Vec2& pos, Vec2& vel, const Vec2& sz, float dt) {
    ColResult r;
    vel.y += GRAVITY * dt;
    if (vel.y < -800) vel.y = -800;
    Vec2 h = sz * 0.5f;
    
    pos.x += vel.x * dt;
    AABB box(pos, h);
    for (const AABB& t : g_tilemap->GetCollidingTiles(box)) {
        if (box.Intersects(t)) {
            Vec2 p = box.GetPenetration(t);
            if (vel.x > 0) { pos.x -= std::abs(p.x); r.wR = true; }
            else { pos.x += std::abs(p.x); r.wL = true; }
            vel.x = 0; box = AABB(pos, h);
        }
    }
    pos.y += vel.y * dt;
    box = AABB(pos, h);
    for (const AABB& t : g_tilemap->GetCollidingTiles(box)) {
        if (box.Intersects(t)) {
            Vec2 p = box.GetPenetration(t);
            if (vel.y < 0) { pos.y += std::abs(p.y); r.ground = true; }
            else { pos.y -= std::abs(p.y); r.ceil = true; }
            vel.y = 0; box = AABB(pos, h);
        }
    }
    return r;
}

void DrawChar(std::shared_ptr<Texture> tex, int c, int r, int fr,
              Vec2 pos, bool fR, Vec4 col = Vec4(1,1,1,1), float sz = 120) {
    Vec2 mn, mx; CalcUV(fr, c, r, mn, mx);
    if (!fR) std::swap(mn.x, mx.x);
    Sprite s; s.SetTexture(tex); s.SetPosition(pos);
    s.SetSize(Vec2(sz, sz)); s.SetUV(mn, mx); s.SetColor(col);
    g_spriteRenderer->DrawSprite(s);
}

// ============================================================================
// 从Room数据构建Tilemap和敌人
// ============================================================================
void BuildRoom(Room* room) {
    if (!room) return;
    
    // 重建地图
    g_tilemap = std::make_unique<Tilemap>();
    g_tilemap->Create(room->width, room->height, room->tileSize);
    int ground = g_tilemap->AddLayer("ground", true);
    
    for (auto& p : room->platforms) {
        for (int dy = 0; dy < p.height; dy++) {
            for (int dx = 0; dx < p.width; dx++) {
                g_tilemap->SetTile(ground, p.x + dx, p.y + dy, p.tileID);
            }
        }
    }
    
    // 重建敌人
    g_enemies.clear();

    // 设置摄像机边界
    if (g_camera) {
        g_camera->SetWorldBounds(0, 0, (float)room->GetPixelWidth(), (float)room->GetPixelHeight());
    }
    
    for (auto& sp : room->enemySpawns) {
        Enemy e;
        e.pos = sp.position;
        e.type = sp.type;
        
        if (sp.type == EnemyType::Melee) {
            e.texIdle = g_r4Idle;  e.siIdle = {7,7,14,0.12f};
            e.texRun  = g_r4Run;   e.siRun  = {7,6,12,0.08f};
            e.texAtk  = g_r4Punch; e.siAtk  = {8,4,10,0.05f};
            e.texDead = g_r4Dead;  e.siDead = {10,3,15,0.08f};
        } else {
            e.texIdle = g_r8Idle;   e.siIdle = {9,6,16,0.1f};
            e.texRun  = g_r8Run;    e.siRun  = {7,6,12,0.08f};
            e.texAtk  = g_r8Shoot;  e.siAtk  = {8,4,10,0.06f};
            e.texDead = g_r8Dead;   e.siDead = {7,4,14,0.08f};
        }
        
        AIConfig cfg;
        cfg.type = sp.type;
        cfg.patrolMinX = sp.patrolMinX;
        cfg.patrolMaxX = sp.patrolMaxX;
        if (sp.type == EnemyType::Ranged) {
            cfg.attackRange = 250; cfg.preferredDistance = 200;
            cfg.retreatDistance = 100; cfg.chaseSpeed = 120;
            cfg.detectionRange = 300; cfg.loseRange = 400;
        } else {
            cfg.attackRange = 65; cfg.chaseSpeed = 160;
            cfg.detectionRange = 220; cfg.loseRange = 320;
        }
        e.ai.SetConfig(cfg);
        
        e.combat.stats.maxHealth = sp.health;
        e.combat.stats.currentHealth = sp.health;
        e.combat.stats.attack = sp.attack;
        e.combat.stats.defense = 1;
        g_enemies.push_back(e);
    }
}

// ============================================================================
// 定义所有房间
// ============================================================================
void DefineRooms() {
    // ---- 房间1：训练场 ----
    {
        Room r;
        r.name = "training"; r.displayName = "Training Ground";
        r.width = 30; r.height = 15;
        r.bgColor = Vec4(0.1f, 0.12f, 0.22f, 1);
        r.defaultSpawn = Vec2(80, 86);
        r.bgmName = "bgm";
        
        r.AddGround(0, 30);
        r.AddWalls(10);
        r.AddPlatform(8, 4, 6);
        r.AddPlatform(18, 5, 5);
        
        r.AddEnemy(EnemyType::Melee, Vec2(300, 86), 200, 450, 30, 6);
        
        r.AddCheckpoint(Vec2(80, 86));
        r.AddDoor(Vec2(r.GetPixelWidth() - 60, 86), "dungeon", Vec2(80, 86), "Dungeon ->");
        
        g_roomMgr.AddRoom(r);
    }
    
    // ---- 房间2：地牢 ----
    {
        Room r;
        r.name = "dungeon"; r.displayName = "Dark Dungeon";
        r.width = 45; r.height = 15;
        r.bgColor = Vec4(0.08f, 0.06f, 0.12f, 1);
        r.defaultSpawn = Vec2(80, 86);
        r.bgmName = "bgm2";
        
        r.AddGround(0, 45);
        r.AddWalls(10);
        r.AddPlatform(6, 4, 8);
        r.AddPlatform(18, 5, 6);
        r.AddPlatform(28, 4, 10);
        r.AddPlatform(12, 8, 5);
        
        r.AddEnemy(EnemyType::Melee, Vec2(280, 86), 200, 500, 50, 10);
        r.AddEnemy(EnemyType::Ranged, Vec2(600, 86), 500, 750, 35, 12);
        r.AddEnemy(EnemyType::Melee, Vec2(900, 86), 800, 1100, 60, 12);
        
        r.AddCheckpoint(Vec2(450, 86));
        r.AddDoor(Vec2(60, 86), "training", Vec2(860, 86), "<- Training");
        r.AddDoor(Vec2(r.GetPixelWidth() - 60, 86), "boss", Vec2(80, 86), "BOSS Room ->");
        
        g_roomMgr.AddRoom(r);
    }
    
    // ---- 房间3：BOSS间 ----
    {
        Room r;
        r.name = "boss"; r.displayName = "BOSS Chamber";
        r.width = 35; r.height = 15;
        r.bgColor = Vec4(0.15f, 0.05f, 0.05f, 1);
        r.defaultSpawn = Vec2(80, 86);
        r.bgmName = "bgm2";
        
        r.AddGround(0, 35);
        r.AddWalls(10);
        r.AddPlatform(10, 4, 6);
        r.AddPlatform(20, 5, 8);
        
        // BOSS级敌人：更高血量和攻击力
        r.AddEnemy(EnemyType::Melee, Vec2(700, 86), 400, 900, 150, 18);
        r.AddEnemy(EnemyType::Ranged, Vec2(500, 86), 350, 650, 60, 14);
        
        r.AddDoor(Vec2(60, 86), "dungeon", Vec2(1340, 86), "<- Dungeon");
        
        g_roomMgr.AddRoom(r);
    }
}

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) return false;
    g_spriteRenderer = std::make_unique<SpriteRenderer>();
    if (!g_spriteRenderer->Initialize()) return false;
    
    g_tilesetTex = std::make_shared<Texture>();
    const char* tp[] = {"data/texture/tileset.png", "../data/texture/tileset.png"};
    for (auto p : tp) if (g_tilesetTex->Load(p)) break;
    
    LoadTex(g_pIdleTex,  "robot3/robot3_idle.png");
    LoadTex(g_pRunTex,   "robot3/robot3_run.png");
    LoadTex(g_pPunchTex, "robot3/robot3_punch.png");
    LoadTex(g_r4Idle, "robot4/robot4_idle.png");
    LoadTex(g_r4Run,  "robot4/robot4_run.png");
    LoadTex(g_r4Punch,"robot4/robot4_punch_r.png");
    LoadTex(g_r4Dead, "robot4/robot4_dead.png");
    LoadTex(g_r8Idle, "robot8/robot8_idle.png");
    LoadTex(g_r8Run,  "robot8/robot8_run.png");
    LoadTex(g_r8Shoot,"robot8/robot8_shoot.png");
    LoadTex(g_r8Dead, "robot8/robot8_dead.png");
    
    g_audio = std::make_unique<AudioSystem>();
    g_audio->Initialize();
    const char* bp[] = {"data/", "../data/", "../../data/"};
    for (auto b : bp) if (g_audio->LoadSound("attack", std::string(b)+"audio/sfx/Sound_Axe.wav")) break;
    for (auto b : bp) if (g_audio->LoadSound("transition", std::string(b)+"audio/sfx/transition.mp3")) break;
    for (auto b : bp) if (g_audio->LoadMusic("bgm", std::string(b)+"audio/bgm/windaswarriors.mp3")) break;
    for (auto b : bp) if (g_audio->LoadMusic("bgm2", std::string(b)+"audio/bgm/vivo.mp3")) break;
    
    // 玩家攻击配置
    g_player.punch.name = "punch";
    g_player.punch.baseDamage = 15;
    g_player.punch.startupFrames = 0.05f;
    g_player.punch.activeFrames = 0.15f;
    g_player.punch.recoveryFrames = 0.1f;
    g_player.punch.knockback = Vec2(120, 30);
    g_player.punch.stunTime = 0.3f;
    g_player.punch.hitboxOffset = Vec2(55, 0);
    g_player.punch.hitboxSize = Vec2(70, 50);
    g_player.punch.animFrameCount = 8;
    g_player.punch.animFPS = 20;
    g_player.combat.stats = {100, 100, 10, 5, 0.1f, 1.5f};
    g_player.combat.onAttackStart = [](const std::string&) { g_audio->PlaySFX("attack", 0.7f); };
    
    // 定义房间并设置回调
    DefineRooms();
    
    // 创建摄像机
    Engine& eng = Engine::Instance();
    g_camera = std::make_unique<Camera2D>();
    g_camera->SetViewportSize((float)eng.GetWindowWidth(), (float)eng.GetWindowHeight());
    g_camera->SetSmoothSpeed(8.0f);
    
    g_roomMgr.onRoomLoaded = [](Room* room) {
        BuildRoom(room);
        g_player.pos = room->defaultSpawn;
        g_player.vel = Vec2(0, 0);
        g_doorCooldown = 1.0f;  // 进入新房间后1秒内不能再触发传送门
        g_enterPressed = true;  // 重置W键状态，防止连续触发
        // 摄像机snap到玩家位置
        if (g_camera) {
            g_camera->SetTarget(g_player.pos);
            g_camera->SnapToTarget();
        }
        if (!room->bgmName.empty()) g_audio->PlayMusic(room->bgmName, 0.3f);
    };
    g_roomMgr.SetRespawn("training", Vec2(80, 86));
    
    // 加载第一个房间
    g_roomMgr.LoadRoom("training", Vec2(80, 86));
    
    std::cout << "\n=== Chapter 16: Room System ===" << std::endl;
    std::cout << "A/D: Move | Space: Jump | J: Attack" << std::endl;
    std::cout << "W: Enter Door | Tab: Debug | R: Respawn" << std::endl;
    
    return true;
}

// ============================================================================
// 更新
// ============================================================================
void UpdatePlayer(float dt, GLFWwindow* w) {
    if (g_roomMgr.IsTransitioning()) { g_player.vel.x = 0; return; }
    
    g_player.combat.Update(dt);
    float mv = 0;
    if (g_player.combat.CanAct()) {
        if (glfwGetKey(w, GLFW_KEY_A)) mv = -1;
        if (glfwGetKey(w, GLFW_KEY_D)) mv = 1;
        if (mv != 0) g_player.faceRight = mv > 0;
        g_player.vel.x = mv * g_player.speed;
    } else g_player.vel.x *= 0.9f;
    
    static bool spL = 0;
    bool sp = glfwGetKey(w, GLFW_KEY_SPACE);
    if (sp && !spL && g_player.grounded && g_player.combat.CanAct()) {
        g_player.vel.y = g_player.jump; g_player.grounded = false;
    }
    spL = sp;
    
    static bool jL = 0;
    bool j = glfwGetKey(w, GLFW_KEY_J);
    if (j && !jL) g_player.combat.StartAttack(g_player.punch);
    jL = j;
    
    ColResult c = MoveCol(g_player.pos, g_player.vel, Vec2(40, 44), dt);
    g_player.grounded = c.ground;
    
    if (g_player.combat.state == CombatState::Attacking) {
        g_player.anim.Play("punch"); g_player.anim.UpdateOnce(dt, 8, 0.05f);
    } else if (std::abs(g_player.vel.x) > 10) {
        g_player.anim.Play("run"); g_player.anim.Update(dt, 9, 0.08f);
    } else {
        g_player.anim.Play("idle"); g_player.anim.Update(dt, 7, 0.1f);
    }
}

void UpdateEnemies(float dt) {
    if (g_roomMgr.IsTransitioning()) return;
    
    for (auto& e : g_enemies) {
        if (!e.alive) {
            e.deathTimer += dt;
            e.anim.Play("dead"); e.anim.UpdateOnce(dt, e.siDead.totalFrames, e.siDead.frameTime);
            continue;
        }
        e.ai.Update(dt, e.pos, g_player.pos);
        e.combat.Update(dt);
        
        e.vel.x = e.ai.GetMoveDirection() * e.ai.GetMoveSpeed();
        e.faceRight = e.ai.IsFacingRight();
        
        if (e.ai.WantsToAttack()) {
            float dist = std::abs(g_player.pos.x - e.pos.x);
            float rng = (e.type == EnemyType::Ranged) ? 250 : 70;
            if (dist <= rng && g_player.combat.stats.IsAlive() && !g_player.combat.IsInvincible()) {
                DamageInfo dmg;
                dmg.amount = e.combat.stats.attack;
                dmg.knockback = Vec2(e.faceRight ? 80.f : -80.f, 20);
                dmg.stunTime = 0.2f;
                g_player.combat.TakeDamage(dmg);
            }
        }
        
        ColResult c = MoveCol(e.pos, e.vel, Vec2(36, 44), dt);
        e.grounded = c.ground;
        
        AIState st = e.ai.GetState();
        if (st == AIState::Attack) { e.anim.Play("attack"); e.anim.UpdateOnce(dt, e.siAtk.totalFrames, e.siAtk.frameTime); }
        else if (st == AIState::Chase) { e.anim.Play("run"); e.anim.Update(dt, e.siRun.totalFrames, e.siRun.frameTime); }
        else { e.anim.Play("idle"); e.anim.Update(dt, e.siIdle.totalFrames, e.siIdle.frameTime); }
    }
}

void CheckCombat() {
    if (g_roomMgr.IsTransitioning()) return;
    if (!g_player.combat.IsAttackActive() || g_player.combat.hitConnected) return;
    
    Vec2 off = g_player.punch.hitboxOffset;
    if (!g_player.faceRight) off.x = -off.x;
    AABB pHit(g_player.pos + off, g_player.punch.hitboxSize * 0.5f);
    
    for (auto& e : g_enemies) {
        if (!e.alive) continue;
        AABB eBox(e.pos, Vec2(20, 22));
        if (pHit.Intersects(eBox)) {
            g_player.combat.hitConnected = true;
            DamageInfo dmg;
            dmg.amount = g_player.punch.baseDamage + g_player.combat.stats.attack;
            dmg.knockback = Vec2(g_player.faceRight ? 120.f : -120.f, 30);
            dmg.stunTime = 0.3f;
            e.combat.TakeDamage(dmg);
            e.ai.OnHurt(0.3f);
            e.vel.x = dmg.knockback.x; e.vel.y = dmg.knockback.y;
            if (!e.combat.stats.IsAlive()) { e.alive = false; e.ai.OnDeath(); e.vel = Vec2(0,0); }
            g_audio->PlaySFX("attack", 0.8f);
            break;
        }
    }
}

void Update(float dt) {
    Engine& eng = Engine::Instance();
    GLFWwindow* w = eng.GetWindow();
    if (dt > 0.1f) dt = 0.1f;
    
    // 过渡更新
    g_roomMgr.UpdateTransition(dt);
    
    // 传送门冷却
    if (g_doorCooldown > 0) g_doorCooldown -= dt;
    
    // 按键
    static bool tabL = 0, rL = 0;
    bool tab = glfwGetKey(w, GLFW_KEY_TAB), r = glfwGetKey(w, GLFW_KEY_R);
    if (tab && !tabL) g_showDebug = !g_showDebug;
    tabL = tab;
    
    // R = 从存档点重生
    if (r && !rL) {
        g_player.combat.stats.FullHeal();
        g_player.combat.state = CombatState::Idle;
        g_roomMgr.TransitionToRoom(g_roomMgr.GetRespawnRoom(), g_roomMgr.GetRespawnPos());
    }
    rL = r;
    
    // 触发器检测（过渡中或冷却中不检测）
    if (!g_roomMgr.IsTransitioning() && g_doorCooldown <= 0) {
        g_nearTrigger = g_roomMgr.CheckTriggers(g_player.pos, Vec2(40, 44));
    } else {
        g_nearTrigger = nullptr;
    }
    
    // W键进入传送门
    bool wKey = glfwGetKey(w, GLFW_KEY_W) || glfwGetKey(w, GLFW_KEY_UP);
    if (wKey && !g_enterPressed && g_nearTrigger && !g_roomMgr.IsTransitioning() && g_doorCooldown <= 0) {
        if (g_nearTrigger->type == TriggerType::Door) {
            g_audio->PlaySFX("transition", 0.6f);
            Room* target = g_roomMgr.GetRoom(g_nearTrigger->targetRoom);
            if (target) {
                target->defaultSpawn = g_nearTrigger->targetSpawn;
                g_roomMgr.TransitionToRoom(g_nearTrigger->targetRoom, g_nearTrigger->targetSpawn);
                g_nearTrigger = nullptr;  // 清空，防止过渡期间误用
            }
        } else if (g_nearTrigger->type == TriggerType::Checkpoint) {
            g_roomMgr.SetRespawn(g_roomMgr.GetCurrentRoom()->name, g_nearTrigger->position);
            std::cout << "[Checkpoint] Respawn set!" << std::endl;
        }
    }
    g_enterPressed = wKey;
    
    // 自动触发存档点（走过即存）
    if (g_nearTrigger && g_nearTrigger->type == TriggerType::Checkpoint) {
        const Room* cr = g_roomMgr.GetCurrentRoom();
        if (cr) g_roomMgr.SetRespawn(cr->name, g_nearTrigger->position);
    }
    
    UpdatePlayer(dt, w);
    UpdateEnemies(dt);
    CheckCombat();
    
    // 摄像机跟随玩家
    if (g_camera) {
        g_camera->SetTarget(g_player.pos);
        g_camera->Update(dt);
    }
    
    // 玩家死亡自动重生
    if (!g_player.combat.stats.IsAlive() && !g_roomMgr.IsTransitioning()) {
        g_player.combat.stats.FullHeal();
        g_player.combat.state = CombatState::Idle;
        g_roomMgr.TransitionToRoom(g_roomMgr.GetRespawnRoom(), g_roomMgr.GetRespawnPos());
    }
}

// ============================================================================
// 渲染
// ============================================================================
void Render() {
    Engine& eng = Engine::Instance();
    int sw = eng.GetWindowWidth(), sh = eng.GetWindowHeight();
    const Room* room = g_roomMgr.GetCurrentRoom();
    
    // 背景色
    Vec4 bg = room ? room->bgColor : Vec4(0.1f, 0.1f, 0.2f, 1);
    glClearColor(bg.x, bg.y, bg.z, bg.w);
    glClear(GL_COLOR_BUFFER_BIT);
    
    Mat4 proj = Mat4::Ortho(0, (float)sw, 0, (float)sh, -1, 1);
    Mat4 view = g_camera ? g_camera->GetViewMatrix() : Mat4::Identity();
    g_spriteRenderer->SetProjection(proj * view);
    g_spriteRenderer->Begin();
    
    // 瓦片地图
    if (g_tilesetTex->IsValid() && g_tilemap) {
        int tw = g_tilemap->GetTileWidth(), th = g_tilemap->GetTileHeight();
        for (int l = 0; l < g_tilemap->GetLayerCount(); l++) {
            const TilemapLayer* layer = g_tilemap->GetLayer(l);
            if (!layer || !layer->visible) continue;
            for (int y = 0; y < g_tilemap->GetHeight(); y++) {
                for (int x = 0; x < g_tilemap->GetWidth(); x++) {
                    int id = layer->GetTile(x, y);
                    if (id == 0) continue;
                    int idx = id-1, col = idx%8, row = idx/8;
                    Vec2 uv0(col/8.f, 1.f-(row+1)/5.f), uv1((col+1)/8.f, 1.f-row/5.f);
                    Sprite tile; tile.SetTexture(g_tilesetTex);
                    tile.SetPosition(Vec2(x*tw+tw*.5f, y*th+th*.5f));
                    tile.SetSize(Vec2((float)tw, (float)th));
                    tile.SetUV(uv0, uv1);
                    g_spriteRenderer->DrawSprite(tile);
                }
            }
        }
    }
    
    // 触发器
    if (room) {
        for (auto& t : room->triggers) {
            if (!t.visible) continue;
            Vec4 c = t.color;
            // 玩家靠近时闪烁
            if (&t == g_nearTrigger) {
                float pulse = std::sin((float)glfwGetTime() * 6.0f) * 0.2f + 0.6f;
                c.w = pulse;
            }
            g_spriteRenderer->DrawRect(t.position, t.size, c);
        }
    }
    
    // 敌人
    for (auto& e : g_enemies) {
        std::shared_ptr<Texture> tex; int c, r; int fr = e.anim.frame;
        if (!e.alive) { tex=e.texDead; c=e.siDead.cols; r=e.siDead.rows; }
        else if (e.anim.cur=="attack") { tex=e.texAtk; c=e.siAtk.cols; r=e.siAtk.rows; }
        else if (e.anim.cur=="run") { tex=e.texRun; c=e.siRun.cols; r=e.siRun.rows; }
        else { tex=e.texIdle; c=e.siIdle.cols; r=e.siIdle.rows; }
        
        float alpha = e.combat.GetRenderAlpha();
        Vec4 col(1,1,1,alpha);
        if (!e.alive) col = Vec4(0.6f,0.6f,0.6f, std::max(0.f, 1.f-e.deathTimer));
        else if (e.combat.IsInvincible()) col = Vec4(1,0.4f,0.4f,alpha);
        DrawChar(tex, c, r, fr, e.pos, e.faceRight, col);
        
        // 血条
        if (e.alive) {
            float hp = e.combat.stats.HealthPercent(), bw = 50, bh = 5;
            Vec2 bp(e.pos.x, e.pos.y + 48);
            g_spriteRenderer->DrawRect(bp, Vec2(bw, bh), Vec4(0.2f,0.2f,0.2f,0.8f));
            Vec4 hc = hp > 0.5f ? Vec4(0,0.8f,0,0.9f) : Vec4(0.9f,0.2f,0,0.9f);
            g_spriteRenderer->DrawRect(Vec2(bp.x-bw*.5f*(1-hp), bp.y), Vec2(bw*hp, bh), hc);
        }
    }
    
    // 玩家
    {
        auto tex = g_pIdleTex; int c=7, r=7;
        if (g_player.anim.cur=="run") { tex=g_pRunTex; c=9; r=6; }
        else if (g_player.anim.cur=="punch") { tex=g_pPunchTex; c=8; r=4; }
        DrawChar(tex, c, r, g_player.anim.frame, g_player.pos, g_player.faceRight,
                Vec4(1,1,1,g_player.combat.GetRenderAlpha()));
    }
    
    // 传送门提示文字（靠近时显示）
    if (g_nearTrigger && g_nearTrigger->type == TriggerType::Door && !g_roomMgr.IsTransitioning()) {
        // 用小矩形做简单的"按W进入"提示
        Vec2 tp(g_nearTrigger->position.x, g_nearTrigger->position.y + 50);
        g_spriteRenderer->DrawRect(tp, Vec2(80, 16), Vec4(0, 0, 0, 0.7f));
        // 箭头指示
        g_spriteRenderer->DrawRect(Vec2(tp.x, tp.y + 14), Vec2(8, 8), Vec4(1, 1, 0, 0.9f));
    }
    
    g_spriteRenderer->End();
    
    // ---- 过渡黑屏 ----
    if (g_roomMgr.GetTransitionAlpha() > 0) {
        g_spriteRenderer->SetProjection(Mat4::Ortho(0, (float)sw, 0, (float)sh, -1, 1));
        g_spriteRenderer->Begin();
        g_spriteRenderer->DrawRect(Vec2(sw*0.5f, sh*0.5f), Vec2((float)sw, (float)sh),
            Vec4(0, 0, 0, g_roomMgr.GetTransitionAlpha()));
        g_spriteRenderer->End();
    }
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui() {
    const Room* room = g_roomMgr.GetCurrentRoom();
    
    ImGui::Begin("Room System", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %.0f", Engine::Instance().GetFPS());
    ImGui::Separator();
    
    if (room) {
        ImGui::Text("Room: %s", room->displayName.c_str());
        ImGui::Text("Size: %dx%d tiles (%dx%d px)",
            room->width, room->height, room->GetPixelWidth(), room->GetPixelHeight());
    }
    
    ImGui::Text("Player: (%.0f, %.0f)", g_player.pos.x, g_player.pos.y);
    float hp = g_player.combat.stats.HealthPercent();
    ImGui::ProgressBar(hp, ImVec2(180, 16), "Player HP");
    
    ImGui::Text("Respawn: %s (%.0f, %.0f)",
        g_roomMgr.GetRespawnRoom().c_str(),
        g_roomMgr.GetRespawnPos().x, g_roomMgr.GetRespawnPos().y);
    
    if (g_roomMgr.IsTransitioning()) {
        ImGui::TextColored(ImVec4(1,1,0,1), "TRANSITIONING... (%.1f)",
            g_roomMgr.GetTransitionAlpha());
    }
    
    if (g_nearTrigger) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f,1,0.5f,1), "Near: %s", g_nearTrigger->label.c_str());
        if (g_nearTrigger->type == TriggerType::Door)
            ImGui::Text("Press W to enter");
        else if (g_nearTrigger->type == TriggerType::Checkpoint)
            ImGui::TextColored(ImVec4(0,1,0,1), "Checkpoint saved!");
    }
    
    ImGui::Separator();
    int alive = (int)std::count_if(g_enemies.begin(), g_enemies.end(), [](const Enemy& e){ return e.alive; });
    ImGui::Text("Enemies: %d/%d alive", alive, (int)g_enemies.size());
    
    ImGui::Checkbox("Debug (Tab)", &g_showDebug);
    ImGui::End();
}

// ============================================================================
// 主函数
// ============================================================================
void Cleanup() {
    if (g_audio) g_audio->Shutdown();
    g_enemies.clear(); g_tilemap.reset();
    g_spriteRenderer.reset(); g_renderer.reset(); g_audio.reset();
}

int main() {
    std::cout << "=== Example 16: Room System ===" << std::endl;
    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth = 1280; cfg.windowHeight = 720;
    cfg.windowTitle = "Mini Engine - Room & Level System";
    if (!engine.Initialize(cfg)) return -1;
    if (!Initialize()) { engine.Shutdown(); return -1; }
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    Cleanup(); engine.Shutdown();
    return 0;
}
