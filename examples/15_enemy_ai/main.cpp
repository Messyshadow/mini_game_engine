/**
 * 第15章：敌人AI系统 (Enemy AI)
 * 
 * 学习内容：
 * 1. 有限状态机（FSM）驱动的敌人行为
 * 2. 巡逻、追击、攻击三大核心状态
 * 3. 近战型 vs 远程型敌人
 * 4. AI与战斗系统、动画系统的整合
 * 
 * 操作：
 * - A/D: 移动
 * - Space: 跳跃
 * - J: 攻击
 * - Tab: 显示碰撞框和AI感知范围
 * - R: 重置所有敌人
 * - 1/2/3: 切换关注的敌人（ImGui显示其详情）
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
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <cmath>
#include <vector>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// Spritesheet帧信息
// ============================================================================
struct SheetInfo {
    int cols, rows, totalFrames;
    float frameTime;
};

// ============================================================================
// 动画控制器（简化版，适用于本章）
// ============================================================================
struct AnimController {
    std::string currentAnim = "idle";
    int frame = 0;
    float timer = 0;
    
    void Play(const std::string& name) {
        if (currentAnim != name) { currentAnim = name; frame = 0; timer = 0; }
    }
    
    void Update(float dt, int maxFrames, float frameTime) {
        timer += dt;
        if (timer >= frameTime) {
            timer = 0;
            frame = (frame + 1) % maxFrames;
        }
    }
    
    void UpdateOnce(float dt, int maxFrames, float frameTime) {
        timer += dt;
        if (timer >= frameTime) {
            timer = 0;
            frame++;
            if (frame >= maxFrames) frame = maxFrames - 1;
        }
    }
};

// ============================================================================
// 敌人结构体
// ============================================================================
struct Enemy {
    Vec2 position;
    Vec2 velocity = Vec2(0, 0);
    bool facingRight = false;
    bool grounded = false;
    bool alive = true;
    
    EnemyType type;
    EnemyAI ai;
    CombatComponent combat;
    AnimController anim;
    
    // 纹理引用（不持有所有权）
    std::shared_ptr<Texture> texIdle, texRun, texAttack, texDead;
    SheetInfo sheetIdle, sheetRun, sheetAttack, sheetDead;
    
    float deathTimer = 0;  // 死亡动画计时
};

// ============================================================================
// 全局变量
// ============================================================================
std::unique_ptr<Renderer2D> g_renderer;
std::unique_ptr<SpriteRenderer> g_spriteRenderer;
std::unique_ptr<Tilemap> g_tilemap;
std::unique_ptr<AudioSystem> g_audio;

// 纹理
std::shared_ptr<Texture> g_tilesetTex;
std::shared_ptr<Texture> g_playerIdleTex, g_playerRunTex, g_playerPunchTex;
std::shared_ptr<Texture> g_robot4Idle, g_robot4Run, g_robot4Punch, g_robot4Dead;
std::shared_ptr<Texture> g_robot8Idle, g_robot8Run, g_robot8Shoot, g_robot8Dead;

// 玩家
struct {
    Vec2 position = Vec2(100, 86);
    Vec2 velocity = Vec2(0, 0);
    bool facingRight = true;
    bool grounded = false;
    CombatComponent combat;
    AttackData punchAttack;
    AnimController anim;
    float moveSpeed = 250.0f;
    float jumpForce = 500.0f;
} g_player;

// 敌人列表
std::vector<Enemy> g_enemies;

// 显示
bool g_showDebug = false;
int g_selectedEnemy = 0;

const float GRAVITY = -1200.f;
const int MAP_W = 40, MAP_H = 15;
const int TILE_SZ = 32;

// ============================================================================
// UV计算
// ============================================================================
void CalcUV(int idx, int cols, int rows, Vec2& uvMin, Vec2& uvMax) {
    int col = idx % cols, row = idx / cols;
    uvMin = Vec2(col / (float)cols, 1.f - (row + 1) / (float)rows);
    uvMax = Vec2((col + 1) / (float)cols, 1.f - row / (float)rows);
}

// ============================================================================
// 碰撞移动
// ============================================================================
struct ColResult { bool ground = false, ceiling = false, wallL = false, wallR = false; };

ColResult MoveWithCollision(Vec2& pos, Vec2& vel, const Vec2& size, float dt) {
    ColResult r;
    vel.y += GRAVITY * dt;
    if (vel.y < -800.f) vel.y = -800.f;
    Vec2 half = size * 0.5f;
    
    pos.x += vel.x * dt;
    AABB box(pos, half);
    for (const AABB& t : g_tilemap->GetCollidingTiles(box)) {
        if (box.Intersects(t)) {
            Vec2 pen = box.GetPenetration(t);
            if (vel.x > 0) { pos.x -= std::abs(pen.x); r.wallR = true; }
            else { pos.x += std::abs(pen.x); r.wallL = true; }
            vel.x = 0;
            box = AABB(pos, half);
        }
    }
    
    pos.y += vel.y * dt;
    box = AABB(pos, half);
    for (const AABB& t : g_tilemap->GetCollidingTiles(box)) {
        if (box.Intersects(t)) {
            Vec2 pen = box.GetPenetration(t);
            if (vel.y < 0) { pos.y += std::abs(pen.y); r.ground = true; }
            else { pos.y -= std::abs(pen.y); r.ceiling = true; }
            vel.y = 0;
            box = AABB(pos, half);
        }
    }
    return r;
}

// ============================================================================
// 创建地图
// ============================================================================
void CreateMap() {
    g_tilemap = std::make_unique<Tilemap>();
    g_tilemap->Create(MAP_W, MAP_H, TILE_SZ);
    int ground = g_tilemap->AddLayer("ground", true);
    
    // 地面
    for (int x = 0; x < MAP_W; x++) {
        g_tilemap->SetTile(ground, x, 0, 11);
        g_tilemap->SetTile(ground, x, 1, 9);
    }
    // 平台
    for (int x = 15; x < 25; x++) g_tilemap->SetTile(ground, x, 5, 9);
    for (int x = 5; x < 12; x++)  g_tilemap->SetTile(ground, x, 4, 9);
    for (int x = 28; x < 38; x++) g_tilemap->SetTile(ground, x, 4, 9);
    // 墙壁
    for (int y = 2; y < 12; y++) {
        g_tilemap->SetTile(ground, 0, y, 17);
        g_tilemap->SetTile(ground, MAP_W - 1, y, 17);
    }
}

// ============================================================================
// 创建敌人
// ============================================================================
void CreateEnemies() {
    g_enemies.clear();
    
    // 近战敌人1 (robot4) - 左侧平台巡逻
    {
        Enemy e;
        e.position = Vec2(250, 86);
        e.type = EnemyType::Melee;
        e.texIdle = g_robot4Idle;   e.sheetIdle   = {7, 7, 14, 0.12f};
        e.texRun  = g_robot4Run;    e.sheetRun    = {7, 6, 12, 0.08f};
        e.texAttack = g_robot4Punch; e.sheetAttack = {8, 4, 10, 0.05f};
        e.texDead = g_robot4Dead;    e.sheetDead   = {10, 3, 15, 0.08f};
        
        AIConfig cfg;
        cfg.type = EnemyType::Melee;
        cfg.patrolMinX = 180; cfg.patrolMaxX = 350;
        cfg.detectionRange = 220; cfg.loseRange = 320;
        cfg.attackRange = 65; cfg.chaseSpeed = 160;
        cfg.attackCooldown = 1.2f;
        e.ai.SetConfig(cfg);
        
        e.combat.stats.maxHealth = 40;
        e.combat.stats.currentHealth = 40;
        e.combat.stats.attack = 8;
        e.combat.stats.defense = 1;
        g_enemies.push_back(e);
    }
    
    // 近战敌人2 (robot4) - 右侧巡逻
    {
        Enemy e;
        e.position = Vec2(700, 86);
        e.type = EnemyType::Melee;
        e.texIdle = g_robot4Idle;   e.sheetIdle   = {7, 7, 14, 0.12f};
        e.texRun  = g_robot4Run;    e.sheetRun    = {7, 6, 12, 0.08f};
        e.texAttack = g_robot4Punch; e.sheetAttack = {8, 4, 10, 0.05f};
        e.texDead = g_robot4Dead;    e.sheetDead   = {10, 3, 15, 0.08f};
        
        AIConfig cfg;
        cfg.type = EnemyType::Melee;
        cfg.patrolMinX = 600; cfg.patrolMaxX = 850;
        cfg.detectionRange = 200; cfg.loseRange = 300;
        cfg.attackRange = 65; cfg.chaseSpeed = 150;
        cfg.attackCooldown = 1.5f;
        e.ai.SetConfig(cfg);
        
        e.combat.stats.maxHealth = 50;
        e.combat.stats.currentHealth = 50;
        e.combat.stats.attack = 10;
        e.combat.stats.defense = 2;
        g_enemies.push_back(e);
    }
    
    // 远程敌人 (robot8) - 中间平台
    {
        Enemy e;
        e.position = Vec2(500, 86);
        e.type = EnemyType::Ranged;
        e.texIdle = g_robot8Idle;    e.sheetIdle   = {9, 6, 16, 0.1f};
        e.texRun  = g_robot8Run;     e.sheetRun    = {7, 6, 12, 0.08f};
        e.texAttack = g_robot8Shoot;  e.sheetAttack = {8, 4, 10, 0.06f};
        e.texDead = g_robot8Dead;     e.sheetDead   = {7, 4, 14, 0.08f};
        
        AIConfig cfg;
        cfg.type = EnemyType::Ranged;
        cfg.patrolMinX = 420; cfg.patrolMaxX = 600;
        cfg.detectionRange = 300; cfg.loseRange = 400;
        cfg.attackRange = 250;  // 远程攻击范围大
        cfg.preferredDistance = 200; cfg.retreatDistance = 100;
        cfg.chaseSpeed = 120;
        cfg.attackCooldown = 1.8f; cfg.attackDuration = 0.4f;
        e.ai.SetConfig(cfg);
        
        e.combat.stats.maxHealth = 30;
        e.combat.stats.currentHealth = 30;
        e.combat.stats.attack = 12;
        e.combat.stats.defense = 0;
        g_enemies.push_back(e);
    }
}

// ============================================================================
// 加载纹理
// ============================================================================
bool LoadTex(std::shared_ptr<Texture>& tex, const char* name) {
    tex = std::make_shared<Texture>();
    std::string paths[] = {
        std::string("data/texture/") + name,
        std::string("../data/texture/") + name,
        std::string("../../data/texture/") + name
    };
    for (auto& p : paths) {
        if (tex->Load(p)) return true;
    }
    std::cout << "[Warn] Failed to load: " << name << std::endl;
    tex->CreateSolidColor(Vec4(1, 0, 1, 1), 64, 64);
    return false;
}

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) return false;
    g_spriteRenderer = std::make_unique<SpriteRenderer>();
    if (!g_spriteRenderer->Initialize()) return false;
    
    // 加载纹理
    g_tilesetTex = std::make_shared<Texture>();
    const char* tp[] = {"data/texture/tileset.png", "../data/texture/tileset.png"};
    for (auto p : tp) if (g_tilesetTex->Load(p)) break;
    
    LoadTex(g_playerIdleTex,  "robot3/robot3_idle.png");
    LoadTex(g_playerRunTex,   "robot3/robot3_run.png");
    LoadTex(g_playerPunchTex, "robot3/robot3_punch.png");
    
    LoadTex(g_robot4Idle,  "robot4/robot4_idle.png");
    LoadTex(g_robot4Run,   "robot4/robot4_run.png");
    LoadTex(g_robot4Punch, "robot4/robot4_punch_r.png");
    LoadTex(g_robot4Dead,  "robot4/robot4_dead.png");
    
    LoadTex(g_robot8Idle,  "robot8/robot8_idle.png");
    LoadTex(g_robot8Run,   "robot8/robot8_run.png");
    LoadTex(g_robot8Shoot, "robot8/robot8_shoot.png");
    LoadTex(g_robot8Dead,  "robot8/robot8_dead.png");
    
    CreateMap();
    
    // 音频
    g_audio = std::make_unique<AudioSystem>();
    g_audio->Initialize();
    const char* bp[] = {"data/", "../data/", "../../data/"};
    for (auto b : bp) {
        if (g_audio->LoadSound("attack", std::string(b) + "audio/sfx/Sound_Axe.wav")) break;
    }
    for (auto b : bp) {
        if (g_audio->LoadMusic("bgm", std::string(b) + "audio/bgm/windaswarriors.mp3")) {
            g_audio->PlayMusic("bgm", 0.3f);
            break;
        }
    }
    
    // 玩家攻击配置
    g_player.punchAttack.name = "punch";
    g_player.punchAttack.baseDamage = 15;
    g_player.punchAttack.startupFrames = 0.05f;
    g_player.punchAttack.activeFrames = 0.15f;
    g_player.punchAttack.recoveryFrames = 0.1f;
    g_player.punchAttack.knockback = Vec2(120, 30);
    g_player.punchAttack.stunTime = 0.3f;
    g_player.punchAttack.hitboxOffset = Vec2(55, 0);
    g_player.punchAttack.hitboxSize = Vec2(70, 50);
    g_player.punchAttack.animFrameCount = 8;
    g_player.punchAttack.animFPS = 20.0f;
    
    g_player.combat.stats.maxHealth = 100;
    g_player.combat.stats.currentHealth = 100;
    g_player.combat.stats.attack = 10;
    
    g_player.combat.onAttackStart = [](const std::string&) {
        g_audio->PlaySFX("attack", 0.7f);
    };
    
    CreateEnemies();
    
    std::cout << "\n=== Chapter 15: Enemy AI ===" << std::endl;
    std::cout << "A/D: Move | Space: Jump | J: Attack" << std::endl;
    std::cout << "Tab: Debug | R: Reset | 1/2/3: Select Enemy" << std::endl;
    
    return true;
}

// ============================================================================
// 玩家更新
// ============================================================================
void UpdatePlayer(float dt, GLFWwindow* w) {
    g_player.combat.Update(dt);
    
    float move = 0;
    if (g_player.combat.CanAct()) {
        if (glfwGetKey(w, GLFW_KEY_A)) move = -1;
        if (glfwGetKey(w, GLFW_KEY_D)) move = 1;
        if (move != 0) g_player.facingRight = move > 0;
        g_player.velocity.x = move * g_player.moveSpeed;
    } else {
        g_player.velocity.x *= 0.9f;
    }
    
    static bool spLast = false;
    bool sp = glfwGetKey(w, GLFW_KEY_SPACE);
    if (sp && !spLast && g_player.grounded && g_player.combat.CanAct()) {
        g_player.velocity.y = g_player.jumpForce;
        g_player.grounded = false;
    }
    spLast = sp;
    
    static bool jLast = false;
    bool j = glfwGetKey(w, GLFW_KEY_J);
    if (j && !jLast) g_player.combat.StartAttack(g_player.punchAttack);
    jLast = j;
    
    ColResult col = MoveWithCollision(g_player.position, g_player.velocity, Vec2(40, 44), dt);
    g_player.grounded = col.ground;
    
    // 动画
    if (g_player.combat.state == CombatState::Attacking) {
        g_player.anim.Play("punch");
        g_player.anim.UpdateOnce(dt, 8, 0.05f);
    } else if (std::abs(g_player.velocity.x) > 10) {
        g_player.anim.Play("run");
        g_player.anim.Update(dt, 9, 0.08f);
    } else {
        g_player.anim.Play("idle");
        g_player.anim.Update(dt, 7, 0.1f);
    }
}

// ============================================================================
// 敌人更新
// ============================================================================
void UpdateEnemies(float dt) {
    for (auto& e : g_enemies) {
        if (!e.alive) {
            e.deathTimer += dt;
            e.anim.Play("dead");
            e.anim.UpdateOnce(dt, e.sheetDead.totalFrames, e.sheetDead.frameTime);
            continue;
        }
        
        // AI更新
        e.ai.Update(dt, e.position, g_player.position);
        e.combat.Update(dt);
        
        // AI决策 → 物理移动
        float dir = e.ai.GetMoveDirection();
        float speed = e.ai.GetMoveSpeed();
        e.velocity.x = dir * speed;
        e.facingRight = e.ai.IsFacingRight();
        
        // AI想要攻击
        if (e.ai.WantsToAttack()) {
            // 简单的攻击判定：如果在攻击范围内就直接造成伤害
            float dist = std::abs(g_player.position.x - e.position.x);
            float atkRange = (e.type == EnemyType::Ranged) ? 250.f : 70.f;
            if (dist <= atkRange && g_player.combat.stats.IsAlive() && !g_player.combat.IsInvincible()) {
                DamageInfo dmg;
                dmg.amount = e.combat.stats.attack;
                dmg.knockback = Vec2(e.facingRight ? 80.f : -80.f, 20.f);
                dmg.stunTime = 0.2f;
                g_player.combat.TakeDamage(dmg);
            }
        }
        
        // 碰撞移动
        ColResult col = MoveWithCollision(e.position, e.velocity, Vec2(36, 44), dt);
        e.grounded = col.ground;
        
        // 动画选择
        AIState st = e.ai.GetState();
        if (st == AIState::Attack) {
            e.anim.Play("attack");
            e.anim.UpdateOnce(dt, e.sheetAttack.totalFrames, e.sheetAttack.frameTime);
        } else if (st == AIState::Chase) {
            e.anim.Play("run");
            e.anim.Update(dt, e.sheetRun.totalFrames, e.sheetRun.frameTime);
        } else if (st == AIState::Patrol) {
            e.anim.Play("idle");  // 巡逻用idle动画（走路较慢）
            e.anim.Update(dt, e.sheetIdle.totalFrames, e.sheetIdle.frameTime);
        } else if (st == AIState::Hurt) {
            // 受击时保持当前帧不动
        } else {
            e.anim.Play("idle");
            e.anim.Update(dt, e.sheetIdle.totalFrames, e.sheetIdle.frameTime);
        }
    }
}

// ============================================================================
// 战斗检测
// ============================================================================
void CheckCombat() {
    if (!g_player.combat.IsAttackActive() || g_player.combat.hitConnected) return;
    
    // 构建玩家攻击hitbox
    Vec2 atkOff = g_player.punchAttack.hitboxOffset;
    if (!g_player.facingRight) atkOff.x = -atkOff.x;
    Vec2 atkCenter = g_player.position + atkOff;
    AABB playerHitbox(atkCenter, g_player.punchAttack.hitboxSize * 0.5f);
    
    for (auto& e : g_enemies) {
        if (!e.alive) continue;
        
        AABB enemyBox(e.position, Vec2(20, 22));
        if (playerHitbox.Intersects(enemyBox)) {
            g_player.combat.hitConnected = true;
            
            DamageInfo dmg;
            dmg.amount = g_player.punchAttack.baseDamage + g_player.combat.stats.attack;
            dmg.knockback = Vec2(g_player.facingRight ? 120.f : -120.f, 30.f);
            dmg.stunTime = 0.3f;
            
            e.combat.TakeDamage(dmg);
            e.ai.OnHurt(dmg.stunTime);
            
            // 击退
            e.velocity.x = dmg.knockback.x;
            e.velocity.y = dmg.knockback.y;
            
            if (!e.combat.stats.IsAlive()) {
                e.alive = false;
                e.ai.OnDeath();
                e.velocity = Vec2(0, 0);
            }
            
            g_audio->PlaySFX("attack", 0.8f);
            break;  // 每次攻击只命中一个
        }
    }
}

// ============================================================================
// 更新
// ============================================================================
void Update(float dt) {
    Engine& engine = Engine::Instance();
    GLFWwindow* w = engine.GetWindow();
    if (dt > 0.1f) dt = 0.1f;
    
    static bool tabL = false, rL = false;
    bool tab = glfwGetKey(w, GLFW_KEY_TAB), r = glfwGetKey(w, GLFW_KEY_R);
    if (tab && !tabL) g_showDebug = !g_showDebug;
    if (r && !rL) {
        g_player.position = Vec2(100, 86);
        g_player.velocity = Vec2(0, 0);
        g_player.combat.stats.FullHeal();
        g_player.combat.state = CombatState::Idle;
        CreateEnemies();
    }
    tabL = tab; rL = r;
    
    if (glfwGetKey(w, GLFW_KEY_1)) g_selectedEnemy = 0;
    if (glfwGetKey(w, GLFW_KEY_2)) g_selectedEnemy = 1;
    if (glfwGetKey(w, GLFW_KEY_3)) g_selectedEnemy = 2;
    
    UpdatePlayer(dt, w);
    UpdateEnemies(dt);
    CheckCombat();
}

// ============================================================================
// 渲染精灵
// ============================================================================
void DrawCharacter(std::shared_ptr<Texture> tex, int cols, int rows, int frame,
                   Vec2 pos, bool faceRight, Vec4 color = Vec4(1,1,1,1), float size = 120.f) {
    Vec2 uvMin, uvMax;
    CalcUV(frame, cols, rows, uvMin, uvMax);
    if (!faceRight) std::swap(uvMin.x, uvMax.x);
    
    Sprite spr;
    spr.SetTexture(tex);
    spr.SetPosition(pos);
    spr.SetSize(Vec2(size, size));
    spr.SetUV(uvMin, uvMax);
    spr.SetColor(color);
    g_spriteRenderer->DrawSprite(spr);
}

// ============================================================================
// 渲染
// ============================================================================
void Render() {
    Engine& engine = Engine::Instance();
    int sw = engine.GetWindowWidth(), sh = engine.GetWindowHeight();
    
    glClearColor(0.12f, 0.15f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    Mat4 proj = Mat4::Ortho(0, (float)sw, 0, (float)sh, -1, 1);
    g_spriteRenderer->SetProjection(proj);
    g_spriteRenderer->Begin();
    
    // 瓦片地图
    if (g_tilesetTex->IsValid()) {
        int tw = TILE_SZ, th = TILE_SZ;
        for (int l = 0; l < g_tilemap->GetLayerCount(); l++) {
            const TilemapLayer* layer = g_tilemap->GetLayer(l);
            if (!layer || !layer->visible) continue;
            for (int y = 0; y < g_tilemap->GetHeight(); y++) {
                for (int x = 0; x < g_tilemap->GetWidth(); x++) {
                    int id = layer->GetTile(x, y);
                    if (id == 0) continue;
                    int idx = id - 1, col = idx % 8, row = idx / 8;
                    Vec2 uvMin(col/8.f, 1.f-(row+1)/5.f), uvMax((col+1)/8.f, 1.f-row/5.f);
                    Sprite tile;
                    tile.SetTexture(g_tilesetTex);
                    tile.SetPosition(Vec2(x*tw+tw*0.5f, y*th+th*0.5f));
                    tile.SetSize(Vec2((float)tw, (float)th));
                    tile.SetUV(uvMin, uvMax);
                    g_spriteRenderer->DrawSprite(tile);
                }
            }
        }
    }
    
    // 渲染敌人
    for (int i = 0; i < (int)g_enemies.size(); i++) {
        auto& e = g_enemies[i];
        std::shared_ptr<Texture> tex; int cols, rows, frame;
        
        if (!e.alive) {
            tex = e.texDead; cols = e.sheetDead.cols; rows = e.sheetDead.rows;
            frame = e.anim.frame;
        } else if (e.anim.currentAnim == "attack") {
            tex = e.texAttack; cols = e.sheetAttack.cols; rows = e.sheetAttack.rows;
            frame = e.anim.frame;
        } else if (e.anim.currentAnim == "run") {
            tex = e.texRun; cols = e.sheetRun.cols; rows = e.sheetRun.rows;
            frame = e.anim.frame;
        } else {
            tex = e.texIdle; cols = e.sheetIdle.cols; rows = e.sheetIdle.rows;
            frame = e.anim.frame;
        }
        
        float alpha = e.combat.GetRenderAlpha();
        Vec4 color(1, 1, 1, alpha);
        if (!e.alive) color = Vec4(0.6f, 0.6f, 0.6f, std::max(0.f, 1.f - e.deathTimer));
        else if (e.combat.IsInvincible()) color = Vec4(1, 0.4f, 0.4f, alpha);
        
        DrawCharacter(tex, cols, rows, frame, e.position, e.facingRight, color, 120.f);
        
        // 敌人血条
        if (e.alive) {
            float hpPct = e.combat.stats.HealthPercent();
            float barW = 50, barH = 5;
            Vec2 barPos(e.position.x, e.position.y + 48);
            // 背景
            g_spriteRenderer->DrawRect(barPos, Vec2(barW, barH), Vec4(0.2f, 0.2f, 0.2f, 0.8f));
            // 血量
            Vec4 hpCol = hpPct > 0.5f ? Vec4(0,0.8f,0,0.9f) : Vec4(0.9f,0.2f,0,0.9f);
            g_spriteRenderer->DrawRect(Vec2(barPos.x - barW*0.5f*(1-hpPct), barPos.y),
                                       Vec2(barW * hpPct, barH), hpCol);
        }
    }
    
    // 渲染玩家
    {
        std::shared_ptr<Texture> tex = g_playerIdleTex;
        int cols = 7, rows = 7;
        if (g_player.anim.currentAnim == "run") { tex = g_playerRunTex; cols = 9; rows = 6; }
        else if (g_player.anim.currentAnim == "punch") { tex = g_playerPunchTex; cols = 8; rows = 4; }
        float alpha = g_player.combat.GetRenderAlpha();
        DrawCharacter(tex, cols, rows, g_player.anim.frame, g_player.position, g_player.facingRight,
                     Vec4(1,1,1,alpha), 120.f);
    }
    
    // 调试渲染
    if (g_showDebug) {
        for (auto& e : g_enemies) {
            if (!e.alive) continue;
            float det = e.ai.GetConfig().detectionRange;
            float atk = e.ai.GetConfig().attackRange;
            
            // 检测范围（蓝色圆圈近似 - 用矩形表示）
            g_spriteRenderer->DrawRect(e.position, Vec2(det*2, 4), Vec4(0.3f, 0.5f, 1, 0.3f));
            g_spriteRenderer->DrawRect(e.position, Vec2(4, det*2), Vec4(0.3f, 0.5f, 1, 0.3f));
            
            // 攻击范围（红色）
            g_spriteRenderer->DrawRect(e.position, Vec2(atk*2, 3), Vec4(1, 0.3f, 0.3f, 0.4f));
        }
        
        // 玩家攻击hitbox
        if (g_player.combat.IsAttackActive()) {
            Vec2 off = g_player.punchAttack.hitboxOffset;
            if (!g_player.facingRight) off.x = -off.x;
            g_spriteRenderer->DrawRect(g_player.position + off,
                g_player.punchAttack.hitboxSize, Vec4(1, 0, 0, 0.5f));
        }
    }
    
    g_spriteRenderer->End();
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui() {
    Engine& e = Engine::Instance();
    
    ImGui::Begin("Enemy AI Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %.0f", e.GetFPS());
    ImGui::Separator();
    
    // 玩家信息
    ImGui::Text("=== Player ===");
    ImGui::Text("Pos: (%.0f, %.0f)", g_player.position.x, g_player.position.y);
    float pHp = g_player.combat.stats.HealthPercent();
    ImGui::ProgressBar(pHp, ImVec2(180, 16), "Player HP");
    ImGui::Separator();
    
    // 选中的敌人信息
    if (g_selectedEnemy >= 0 && g_selectedEnemy < (int)g_enemies.size()) {
        auto& en = g_enemies[g_selectedEnemy];
        ImGui::Text("=== Enemy %d (%s) ===", g_selectedEnemy + 1,
            en.type == EnemyType::Melee ? "Melee" : "Ranged");
        ImGui::Text("Pos: (%.0f, %.0f)", en.position.x, en.position.y);
        ImGui::Text("AI State: %s", en.ai.GetStateName().c_str());
        ImGui::Text("Facing: %s", en.facingRight ? "Right" : "Left");
        ImGui::Text("Move Dir: %.0f  Speed: %.0f", en.ai.GetMoveDirection(), en.ai.GetMoveSpeed());
        ImGui::Text("Alive: %s", en.alive ? "Yes" : "No");
        
        float dist = std::abs(g_player.position.x - en.position.x);
        ImGui::Text("Dist to Player: %.0f", dist);
        ImGui::Text("Detection: %.0f  Attack: %.0f",
            en.ai.GetConfig().detectionRange, en.ai.GetConfig().attackRange);
        
        float eHp = en.combat.stats.HealthPercent();
        ImVec4 hpCol = eHp > 0.5f ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpCol);
        char buf[32]; snprintf(buf, 32, "%d/%d", en.combat.stats.currentHealth, en.combat.stats.maxHealth);
        ImGui::ProgressBar(eHp, ImVec2(180, 16), buf);
        ImGui::PopStyleColor();
    }
    
    ImGui::Separator();
    ImGui::Text("Enemies: %d alive", (int)std::count_if(g_enemies.begin(), g_enemies.end(),
        [](const Enemy& e){ return e.alive; }));
    ImGui::Checkbox("Show Debug (Tab)", &g_showDebug);
    if (ImGui::Button("Reset All (R)")) {
        g_player.position = Vec2(100, 86);
        g_player.combat.stats.FullHeal();
        CreateEnemies();
    }
    
    ImGui::End();
}

// ============================================================================
// 清理 & 主函数
// ============================================================================
void Cleanup() {
    if (g_audio) g_audio->Shutdown();
    g_enemies.clear();
    g_tilemap.reset();
    g_spriteRenderer.reset();
    g_renderer.reset();
    g_audio.reset();
}

int main() {
    std::cout << "=== Example 15: Enemy AI ===" << std::endl;
    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth = 1280; cfg.windowHeight = 720;
    cfg.windowTitle = "Mini Engine - Enemy AI";
    if (!engine.Initialize(cfg)) return -1;
    if (!Initialize()) { engine.Shutdown(); return -1; }
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    Cleanup();
    engine.Shutdown();
    return 0;
}
