/**
 * 第14章：战斗系统 (Combat System)
 * 
 * 学习内容：
 * 1. 攻击状态机（起手、判定、收招）
 * 2. Hitbox/Hurtbox碰撞检测
 * 3. 伤害计算与受击反馈
 * 4. 攻击动画与音效整合
 * 
 * 操作：
 * - A/D: 移动
 * - Space: 跳跃
 * - J: 攻击（拳击）
 * - Tab: 显示/隐藏碰撞框
 * - R: 重置假人
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
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <cmath>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 全局变量
// ============================================================================

// 渲染
std::unique_ptr<Renderer2D> g_renderer;
std::unique_ptr<SpriteRenderer> g_spriteRenderer;

// 纹理
std::shared_ptr<Texture> g_tilesetTex;
std::shared_ptr<Texture> g_playerIdleTex;
std::shared_ptr<Texture> g_playerRunTex;
std::shared_ptr<Texture> g_playerPunchTex;

// 地图
std::unique_ptr<Tilemap> g_tilemap;

// 音频
std::unique_ptr<AudioSystem> g_audio;

// 玩家
struct Player {
    Vec2 position = Vec2(200, 86);  // 地面顶部y=64，碰撞半高22
    Vec2 velocity = Vec2(0, 0);
    bool facingRight = true;
    bool grounded = false;
    
    // 动画
    int animFrame = 0;
    float animTimer = 0;
    std::string currentAnim = "idle";
    
    // 战斗
    CombatComponent combat;
    AttackData punchAttack;
    
    // 配置
    float moveSpeed = 250.0f;
    float jumpForce = 500.0f;
} g_player;

// 训练假人（受击目标）
struct Dummy {
    Vec2 position = Vec2(500, 86);
    bool facingRight = false;
    
    CombatComponent combat;
    
    // 受击效果
    Vec2 knockbackVelocity = Vec2(0, 0);
} g_dummy;

// 显示选项
bool g_showHitboxes = false;

// 物理常量
const float GRAVITY = -1200.f;
const float TILE_SIZE = 32.0f;

// ============================================================================
// 辅助函数
// ============================================================================

void CalculateFrameUV(int frameIdx, int cols, int rows, Vec2& uvMin, Vec2& uvMax) {
    int col = frameIdx % cols;
    int row = frameIdx / cols;
    uvMin = Vec2(col / (float)cols, 1.0f - (row + 1) / (float)rows);
    uvMax = Vec2((col + 1) / (float)cols, 1.0f - row / (float)rows);
}

// 使用正确的AABB构造方式进行碰撞检测移动
struct ColResult { bool ground = false, ceiling = false, wallL = false, wallR = false; };

ColResult MoveWithCollision(Vec2& pos, Vec2& vel, const Vec2& size, float dt) {
    ColResult r;
    vel.y += GRAVITY * dt;
    if (vel.y < -800.f) vel.y = -800.f;
    
    Vec2 halfSz = size * 0.5f;
    
    // X轴移动
    pos.x += vel.x * dt;
    AABB box(pos, halfSz);
    for (const AABB& t : g_tilemap->GetCollidingTiles(box)) {
        if (box.Intersects(t)) {
            Vec2 pen = box.GetPenetration(t);
            if (vel.x > 0) { pos.x -= std::abs(pen.x); r.wallR = true; }
            else { pos.x += std::abs(pen.x); r.wallL = true; }
            vel.x = 0;
            box = AABB(pos, halfSz);
        }
    }
    
    // Y轴移动
    pos.y += vel.y * dt;
    box = AABB(pos, halfSz);
    for (const AABB& t : g_tilemap->GetCollidingTiles(box)) {
        if (box.Intersects(t)) {
            Vec2 pen = box.GetPenetration(t);
            if (vel.y < 0) { pos.y += std::abs(pen.y); r.ground = true; }
            else { pos.y -= std::abs(pen.y); r.ceiling = true; }
            vel.y = 0;
            box = AABB(pos, halfSz);
        }
    }
    return r;
}

// ============================================================================
// 创建地图
// ============================================================================

void CreateArena() {
    g_tilemap = std::make_unique<Tilemap>();
    int w = 25, h = 15;
    g_tilemap->Create(w, h, 32);
    
    int ground = g_tilemap->AddLayer("ground", true);
    
    // 地面（底部2层）
    for (int x = 0; x < w; x++) {
        g_tilemap->SetTile(ground, x, 0, 11);  // 底层泥土
        g_tilemap->SetTile(ground, x, 1, 9);   // 草地
    }
    
    // 中间平台
    for (int x = 8; x < 17; x++) {
        g_tilemap->SetTile(ground, x, 5, 9);
    }
    
    // 左右墙壁
    for (int y = 2; y < 12; y++) {
        g_tilemap->SetTile(ground, 0, y, 17);
        g_tilemap->SetTile(ground, w - 1, y, 17);
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
    
    // 加载纹理
    g_tilesetTex = std::make_shared<Texture>();
    g_playerIdleTex = std::make_shared<Texture>();
    g_playerRunTex = std::make_shared<Texture>();
    g_playerPunchTex = std::make_shared<Texture>();
    
    const char* basePaths[] = {"data/", "../data/", "../../data/"};
    bool loaded = false;
    
    for (const char* base : basePaths) {
        std::string tilesetPath = std::string(base) + "texture/tileset.png";
        std::string idlePath = std::string(base) + "texture/robot3/robot3_idle.png";
        std::string runPath = std::string(base) + "texture/robot3/robot3_run.png";
        std::string punchPath = std::string(base) + "texture/robot3/robot3_punch.png";
        
        if (g_tilesetTex->Load(tilesetPath) &&
            g_playerIdleTex->Load(idlePath) &&
            g_playerRunTex->Load(runPath) &&
            g_playerPunchTex->Load(punchPath)) {
            loaded = true;
            std::cout << "[Chapter14] Textures loaded from: " << base << std::endl;
            break;
        }
    }
    
    if (!loaded) {
        std::cout << "[Chapter14] WARNING: Some textures not found, using fallback colors" << std::endl;
        if (!g_tilesetTex->IsValid()) g_tilesetTex->CreateSolidColor(Vec4(0.4f, 0.6f, 0.3f, 1), 32, 32);
        if (!g_playerIdleTex->IsValid()) g_playerIdleTex->CreateSolidColor(Vec4(0.3f, 0.6f, 1, 1), 64, 64);
        if (!g_playerRunTex->IsValid()) g_playerRunTex->CreateSolidColor(Vec4(0.3f, 0.8f, 1, 1), 64, 64);
        if (!g_playerPunchTex->IsValid()) g_playerPunchTex->CreateSolidColor(Vec4(1, 0.5f, 0.3f, 1), 64, 64);
    }
    
    // 创建战斗场地
    CreateArena();
    
    // 初始化音频
    g_audio = std::make_unique<AudioSystem>();
    g_audio->Initialize();
    
    for (const char* base : basePaths) {
        std::string axePath = std::string(base) + "audio/sfx/Sound_Axe.wav";
        if (g_audio->LoadSound("attack", axePath)) {
            std::cout << "[Chapter14] Attack SFX loaded" << std::endl;
            break;
        }
    }
    
    // 初始化玩家攻击数据
    g_player.punchAttack.name = "punch";
    g_player.punchAttack.baseDamage = 15;
    g_player.punchAttack.startupFrames = 0.05f;   // 起手50ms
    g_player.punchAttack.activeFrames = 0.15f;     // 判定150ms
    g_player.punchAttack.recoveryFrames = 0.1f;    // 收招100ms
    g_player.punchAttack.knockback = Vec2(150, 50);
    g_player.punchAttack.stunTime = 0.3f;
    g_player.punchAttack.hitboxOffset = Vec2(60, 0);
    g_player.punchAttack.hitboxSize = Vec2(80, 60);
    g_player.punchAttack.animFrameCount = 8;
    g_player.punchAttack.animFPS = 20.0f;
    
    // 初始化战斗属性
    g_player.combat.stats.maxHealth = 100;
    g_player.combat.stats.currentHealth = 100;
    g_player.combat.stats.attack = 10;
    
    g_dummy.combat.stats.maxHealth = 100;
    g_dummy.combat.stats.currentHealth = 100;
    g_dummy.combat.stats.defense = 2;
    
    // 设置假人受击回调
    g_dummy.combat.onTakeDamage = [](const DamageInfo& info) {
        std::cout << "[Dummy] Took " << info.amount << " damage! HP: "
                  << g_dummy.combat.stats.currentHealth << "/"
                  << g_dummy.combat.stats.maxHealth << std::endl;
        g_dummy.knockbackVelocity = info.knockback;
    };
    
    g_dummy.combat.onDeath = []() {
        std::cout << "[Dummy] Defeated!" << std::endl;
    };
    
    // 设置玩家攻击回调 - 播放攻击音效
    g_player.combat.onAttackStart = [](const std::string& name) {
        g_audio->PlaySFX("attack", 0.8f);
    };
    
    std::cout << "\n=== Chapter 14: Combat System ===" << std::endl;
    std::cout << "A/D: Move | Space: Jump | J: Attack" << std::endl;
    std::cout << "Tab: Toggle Hitbox | R: Reset Dummy" << std::endl;
    
    return true;
}

// ============================================================================
// 更新逻辑
// ============================================================================

void UpdatePlayer(float dt, GLFWwindow* w) {
    // 更新战斗组件
    g_player.combat.Update(dt);
    
    // 输入处理（只有在可行动状态下才能移动）
    float moveInput = 0;
    if (g_player.combat.CanAct()) {
        if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) moveInput = -1;
        if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) moveInput = 1;
        if (moveInput != 0) g_player.facingRight = moveInput > 0;
    }
    
    // 水平移动
    if (g_player.combat.CanAct()) {
        g_player.velocity.x = moveInput * g_player.moveSpeed;
    } else {
        g_player.velocity.x *= 0.9f;  // 攻击中减速
    }
    
    // 跳跃
    static bool spaceLastFrame = false;
    bool spacePressed = glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spacePressed && !spaceLastFrame && g_player.grounded && g_player.combat.CanAct()) {
        g_player.velocity.y = g_player.jumpForce;
        g_player.grounded = false;
    }
    spaceLastFrame = spacePressed;
    
    // 攻击 (J键)
    static bool jLastFrame = false;
    bool jPressed = glfwGetKey(w, GLFW_KEY_J) == GLFW_PRESS;
    if (jPressed && !jLastFrame) {
        g_player.combat.StartAttack(g_player.punchAttack);
    }
    jLastFrame = jPressed;
    
    // 碰撞移动（使用修复后的AABB构造）
    ColResult col = MoveWithCollision(g_player.position, g_player.velocity, Vec2(40, 44), dt);
    g_player.grounded = col.ground;
    
    // 更新动画
    if (g_player.combat.state == CombatState::Attacking) {
        if (g_player.currentAnim != "punch") { g_player.animFrame = 0; g_player.animTimer = 0; }
        g_player.currentAnim = "punch";
        g_player.animTimer += dt;
        if (g_player.animTimer >= 1.0f / g_player.punchAttack.animFPS) {
            g_player.animTimer = 0;
            g_player.animFrame++;
            if (g_player.animFrame >= g_player.punchAttack.animFrameCount) {
                g_player.animFrame = g_player.punchAttack.animFrameCount - 1;
            }
        }
    } else if (std::abs(g_player.velocity.x) > 10) {
        if (g_player.currentAnim != "run") { g_player.animFrame = 0; g_player.animTimer = 0; }
        g_player.currentAnim = "run";
        g_player.animTimer += dt;
        if (g_player.animTimer >= 0.08f) {
            g_player.animTimer = 0;
            g_player.animFrame = (g_player.animFrame + 1) % 9;  // robot3_run: 9列
        }
    } else {
        if (g_player.currentAnim != "idle") { g_player.animFrame = 0; g_player.animTimer = 0; }
        g_player.currentAnim = "idle";
        g_player.animTimer += dt;
        if (g_player.animTimer >= 0.1f) {
            g_player.animTimer = 0;
            g_player.animFrame = (g_player.animFrame + 1) % 7;  // robot3_idle: 7列
        }
    }
}

void UpdateDummy(float dt) {
    g_dummy.combat.Update(dt);
    
    // 应用击退
    if (g_dummy.knockbackVelocity.x != 0 || g_dummy.knockbackVelocity.y != 0) {
        g_dummy.position = g_dummy.position + g_dummy.knockbackVelocity * dt;
        g_dummy.knockbackVelocity = g_dummy.knockbackVelocity * 0.9f;
        
        if (std::abs(g_dummy.knockbackVelocity.x) < 1 && std::abs(g_dummy.knockbackVelocity.y) < 1) {
            g_dummy.knockbackVelocity = Vec2(0, 0);
        }
    }
    
    // 保持在地面上（地面2层瓦片=64px，碰撞半高22）
    float groundY = 2 * TILE_SIZE + 22;
    if (g_dummy.position.y < groundY) {
        g_dummy.position.y = groundY;
    }
}

void CheckCombat() {
    // 检测玩家攻击是否命中假人
    if (g_player.combat.IsAttackActive() && !g_player.combat.hitConnected) {
        if (g_player.combat.hitboxes.CheckHit(
                g_dummy.combat.hitboxes,
                g_player.position, g_player.facingRight,
                g_dummy.position, g_dummy.facingRight)) {
            
            // 命中！
            g_player.combat.hitConnected = true;
            
            DamageInfo dmg;
            dmg.amount = g_player.punchAttack.baseDamage + g_player.combat.stats.attack;
            dmg.knockback = g_player.punchAttack.knockback;
            if (!g_player.facingRight) {
                dmg.knockback.x = -dmg.knockback.x;
            }
            dmg.stunTime = g_player.punchAttack.stunTime;
            dmg.attacker = &g_player;
            
            g_dummy.combat.TakeDamage(dmg);
        }
    }
}

void Update(float dt) {
    Engine& engine = Engine::Instance();
    GLFWwindow* w = engine.GetWindow();
    if (dt > 0.1f) dt = 0.1f;
    
    // Tab切换hitbox显示
    static bool tabLast = false;
    bool tabPressed = glfwGetKey(w, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabPressed && !tabLast) g_showHitboxes = !g_showHitboxes;
    tabLast = tabPressed;
    
    // R重置假人
    static bool rLast = false;
    bool rPressed = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS;
    if (rPressed && !rLast) {
        g_dummy.combat.stats.FullHeal();
        g_dummy.combat.state = CombatState::Idle;
        g_dummy.position = Vec2(500, 86);
        g_dummy.knockbackVelocity = Vec2(0, 0);
        std::cout << "[Dummy] Reset!" << std::endl;
    }
    rLast = rPressed;
    
    UpdatePlayer(dt, w);
    UpdateDummy(dt);
    CheckCombat();
}

// ============================================================================
// 渲染
// ============================================================================

void Render() {
    Engine& engine = Engine::Instance();
    int sw = engine.GetWindowWidth(), sh = engine.GetWindowHeight();
    
    glClearColor(0.15f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    Mat4 proj = Mat4::Ortho(0, (float)sw, 0, (float)sh, -1, 1);
    g_spriteRenderer->SetProjection(proj);
    g_spriteRenderer->Begin();
    
    // ---- 渲染瓦片地图 ----
    if (g_tilesetTex->IsValid()) {
        int tw = g_tilemap->GetTileWidth(), th = g_tilemap->GetTileHeight();
        for (int l = 0; l < g_tilemap->GetLayerCount(); l++) {
            const TilemapLayer* layer = g_tilemap->GetLayer(l);
            if (!layer || !layer->visible) continue;
            for (int y = 0; y < g_tilemap->GetHeight(); y++) {
                for (int x = 0; x < g_tilemap->GetWidth(); x++) {
                    int id = layer->GetTile(x, y);
                    if (id == 0) continue;
                    int idx = id - 1, col = idx % 8, row = idx / 8;
                    Vec2 uvMin(col / 8.f, 1.f - (row + 1) / 5.f);
                    Vec2 uvMax((col + 1) / 8.f, 1.f - row / 5.f);
                    Sprite tile;
                    tile.SetTexture(g_tilesetTex);
                    tile.SetPosition(Vec2(x * tw + tw * 0.5f, y * th + th * 0.5f));
                    tile.SetSize(Vec2((float)tw, (float)th));
                    tile.SetUV(uvMin, uvMax);
                    g_spriteRenderer->DrawSprite(tile);
                }
            }
        }
    }
    
    // ---- 渲染假人 ----
    {
        Vec2 uvMin, uvMax;
        CalculateFrameUV(0, 7, 7, uvMin, uvMax);  // idle第一帧
        if (!g_dummy.facingRight) std::swap(uvMin.x, uvMax.x);
        
        float alpha = g_dummy.combat.GetRenderAlpha();
        Vec4 color(1, 1, 1, alpha);
        
        if (!g_dummy.combat.stats.IsAlive()) {
            color = Vec4(0.5f, 0.5f, 0.5f, 0.5f);  // 死亡变灰
        } else if (g_dummy.combat.IsInvincible()) {
            color = Vec4(1, 0.5f, 0.5f, alpha);      // 受击变红
        }
        
        Sprite dummySprite;
        dummySprite.SetTexture(g_playerIdleTex);
        dummySprite.SetPosition(g_dummy.position);
        dummySprite.SetSize(Vec2(100, 100));
        dummySprite.SetUV(uvMin, uvMax);
        dummySprite.SetColor(color);
        g_spriteRenderer->DrawSprite(dummySprite);
    }
    
    // ---- 渲染玩家 ----
    {
        std::shared_ptr<Texture> tex = g_playerIdleTex;
        int cols = 7, rows = 7;
        int frame = g_player.animFrame;
        
        if (g_player.currentAnim == "run") {
            tex = g_playerRunTex;
            cols = 9; rows = 6;
        } else if (g_player.currentAnim == "punch") {
            tex = g_playerPunchTex;
            cols = 8; rows = 4;
        }
        
        Vec2 uvMin, uvMax;
        CalculateFrameUV(frame, cols, rows, uvMin, uvMax);
        if (!g_player.facingRight) std::swap(uvMin.x, uvMax.x);
        
        float alpha = g_player.combat.GetRenderAlpha();
        
        Sprite playerSprite;
        playerSprite.SetTexture(tex);
        playerSprite.SetPosition(g_player.position);
        playerSprite.SetSize(Vec2(100, 100));
        playerSprite.SetUV(uvMin, uvMax);
        playerSprite.SetColor(Vec4(1, 1, 1, alpha));
        g_spriteRenderer->DrawSprite(playerSprite);
    }
    
    // ---- 渲染Hitbox（调试） ----
    if (g_showHitboxes) {
        // 玩家hurtbox（绿色线框）
        for (auto& box : g_player.combat.hitboxes.boxes) {
            if (box.type == HitboxType::Hurtbox && box.active) {
                AABB aabb = box.GetWorldAABB(g_player.position, g_player.facingRight);
                Vec2 mn = aabb.GetMin(), mx = aabb.GetMax();
                float bw = mx.x - mn.x, bh = mx.y - mn.y;
                Vec4 c(0, 1, 0, 0.4f);
                g_spriteRenderer->DrawRect(Vec2(mn.x + bw * 0.5f, mn.y + 1), Vec2(bw, 2), c);
                g_spriteRenderer->DrawRect(Vec2(mn.x + bw * 0.5f, mx.y - 1), Vec2(bw, 2), c);
                g_spriteRenderer->DrawRect(Vec2(mn.x + 1, mn.y + bh * 0.5f), Vec2(2, bh), c);
                g_spriteRenderer->DrawRect(Vec2(mx.x - 1, mn.y + bh * 0.5f), Vec2(2, bh), c);
            }
        }
        
        // 玩家hitbox（红色线框）
        for (auto& box : g_player.combat.hitboxes.boxes) {
            if (box.type == HitboxType::Hitbox && box.active) {
                AABB aabb = box.GetWorldAABB(g_player.position, g_player.facingRight);
                Vec2 mn = aabb.GetMin(), mx = aabb.GetMax();
                float bw = mx.x - mn.x, bh = mx.y - mn.y;
                Vec4 c(1, 0, 0, 0.7f);
                g_spriteRenderer->DrawRect(Vec2(mn.x + bw * 0.5f, mn.y + 1), Vec2(bw, 2), c);
                g_spriteRenderer->DrawRect(Vec2(mn.x + bw * 0.5f, mx.y - 1), Vec2(bw, 2), c);
                g_spriteRenderer->DrawRect(Vec2(mn.x + 1, mn.y + bh * 0.5f), Vec2(2, bh), c);
                g_spriteRenderer->DrawRect(Vec2(mx.x - 1, mn.y + bh * 0.5f), Vec2(2, bh), c);
            }
        }
        
        // 假人hurtbox（蓝色线框）
        for (auto& box : g_dummy.combat.hitboxes.boxes) {
            if (box.type == HitboxType::Hurtbox && box.active) {
                AABB aabb = box.GetWorldAABB(g_dummy.position, g_dummy.facingRight);
                Vec2 mn = aabb.GetMin(), mx = aabb.GetMax();
                float bw = mx.x - mn.x, bh = mx.y - mn.y;
                Vec4 c(0, 0.5f, 1, 0.4f);
                g_spriteRenderer->DrawRect(Vec2(mn.x + bw * 0.5f, mn.y + 1), Vec2(bw, 2), c);
                g_spriteRenderer->DrawRect(Vec2(mn.x + bw * 0.5f, mx.y - 1), Vec2(bw, 2), c);
                g_spriteRenderer->DrawRect(Vec2(mn.x + 1, mn.y + bh * 0.5f), Vec2(2, bh), c);
                g_spriteRenderer->DrawRect(Vec2(mx.x - 1, mn.y + bh * 0.5f), Vec2(2, bh), c);
            }
        }
    }
    
    g_spriteRenderer->End();
}

// ============================================================================
// ImGui调试面板
// ============================================================================

void RenderImGui() {
    ImGui::Begin("Combat Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    // 玩家信息
    ImGui::Text("=== Player ===");
    ImGui::Text("Position: (%.0f, %.0f)", g_player.position.x, g_player.position.y);
    ImGui::Text("Grounded: %s", g_player.grounded ? "Yes" : "No");
    ImGui::Text("State: %s",
        g_player.combat.state == CombatState::Idle ? "Idle" :
        g_player.combat.state == CombatState::Attacking ? "Attacking" :
        g_player.combat.state == CombatState::Recovery ? "Recovery" : "Other");
    ImGui::Text("Animation: %s [%d]", g_player.currentAnim.c_str(), g_player.animFrame);
    ImGui::Text("Attack Active: %s", g_player.combat.IsAttackActive() ? "YES" : "no");
    
    ImGui::ProgressBar(g_player.combat.stats.HealthPercent(), ImVec2(200, 20), "Player HP");
    
    ImGui::Separator();
    
    // 假人信息
    ImGui::Text("=== Dummy ===");
    ImGui::Text("Position: (%.0f, %.0f)", g_dummy.position.x, g_dummy.position.y);
    ImGui::Text("State: %s",
        g_dummy.combat.state == CombatState::Idle ? "Idle" :
        g_dummy.combat.state == CombatState::Stunned ? "Stunned" :
        g_dummy.combat.state == CombatState::Dead ? "DEAD" : "Other");
    ImGui::Text("Invincible: %s", g_dummy.combat.IsInvincible() ? "YES" : "no");
    
    float hpPercent = g_dummy.combat.stats.HealthPercent();
    ImVec4 hpColor = hpPercent > 0.5f ? ImVec4(0, 1, 0, 1) :
                     hpPercent > 0.2f ? ImVec4(1, 1, 0, 1) : ImVec4(1, 0, 0, 1);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpColor);
    char hpText[32];
    snprintf(hpText, sizeof(hpText), "%d / %d",
             g_dummy.combat.stats.currentHealth, g_dummy.combat.stats.maxHealth);
    ImGui::ProgressBar(hpPercent, ImVec2(200, 20), hpText);
    ImGui::PopStyleColor();
    
    ImGui::Separator();
    ImGui::Checkbox("Show Hitboxes (Tab)", &g_showHitboxes);
    if (ImGui::Button("Reset Dummy (R)")) {
        g_dummy.combat.stats.FullHeal();
        g_dummy.combat.state = CombatState::Idle;
        g_dummy.position = Vec2(500, 86);
        g_dummy.knockbackVelocity = Vec2(0, 0);
    }
    
    ImGui::End();
}

// ============================================================================
// 清理
// ============================================================================

void Cleanup() {
    if (g_audio) g_audio->Shutdown();
    g_tilemap.reset();
    g_spriteRenderer.reset();
    g_renderer.reset();
    g_tilesetTex.reset();
    g_playerIdleTex.reset();
    g_playerRunTex.reset();
    g_playerPunchTex.reset();
    g_audio.reset();
}

// ============================================================================
// 主函数 - 使用Engine回调模式（与其他章节一致）
// ============================================================================

int main() {
    std::cout << "=== Example 14: Combat System ===" << std::endl;
    
    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth = 800;
    cfg.windowHeight = 600;
    cfg.windowTitle = "Mini Engine - Combat System";
    
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
