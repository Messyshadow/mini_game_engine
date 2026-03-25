/**
 * 第17章：UI系统与进阶玩法 (UI System & Advanced Gameplay)
 *
 * 新增内容：
 * 1. HUD — HP血条 + MP能量条（左下角）、伤害飘字、房间名提示
 * 2. 玩家新能力 — 二段跳、抓墙跳、瞬移到敌人身后（消耗MP）
 * 3. 混合型敌人（robot6）— 远距离射击 + 近距离近战
 * 4. 暂停菜单（ESC）
 * 5. 更丰富的音效
 *
 * 操作：
 * - A/D: 移动 | Space: 跳跃（支持二段跳、贴墙可墙跳）
 * - J: 攻击 | K: 瞬移到最近敌人身后（消耗20MP）
 * - W/↑: 进入传送门 | ESC: 暂停
 * - Tab: 调试 | R: 重生
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
#include "ui/HUD.h"
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
// 动画 & 敌人结构
// ============================================================================
struct SheetInfo { int cols, rows, totalFrames; float frameTime; };
struct AnimCtrl {
    std::string cur = "idle"; int frame = 0; float timer = 0;
    void Play(const std::string& n) { if (cur != n) { cur = n; frame = 0; timer = 0; } }
    void Update(float dt, int max, float ft) { timer += dt; if (timer >= ft) { timer = 0; frame = (frame+1) % max; } }
    void UpdateOnce(float dt, int max, float ft) { timer += dt; if (timer >= ft) { timer = 0; frame++; if (frame >= max) frame = max-1; } }
};

struct Enemy {
    Vec2 pos, vel = Vec2(0,0);
    bool faceRight = false, grounded = false, alive = true;
    EnemyType type; EnemyAI ai; CombatComponent combat; AnimCtrl anim;
    std::shared_ptr<Texture> texIdle, texRun, texAtk, texShoot, texDead;
    SheetInfo siIdle, siRun, siAtk, siShoot, siDead;
    float deathTimer = 0;
};

// ============================================================================
// 全局变量
// ============================================================================
std::unique_ptr<Renderer2D> g_renderer;
std::unique_ptr<SpriteRenderer> g_spriteRenderer;
std::unique_ptr<Tilemap> g_tilemap;
std::unique_ptr<AudioSystem> g_audio;
std::unique_ptr<Camera2D> g_camera;
RoomManager g_roomMgr;
HUD g_hud;

// 纹理
std::shared_ptr<Texture> g_tilesetTex;
std::shared_ptr<Texture> g_pIdleTex, g_pRunTex, g_pPunchTex;
std::shared_ptr<Texture> g_r4Idle, g_r4Run, g_r4Punch, g_r4Dead;
std::shared_ptr<Texture> g_r6Idle, g_r6Run, g_r6PunchL, g_r6PunchR, g_r6Dead;
std::shared_ptr<Texture> g_r8Idle, g_r8Run, g_r8Shoot, g_r8Dead;

// 玩家
struct PlayerData {
    Vec2 pos = Vec2(100, 86), vel = Vec2(0, 0);
    bool faceRight = true, grounded = false;
    CombatComponent combat; AttackData punch; AnimCtrl anim;
    float speed = 250, jumpForce = 520;
    
    // MP系统
    int mp = 50, maxMP = 50;
    float mpRegen = 5.0f;  // 每秒回复
    
    // 二段跳
    int jumpCount = 0;
    int maxJumps = 2;
    
    // 墙跳
    bool touchingWallL = false, touchingWallR = false;
    float wallSlideSpeed = -60.0f;
    
    // 瞬移
    float teleportCooldown = 0;
    float teleportMaxCD = 3.0f;
    int teleportMPCost = 20;
} g_player;

std::vector<Enemy> g_enemies;
bool g_showDebug = false;
bool g_paused = false;
Trigger* g_nearTrigger = nullptr;
bool g_enterPressed = false;
float g_doorCooldown = 0;

const float GRAVITY = -1200.f;

// ============================================================================
// 辅助函数
// ============================================================================
void CalcUV(int idx, int c, int r, Vec2& mn, Vec2& mx) {
    int col = idx%c, row = idx/c;
    mn = Vec2(col/(float)c, 1.f-(row+1)/(float)r);
    mx = Vec2((col+1)/(float)c, 1.f-row/(float)r);
}

bool LoadTex(std::shared_ptr<Texture>& t, const char* n) {
    t = std::make_shared<Texture>();
    std::string p[] = {std::string("data/texture/")+n, std::string("../data/texture/")+n, std::string("../../data/texture/")+n};
    for (auto& s : p) if (t->Load(s)) return true;
    t->CreateSolidColor(Vec4(1,0,1,1), 64, 64); return false;
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

void DrawChar(std::shared_ptr<Texture> tex, int c, int r, int fr, Vec2 pos, bool fR, Vec4 col = Vec4(1,1,1,1), float sz = 120) {
    Vec2 mn, mx; CalcUV(fr, c, r, mn, mx);
    if (!fR) std::swap(mn.x, mx.x);
    Sprite s; s.SetTexture(tex); s.SetPosition(pos); s.SetSize(Vec2(sz,sz)); s.SetUV(mn,mx); s.SetColor(col);
    g_spriteRenderer->DrawSprite(s);
}

// ============================================================================
// 瞬移到最近敌人身后
// ============================================================================
void TeleportBehindEnemy() {
    if (g_player.teleportCooldown > 0 || g_player.mp < g_player.teleportMPCost) return;
    
    // 找最近的活着的敌人
    Enemy* nearest = nullptr;
    float minDist = 99999;
    for (auto& e : g_enemies) {
        if (!e.alive) continue;
        float d = std::abs(e.pos.x - g_player.pos.x) + std::abs(e.pos.y - g_player.pos.y);
        if (d < minDist) { minDist = d; nearest = &e; }
    }
    
    if (!nearest || minDist > 400) return;  // 太远了不能瞬移
    
    // 瞬移到敌人身后
    float behindOffset = nearest->faceRight ? -70.0f : 70.0f;
    g_player.pos.x = nearest->pos.x + behindOffset;
    g_player.pos.y = nearest->pos.y;
    g_player.faceRight = (nearest->pos.x > g_player.pos.x);
    g_player.vel = Vec2(0, 0);
    
    g_player.mp -= g_player.teleportMPCost;
    g_player.teleportCooldown = g_player.teleportMaxCD;
    
    g_audio->PlaySFX("transition", 0.5f);
}

// ============================================================================
// 构建房间
// ============================================================================
void BuildRoom(Room* room) {
    if (!room) return;
    g_tilemap = std::make_unique<Tilemap>();
    g_tilemap->Create(room->width, room->height, room->tileSize);
    int ground = g_tilemap->AddLayer("ground", true);
    for (auto& p : room->platforms)
        for (int dy = 0; dy < p.height; dy++)
            for (int dx = 0; dx < p.width; dx++)
                g_tilemap->SetTile(ground, p.x+dx, p.y+dy, p.tileID);
    
    g_enemies.clear();
    for (auto& sp : room->enemySpawns) {
        Enemy e; e.pos = sp.position; e.type = sp.type;
        if (sp.type == EnemyType::Melee) {
            e.texIdle=g_r4Idle; e.siIdle={7,7,14,0.12f};
            e.texRun=g_r4Run;   e.siRun={7,6,12,0.08f};
            e.texAtk=g_r4Punch; e.siAtk={8,4,10,0.05f};
            e.texDead=g_r4Dead; e.siDead={10,3,15,0.08f};
        } else if (sp.type == EnemyType::Hybrid) {
            e.texIdle=g_r6Idle;    e.siIdle={9,6,16,0.1f};
            e.texRun=g_r6Run;      e.siRun={7,6,12,0.08f};
            e.texAtk=g_r6PunchR;   e.siAtk={8,4,10,0.05f};  // 近战用punch
            e.texShoot=g_r6PunchL; e.siShoot={7,3,10,0.06f}; // 远程用另一拳
            e.texDead=g_r6Dead;    e.siDead={8,4,15,0.08f};
        } else {
            e.texIdle=g_r8Idle;  e.siIdle={9,6,16,0.1f};
            e.texRun=g_r8Run;    e.siRun={7,6,12,0.08f};
            e.texAtk=g_r8Shoot;  e.siAtk={8,4,10,0.06f};
            e.texDead=g_r8Dead;  e.siDead={7,4,14,0.08f};
        }
        AIConfig cfg; cfg.type = sp.type;
        cfg.patrolMinX = sp.patrolMinX; cfg.patrolMaxX = sp.patrolMaxX;
        if (sp.type == EnemyType::Ranged) {
            cfg.attackRange=250; cfg.preferredDistance=200; cfg.retreatDistance=100;
            cfg.chaseSpeed=120; cfg.detectionRange=300; cfg.loseRange=400;
        } else if (sp.type == EnemyType::Hybrid) {
            cfg.attackRange=200; cfg.meleeRange=80; cfg.chaseSpeed=150;
            cfg.detectionRange=280; cfg.loseRange=380; cfg.attackCooldown=1.0f;
        } else {
            cfg.attackRange=65; cfg.chaseSpeed=160; cfg.detectionRange=220; cfg.loseRange=320;
        }
        e.ai.SetConfig(cfg);
        e.combat.stats.maxHealth = sp.health; e.combat.stats.currentHealth = sp.health;
        e.combat.stats.attack = sp.attack; e.combat.stats.defense = 1;
        g_enemies.push_back(e);
    }
}

// ============================================================================
// 房间定义
// ============================================================================
void DefineRooms() {
    // 房间1：训练场
    {
        Room r; r.name = "training"; r.displayName = "Training Ground";
        r.width = 35; r.height = 15; r.bgColor = Vec4(0.1f,0.12f,0.22f,1);
        r.defaultSpawn = Vec2(80, 86); r.bgmName = "bgm";
        r.AddGround(0, 35); r.AddWalls(10);
        r.AddPlatform(8, 4, 6); r.AddPlatform(20, 5, 5); r.AddPlatform(14, 8, 4);
        r.AddEnemy(EnemyType::Melee, Vec2(350, 86), 250, 500, 30, 6);
        r.AddCheckpoint(Vec2(80, 86));
        r.AddDoor(Vec2(r.GetPixelWidth()-60, 86), "dungeon", Vec2(80, 86), "Dungeon ->");
        g_roomMgr.AddRoom(r);
    }
    // 房间2：地牢
    {
        Room r; r.name = "dungeon"; r.displayName = "Dark Dungeon";
        r.width = 50; r.height = 15; r.bgColor = Vec4(0.08f,0.06f,0.12f,1);
        r.defaultSpawn = Vec2(80, 86); r.bgmName = "bgm2";
        r.AddGround(0, 50); r.AddWalls(10);
        r.AddPlatform(8, 4, 8); r.AddPlatform(22, 5, 6); r.AddPlatform(34, 4, 10);
        r.AddPlatform(15, 8, 5);
        r.AddEnemy(EnemyType::Melee, Vec2(300, 86), 200, 500, 50, 10);
        r.AddEnemy(EnemyType::Hybrid, Vec2(600, 86), 500, 800, 60, 12);
        r.AddEnemy(EnemyType::Ranged, Vec2(1000, 86), 850, 1200, 35, 14);
        r.AddCheckpoint(Vec2(500, 86));
        r.AddDoor(Vec2(60, 86), "training", Vec2(1040, 86), "<- Training");
        r.AddDoor(Vec2(r.GetPixelWidth()-60, 86), "boss", Vec2(80, 86), "BOSS ->");
        g_roomMgr.AddRoom(r);
    }
    // 房间3：BOSS间
    {
        Room r; r.name = "boss"; r.displayName = "BOSS Chamber";
        r.width = 40; r.height = 15; r.bgColor = Vec4(0.15f,0.05f,0.05f,1);
        r.defaultSpawn = Vec2(80, 86); r.bgmName = "bgm2";
        r.AddGround(0, 40); r.AddWalls(10);
        r.AddPlatform(12, 4, 6); r.AddPlatform(24, 5, 8); r.AddPlatform(8, 8, 4);
        r.AddEnemy(EnemyType::Hybrid, Vec2(700, 86), 400, 1000, 180, 20);
        r.AddEnemy(EnemyType::Ranged, Vec2(500, 86), 350, 700, 60, 15);
        r.AddEnemy(EnemyType::Melee, Vec2(900, 86), 700, 1100, 80, 14);
        r.AddDoor(Vec2(60, 86), "dungeon", Vec2(1520, 86), "<- Dungeon");
        g_roomMgr.AddRoom(r);
    }
}

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    Engine& eng = Engine::Instance();
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) return false;
    g_spriteRenderer = std::make_unique<SpriteRenderer>();
    if (!g_spriteRenderer->Initialize()) return false;
    
    g_tilesetTex = std::make_shared<Texture>();
    const char* tp[] = {"data/texture/tileset.png","../data/texture/tileset.png"};
    for (auto p : tp) if (g_tilesetTex->Load(p)) break;
    
    LoadTex(g_pIdleTex,"robot3/robot3_idle.png");
    LoadTex(g_pRunTex,"robot3/robot3_run.png");
    LoadTex(g_pPunchTex,"robot3/robot3_punch.png");
    LoadTex(g_r4Idle,"robot4/robot4_idle.png"); LoadTex(g_r4Run,"robot4/robot4_run.png");
    LoadTex(g_r4Punch,"robot4/robot4_punch_r.png"); LoadTex(g_r4Dead,"robot4/robot4_dead.png");
    LoadTex(g_r6Idle,"robot6/robot6_idle.png"); LoadTex(g_r6Run,"robot6/robot6_run.png");
    LoadTex(g_r6PunchL,"robot6/robot6_punch_l.png"); LoadTex(g_r6PunchR,"robot6/robot6_punch_r.png");
    LoadTex(g_r6Dead,"robot6/robot6_dead.png");
    LoadTex(g_r8Idle,"robot8/robot8_idle.png"); LoadTex(g_r8Run,"robot8/robot8_run.png");
    LoadTex(g_r8Shoot,"robot8/robot8_shoot.png"); LoadTex(g_r8Dead,"robot8/robot8_dead.png");
    
    g_camera = std::make_unique<Camera2D>();
    g_camera->SetViewportSize((float)eng.GetWindowWidth(), (float)eng.GetWindowHeight());
    g_camera->SetSmoothSpeed(8.0f);
    
    g_audio = std::make_unique<AudioSystem>();
    g_audio->Initialize();
    const char* bp[] = {"data/","../data/","../../data/"};
    for (auto b : bp) if (g_audio->LoadSound("attack", std::string(b)+"audio/sfx/Sound_Axe.wav")) break;
    for (auto b : bp) if (g_audio->LoadSound("sword", std::string(b)+"audio/sfx/sword_slash.mp3")) break;
    for (auto b : bp) if (g_audio->LoadSound("explosion", std::string(b)+"audio/sfx/explosion.mp3")) break;
    for (auto b : bp) if (g_audio->LoadSound("transition", std::string(b)+"audio/sfx/transition.mp3")) break;
    for (auto b : bp) if (g_audio->LoadSound("ui_click", std::string(b)+"audio/sfx/ui_click.mp3")) break;
    for (auto b : bp) if (g_audio->LoadMusic("bgm", std::string(b)+"audio/bgm/windaswarriors.mp3")) break;
    for (auto b : bp) if (g_audio->LoadMusic("bgm2", std::string(b)+"audio/bgm/vivo.mp3")) break;
    
    // 玩家攻击
    g_player.punch.name = "punch"; g_player.punch.baseDamage = 15;
    g_player.punch.startupFrames = 0.05f; g_player.punch.activeFrames = 0.15f;
    g_player.punch.recoveryFrames = 0.1f; g_player.punch.knockback = Vec2(120,30);
    g_player.punch.stunTime = 0.3f; g_player.punch.hitboxOffset = Vec2(55,0);
    g_player.punch.hitboxSize = Vec2(70,50); g_player.punch.animFrameCount = 8;
    g_player.punch.animFPS = 20;
    g_player.combat.stats = {100, 100, 10, 5, 0.1f, 1.5f};
    g_player.combat.onAttackStart = [](const std::string&) { g_audio->PlaySFX("sword", 0.7f); };
    
    DefineRooms();
    g_roomMgr.onRoomLoaded = [](Room* room) {
        BuildRoom(room);
        g_player.pos = room->defaultSpawn; g_player.vel = Vec2(0,0);
        g_doorCooldown = 1.0f; g_enterPressed = true;
        if (g_camera) { g_camera->SetTarget(g_player.pos); g_camera->SnapToTarget();
            g_camera->SetWorldBounds(0, 0, (float)room->GetPixelWidth(), (float)room->GetPixelHeight()); }
        if (!room->bgmName.empty()) g_audio->PlayMusic(room->bgmName, 0.3f);
        g_hud.roomTitle.Show(room->displayName);
    };
    g_roomMgr.SetRespawn("training", Vec2(80, 86));
    g_roomMgr.LoadRoom("training", Vec2(80, 86));
    
    std::cout << "\n=== Chapter 17: UI System & Advanced Gameplay ===" << std::endl;
    std::cout << "A/D: Move | Space: Jump(x2) | J: Attack | K: Teleport" << std::endl;
    std::cout << "W: Door | ESC: Pause | Tab: Debug | R: Respawn" << std::endl;
    return true;
}

// ============================================================================
// 玩家更新
// ============================================================================
void UpdatePlayer(float dt, GLFWwindow* w) {
    if (g_roomMgr.IsTransitioning() || g_paused) { g_player.vel.x = 0; return; }
    g_player.combat.Update(dt);
    
    // MP回复
    if (g_player.mp < g_player.maxMP) {
        g_player.mp += (int)(g_player.mpRegen * dt);
        if (g_player.mp > g_player.maxMP) g_player.mp = g_player.maxMP;
    }
    if (g_player.teleportCooldown > 0) g_player.teleportCooldown -= dt;
    
    // 移动
    float mv = 0;
    if (g_player.combat.CanAct()) {
        if (glfwGetKey(w, GLFW_KEY_A)) mv = -1;
        if (glfwGetKey(w, GLFW_KEY_D)) mv = 1;
        if (mv != 0) g_player.faceRight = mv > 0;
        g_player.vel.x = mv * g_player.speed;
    } else g_player.vel.x *= 0.9f;
    
    // 跳跃（二段跳 + 墙跳）
    static bool spL = 0;
    bool sp = glfwGetKey(w, GLFW_KEY_SPACE);
    if (sp && !spL && g_player.combat.CanAct()) {
        if (g_player.grounded) {
            g_player.vel.y = g_player.jumpForce; g_player.grounded = false;
            g_player.jumpCount = 1;
        } else if (g_player.touchingWallL || g_player.touchingWallR) {
            // 墙跳
            g_player.vel.y = g_player.jumpForce * 0.9f;
            g_player.vel.x = g_player.touchingWallL ? 200.f : -200.f;
            g_player.faceRight = g_player.touchingWallL;
            g_player.jumpCount = 1;
        } else if (g_player.jumpCount < g_player.maxJumps) {
            // 二段跳
            g_player.vel.y = g_player.jumpForce * 0.85f;
            g_player.jumpCount++;
        }
    }
    spL = sp;
    
    // 攻击
    static bool jL = 0;
    bool j = glfwGetKey(w, GLFW_KEY_J);
    if (j && !jL) g_player.combat.StartAttack(g_player.punch);
    jL = j;
    
    // 瞬移 (K键)
    static bool kL = 0;
    bool k = glfwGetKey(w, GLFW_KEY_K);
    if (k && !kL) TeleportBehindEnemy();
    kL = k;
    
    // 墙面滑行
    bool wasOnWall = g_player.touchingWallL || g_player.touchingWallR;
    
    ColResult c = MoveCol(g_player.pos, g_player.vel, Vec2(40, 44), dt);
    g_player.grounded = c.ground;
    g_player.touchingWallL = c.wL && !g_player.grounded;
    g_player.touchingWallR = c.wR && !g_player.grounded;
    
    if (g_player.grounded) g_player.jumpCount = 0;
    
    // 贴墙减速下落
    if ((g_player.touchingWallL || g_player.touchingWallR) && g_player.vel.y < g_player.wallSlideSpeed) {
        g_player.vel.y = g_player.wallSlideSpeed;
    }
    
    // 动画
    if (g_player.combat.state == CombatState::Attacking) {
        g_player.anim.Play("punch"); g_player.anim.UpdateOnce(dt, 8, 0.05f);
    } else if (std::abs(g_player.vel.x) > 10) {
        g_player.anim.Play("run"); g_player.anim.Update(dt, 9, 0.08f);
    } else {
        g_player.anim.Play("idle"); g_player.anim.Update(dt, 7, 0.1f);
    }
}

// ============================================================================
// 敌人更新
// ============================================================================
void UpdateEnemies(float dt) {
    if (g_roomMgr.IsTransitioning() || g_paused) return;
    for (auto& e : g_enemies) {
        if (!e.alive) { e.deathTimer += dt; e.anim.Play("dead"); e.anim.UpdateOnce(dt, e.siDead.totalFrames, e.siDead.frameTime); continue; }
        e.ai.Update(dt, e.pos, g_player.pos); e.combat.Update(dt);
        e.vel.x = e.ai.GetMoveDirection() * e.ai.GetMoveSpeed();
        e.faceRight = e.ai.IsFacingRight();
        
        if (e.ai.WantsToAttack()) {
            float dist = std::abs(g_player.pos.x - e.pos.x);
            bool isRanged = e.ai.IsRangedAttack();
            float rng = isRanged ? 250 : 70;
            if (dist <= rng && g_player.combat.stats.IsAlive() && !g_player.combat.IsInvincible()) {
                DamageInfo dmg; dmg.amount = e.combat.stats.attack;
                dmg.knockback = Vec2(e.faceRight ? 80.f : -80.f, 20);
                dmg.stunTime = 0.2f;
                g_player.combat.TakeDamage(dmg);
                g_hud.AddDamagePopup(g_player.pos, dmg.amount);
                if (isRanged) g_audio->PlaySFX("explosion", 0.4f);
            }
        }
        
        ColResult c = MoveCol(e.pos, e.vel, Vec2(36,44), dt);
        e.grounded = c.ground;
        
        AIState st = e.ai.GetState();
        if (st == AIState::Attack) {
            // 混合型根据攻击类型选不同动画
            if (e.type == EnemyType::Hybrid && e.ai.IsRangedAttack() && e.texShoot) {
                e.anim.Play("shoot"); e.anim.UpdateOnce(dt, e.siShoot.totalFrames, e.siShoot.frameTime);
            } else {
                e.anim.Play("attack"); e.anim.UpdateOnce(dt, e.siAtk.totalFrames, e.siAtk.frameTime);
            }
        } else if (st == AIState::Chase) { e.anim.Play("run"); e.anim.Update(dt, e.siRun.totalFrames, e.siRun.frameTime); }
        else { e.anim.Play("idle"); e.anim.Update(dt, e.siIdle.totalFrames, e.siIdle.frameTime); }
    }
}

// ============================================================================
// 战斗检测
// ============================================================================
void CheckCombat() {
    if (g_roomMgr.IsTransitioning() || g_paused) return;
    if (!g_player.combat.IsAttackActive() || g_player.combat.hitConnected) return;
    Vec2 off = g_player.punch.hitboxOffset;
    if (!g_player.faceRight) off.x = -off.x;
    AABB pHit(g_player.pos + off, g_player.punch.hitboxSize * 0.5f);
    for (auto& e : g_enemies) {
        if (!e.alive) continue;
        AABB eBox(e.pos, Vec2(20,22));
        if (pHit.Intersects(eBox)) {
            g_player.combat.hitConnected = true;
            int dmgAmt = g_player.punch.baseDamage + g_player.combat.stats.attack;
            DamageInfo dmg; dmg.amount = dmgAmt;
            dmg.knockback = Vec2(g_player.faceRight ? 120.f : -120.f, 30);
            dmg.stunTime = 0.3f;
            e.combat.TakeDamage(dmg); e.ai.OnHurt(0.3f);
            e.vel.x = dmg.knockback.x; e.vel.y = dmg.knockback.y;
            g_hud.AddDamagePopup(e.pos, dmgAmt);
            if (!e.combat.stats.IsAlive()) { e.alive = false; e.ai.OnDeath(); e.vel = Vec2(0,0); }
            g_audio->PlaySFX("attack", 0.8f);
            break;
        }
    }
}

// ============================================================================
// 主更新
// ============================================================================
void Update(float dt) {
    Engine& eng = Engine::Instance();
    GLFWwindow* w = eng.GetWindow();
    if (dt > 0.1f) dt = 0.1f;
    
    // ESC暂停
    static bool escL = 0;
    bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE);
    if (esc && !escL) { g_paused = !g_paused; g_audio->PlaySFX("ui_click", 0.5f); }
    escL = esc;
    
    if (g_paused) { g_hud.isPaused = true; return; }
    g_hud.isPaused = false;
    
    g_roomMgr.UpdateTransition(dt);
    g_hud.Update(dt);
    if (g_doorCooldown > 0) g_doorCooldown -= dt;
    
    static bool tabL = 0, rL = 0;
    bool tab = glfwGetKey(w, GLFW_KEY_TAB), r = glfwGetKey(w, GLFW_KEY_R);
    if (tab && !tabL) g_showDebug = !g_showDebug;
    tabL = tab;
    if (r && !rL) {
        g_player.combat.stats.FullHeal(); g_player.mp = g_player.maxMP;
        g_player.combat.state = CombatState::Idle;
        g_roomMgr.TransitionToRoom(g_roomMgr.GetRespawnRoom(), g_roomMgr.GetRespawnPos());
    }
    rL = r;
    
    // 触发器
    if (!g_roomMgr.IsTransitioning() && g_doorCooldown <= 0)
        g_nearTrigger = g_roomMgr.CheckTriggers(g_player.pos, Vec2(40,44));
    else g_nearTrigger = nullptr;
    
    bool wKey = glfwGetKey(w, GLFW_KEY_W) || glfwGetKey(w, GLFW_KEY_UP);
    if (wKey && !g_enterPressed && g_nearTrigger && !g_roomMgr.IsTransitioning() && g_doorCooldown <= 0) {
        if (g_nearTrigger->type == TriggerType::Door) {
            g_audio->PlaySFX("transition", 0.6f);
            Room* tgt = g_roomMgr.GetRoom(g_nearTrigger->targetRoom);
            if (tgt) { tgt->defaultSpawn = g_nearTrigger->targetSpawn;
                g_roomMgr.TransitionToRoom(g_nearTrigger->targetRoom, g_nearTrigger->targetSpawn);
                g_nearTrigger = nullptr; }
        } else if (g_nearTrigger->type == TriggerType::Checkpoint) {
            g_roomMgr.SetRespawn(g_roomMgr.GetCurrentRoom()->name, g_nearTrigger->position);
        }
    }
    g_enterPressed = wKey;
    if (g_nearTrigger && g_nearTrigger->type == TriggerType::Checkpoint) {
        const Room* cr = g_roomMgr.GetCurrentRoom();
        if (cr) g_roomMgr.SetRespawn(cr->name, g_nearTrigger->position);
    }
    
    UpdatePlayer(dt, w); UpdateEnemies(dt); CheckCombat();
    
    // 摄像机
    if (g_camera) { g_camera->SetTarget(g_player.pos); g_camera->Update(dt); }
    
    // 更新HUD数据
    g_hud.playerHP = g_player.combat.stats.currentHealth;
    g_hud.playerMaxHP = g_player.combat.stats.maxHealth;
    g_hud.playerMP = g_player.mp;
    g_hud.playerMaxMP = g_player.maxMP;
    g_hud.teleportCooldown = g_player.teleportCooldown;
    g_hud.teleportMaxCooldown = g_player.teleportMaxCD;
    
    // 玩家死亡
    if (!g_player.combat.stats.IsAlive() && !g_roomMgr.IsTransitioning()) {
        g_player.combat.stats.FullHeal(); g_player.mp = g_player.maxMP;
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
    Vec4 bg = room ? room->bgColor : Vec4(0.1f,0.1f,0.2f,1);
    glClearColor(bg.x, bg.y, bg.z, bg.w);
    glClear(GL_COLOR_BUFFER_BIT);
    
    Mat4 proj = Mat4::Ortho(0, (float)sw, 0, (float)sh, -1, 1);
    Mat4 view = g_camera ? g_camera->GetViewMatrix() : Mat4::Identity();
    Mat4 pv = proj * view;
    g_spriteRenderer->SetProjection(pv);
    g_spriteRenderer->Begin();
    
    // 瓦片
    if (g_tilesetTex->IsValid() && g_tilemap) {
        int tw = g_tilemap->GetTileWidth(), th = g_tilemap->GetTileHeight();
        for (int l = 0; l < g_tilemap->GetLayerCount(); l++) {
            const TilemapLayer* layer = g_tilemap->GetLayer(l);
            if (!layer || !layer->visible) continue;
            for (int y = 0; y < g_tilemap->GetHeight(); y++)
                for (int x = 0; x < g_tilemap->GetWidth(); x++) {
                    int id = layer->GetTile(x,y); if (id==0) continue;
                    int idx=id-1, col=idx%8, row=idx/8;
                    Vec2 u0(col/8.f,1.f-(row+1)/5.f), u1((col+1)/8.f,1.f-row/5.f);
                    Sprite t; t.SetTexture(g_tilesetTex);
                    t.SetPosition(Vec2(x*tw+tw*.5f, y*th+th*.5f));
                    t.SetSize(Vec2((float)tw,(float)th)); t.SetUV(u0,u1);
                    g_spriteRenderer->DrawSprite(t);
                }
        }
    }
    
    // 触发器
    if (room) for (auto& t : room->triggers) {
        if (!t.visible) continue;
        Vec4 c = t.color;
        if (&t == g_nearTrigger) { float p = std::sin((float)glfwGetTime()*6)*0.2f+0.6f; c.w = p; }
        g_spriteRenderer->DrawRect(t.position, t.size, c);
    }
    
    // 敌人
    for (auto& e : g_enemies) {
        std::shared_ptr<Texture> tex; int c,r; int fr = e.anim.frame;
        if (!e.alive) { tex=e.texDead; c=e.siDead.cols; r=e.siDead.rows; }
        else if (e.anim.cur=="shoot" && e.texShoot) { tex=e.texShoot; c=e.siShoot.cols; r=e.siShoot.rows; }
        else if (e.anim.cur=="attack") { tex=e.texAtk; c=e.siAtk.cols; r=e.siAtk.rows; }
        else if (e.anim.cur=="run") { tex=e.texRun; c=e.siRun.cols; r=e.siRun.rows; }
        else { tex=e.texIdle; c=e.siIdle.cols; r=e.siIdle.rows; }
        float a = e.combat.GetRenderAlpha();
        Vec4 col(1,1,1,a);
        if (!e.alive) col = Vec4(0.6f,0.6f,0.6f, std::max(0.f,1.f-e.deathTimer));
        else if (e.combat.IsInvincible()) col = Vec4(1,0.4f,0.4f,a);
        DrawChar(tex, c, r, fr, e.pos, e.faceRight, col);
        if (e.alive) {
            float hp = e.combat.stats.HealthPercent(), bw=50, bh=5;
            Vec2 bp(e.pos.x, e.pos.y+48);
            g_spriteRenderer->DrawRect(bp, Vec2(bw,bh), Vec4(0.2f,0.2f,0.2f,0.8f));
            Vec4 hc = hp>0.5f ? Vec4(0,0.8f,0,0.9f) : Vec4(0.9f,0.2f,0,0.9f);
            g_spriteRenderer->DrawRect(Vec2(bp.x-bw*.5f*(1-hp),bp.y), Vec2(bw*hp,bh), hc);
        }
    }
    
    // 玩家
    {
        auto tex = g_pIdleTex; int c=7,r=7;
        if (g_player.anim.cur=="run") { tex=g_pRunTex; c=9; r=6; }
        else if (g_player.anim.cur=="punch") { tex=g_pPunchTex; c=8; r=4; }
        DrawChar(tex,c,r,g_player.anim.frame, g_player.pos, g_player.faceRight,
                Vec4(1,1,1,g_player.combat.GetRenderAlpha()));
    }
    
    g_spriteRenderer->End();
    
    // ---- HUD（屏幕空间，不跟随摄像机）----
    g_hud.Render(g_spriteRenderer.get(), sw, sh, pv);
    
    // ---- 过渡黑屏 ----
    if (g_roomMgr.GetTransitionAlpha() > 0) {
        g_spriteRenderer->SetProjection(Mat4::Ortho(0,(float)sw,0,(float)sh,-1,1));
        g_spriteRenderer->Begin();
        g_spriteRenderer->DrawRect(Vec2(sw*.5f,sh*.5f), Vec2((float)sw,(float)sh),
            Vec4(0,0,0,g_roomMgr.GetTransitionAlpha()));
        g_spriteRenderer->End();
    }
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui() {
    if (!g_showDebug) return;
    const Room* room = g_roomMgr.GetCurrentRoom();
    ImGui::Begin("Ch17 Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %.0f", Engine::Instance().GetFPS());
    if (room) ImGui::Text("Room: %s (%dx%d)", room->displayName.c_str(), room->width, room->height);
    ImGui::Separator();
    ImGui::Text("Player: (%.0f, %.0f) Grounded:%s", g_player.pos.x, g_player.pos.y, g_player.grounded?"Y":"N");
    ImGui::Text("HP: %d/%d  MP: %d/%d", g_player.combat.stats.currentHealth, g_player.combat.stats.maxHealth, g_player.mp, g_player.maxMP);
    ImGui::Text("Jumps: %d/%d  WallL:%s WallR:%s", g_player.jumpCount, g_player.maxJumps,
        g_player.touchingWallL?"Y":"N", g_player.touchingWallR?"Y":"N");
    ImGui::Text("Teleport CD: %.1f", g_player.teleportCooldown);
    ImGui::Separator();
    int alive = (int)std::count_if(g_enemies.begin(), g_enemies.end(), [](const Enemy& e){return e.alive;});
    ImGui::Text("Enemies: %d/%d", alive, (int)g_enemies.size());
    for (int i = 0; i < (int)g_enemies.size(); i++) {
        auto& e = g_enemies[i];
        const char* tp = e.type==EnemyType::Melee?"Melee":e.type==EnemyType::Hybrid?"Hybrid":"Ranged";
        ImGui::Text("[%d] %s %s HP:%d/%d", i, tp, e.ai.GetStateName().c_str(),
            e.combat.stats.currentHealth, e.combat.stats.maxHealth);
    }
    ImGui::End();
}

// ============================================================================
// 清理 & 主函数
// ============================================================================
void Cleanup() {
    if (g_audio) g_audio->Shutdown();
    g_enemies.clear(); g_tilemap.reset(); g_camera.reset();
    g_spriteRenderer.reset(); g_renderer.reset(); g_audio.reset();
    // 释放所有纹理（修复GLFW退出警告）
    g_tilesetTex.reset(); g_pIdleTex.reset(); g_pRunTex.reset(); g_pPunchTex.reset();
    g_r4Idle.reset(); g_r4Run.reset(); g_r4Punch.reset(); g_r4Dead.reset();
    g_r6Idle.reset(); g_r6Run.reset(); g_r6PunchL.reset(); g_r6PunchR.reset(); g_r6Dead.reset();
    g_r8Idle.reset(); g_r8Run.reset(); g_r8Shoot.reset(); g_r8Dead.reset();
}

int main() {
    std::cout << "=== Example 17: UI System ===" << std::endl;
    Engine& engine = Engine::Instance();
    EngineConfig cfg; cfg.windowWidth=1280; cfg.windowHeight=720;
    cfg.windowTitle = "Mini Engine - UI & Advanced Gameplay";
    if (!engine.Initialize(cfg)) return -1;
    if (!Initialize()) { engine.Shutdown(); return -1; }
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    Cleanup(); engine.Shutdown();
    return 0;
}
