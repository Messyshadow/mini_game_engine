/**
 * ============================================================================
 * 示例10：纹理瓦片地图渲染 (Textured Tilemap Rendering)
 * ============================================================================
 * 
 * 本章学习目标：
 * ----------------------------------------------------------------------------
 * 1. 从tileset.png加载瓦片纹理图集
 * 2. 根据瓦片ID计算正确的UV坐标
 * 3. 使用SpriteRenderer批量渲染带纹理的瓦片
 * 4. 理解纹理图集(Texture Atlas)的工作原理
 * 
 * 核心概念：
 * ----------------------------------------------------------------------------
 * 
 * 【什么是Tileset（瓦片图集）？】
 * 
 *   tileset.png 是一张包含多个小图的大图：
 *   
 *   ┌────┬────┬────┬────┬────┬────┬────┬────┐
 *   │ 0  │ 1  │ 2  │ 3  │ 4  │ 5  │ 6  │ 7  │  ← 第0行
 *   ├────┼────┼────┼────┼────┼────┼────┼────┤
 *   │ 8  │ 9  │ 10 │ 11 │ 12 │ 13 │ 14 │ 15 │  ← 第1行
 *   ├────┼────┼────┼────┼────┼────┼────┼────┤
 *   │ 16 │ 17 │ 18 │ 19 │ 20 │ 21 │ 22 │ 23 │  ← 第2行
 *   ├────┼────┼────┼────┼────┼────┼────┼────┤
 *   │ 24 │ 25 │ 26 │ 27 │ 28 │ 29 │ 30 │ 31 │  ← 第3行
 *   └────┴────┴────┴────┴────┴────┴────┴────┘
 *         每个格子是一个瓦片（32x32像素）
 * 
 * 【UV坐标计算】
 * 
 *   UV坐标范围是0.0到1.0，表示纹理的相对位置。
 *   对于tileset中的某个瓦片，需要计算它在整张图中的UV范围。
 *   
 *   例如：tileset是256x128像素，8列x4行
 *   
 *   tileID = 9 (第1行第1列，从0开始计数)
 *   列号 = 9 % 8 = 1
 *   行号 = 9 / 8 = 1
 *   
 *   uvMinX = 1 * 32 / 256 = 0.125
 *   uvMaxX = 2 * 32 / 256 = 0.25
 *   uvMinY = 1 - (2 * 32 / 128) = 0.5  (Y轴翻转)
 *   uvMaxY = 1 - (1 * 32 / 128) = 0.75
 * 
 * 文件依赖：
 * ----------------------------------------------------------------------------
 * - data/texture/tileset.png : 256x128像素，8x4瓦片，每个32x32
 * 
 * 操作说明：
 * ----------------------------------------------------------------------------
 * - A/D     : 左右移动
 * - 空格    : 跳跃
 * - 方向键  : 手动移动摄像机
 * - G       : 显示/隐藏网格
 * - C       : 显示/隐藏碰撞层
 * - T       : 切换纹理/彩色渲染
 * - 1/2/3   : 切换图层可见性
 * - R       : 重置玩家位置
 * - E       : 编辑模式
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "engine/SpriteRenderer.h"
#include "engine/Texture.h"
#include "physics/Physics.h"
#include "game/Game.h"
#include "tilemap/TilemapSystem.h"
#include "math/Math.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 全局变量
// ============================================================================

std::unique_ptr<Renderer2D> g_renderer;
std::unique_ptr<SpriteRenderer> g_spriteRenderer;
std::unique_ptr<Tilemap> g_tilemap;
std::unique_ptr<TilemapRenderer> g_tilemapRenderer;
std::unique_ptr<Tileset> g_tileset;
std::shared_ptr<Texture> g_tilesetTexture;
std::unique_ptr<PlatformPhysics> g_physics;

PhysicsBody2D g_player;
Vec2 g_cameraPos(0, 0);
bool g_cameraFollowPlayer = true;
bool g_useTextureRendering = true;
bool g_editMode = false;
int g_selectedTile = 9;
int g_selectedLayer = 1;
bool g_jumpPressedLastFrame = false;
bool g_isGrounded = false;

// ============================================================================
// UV坐标计算
// ============================================================================

/**
 * 根据瓦片ID计算UV坐标
 * 
 * @param tileID  瓦片ID（1开始，0表示空）
 * @param cols    tileset列数
 * @param rows    tileset行数
 * @param uvMin   输出UV左下角
 * @param uvMax   输出UV右上角
 */
void CalculateTileUV(int tileID, int cols, int rows, Vec2& uvMin, Vec2& uvMax) {
    if (tileID <= 0) {
        uvMin = uvMax = Vec2(0, 0);
        return;
    }
    
    int index = tileID - 1;
    int col = index % cols;
    int row = index / cols;
    
    float tileU = 1.0f / cols;
    float tileV = 1.0f / rows;
    
    // Y轴翻转（OpenGL纹理坐标原点在左下）
    uvMin.x = col * tileU;
    uvMin.y = 1.0f - (row + 1) * tileV;
    uvMax.x = (col + 1) * tileU;
    uvMax.y = 1.0f - row * tileV;
}

// ============================================================================
// 纹理瓦片渲染
// ============================================================================

void RenderTexturedTilemap(float cameraX, float cameraY, int screenW, int screenH) {
    if (!g_tilemap || !g_tileset || !g_spriteRenderer) return;
    
    int tileW = g_tilemap->GetTileWidth();
    int tileH = g_tilemap->GetTileHeight();
    int cols = g_tileset->GetColumns();
    int rows = g_tileset->GetRows();
    
    g_spriteRenderer->Begin();
    
    for (int layerIdx = 0; layerIdx < g_tilemap->GetLayerCount(); layerIdx++) {
        const TilemapLayer* layer = g_tilemap->GetLayer(layerIdx);
        if (!layer || !layer->visible) continue;
        
        // 可见范围裁剪
        int startX = std::max(0, (int)(cameraX / tileW) - 1);
        int startY = std::max(0, (int)(cameraY / tileH) - 1);
        int endX = std::min(layer->width - 1, (int)((cameraX + screenW) / tileW) + 1);
        int endY = std::min(layer->height - 1, (int)((cameraY + screenH) / tileH) + 1);
        
        for (int y = startY; y <= endY; y++) {
            for (int x = startX; x <= endX; x++) {
                int tileID = layer->GetTile(x, y);
                if (tileID == 0) continue;
                
                float screenX = x * tileW - cameraX;
                float screenY = y * tileH - cameraY;
                
                Vec2 uvMin, uvMax;
                CalculateTileUV(tileID, cols, rows, uvMin, uvMax);
                
                Sprite tile;
                tile.SetTexture(g_tileset->GetTexture());
                tile.SetPosition(Vec2(screenX + tileW * 0.5f, screenY + tileH * 0.5f));
                tile.SetSize(Vec2((float)tileW, (float)tileH));
                tile.SetUV(uvMin, uvMax);
                tile.SetColor(Vec4(1, 1, 1, layer->opacity));
                
                g_spriteRenderer->DrawSprite(tile);
            }
        }
    }
    
    g_spriteRenderer->End();
}

// ============================================================================
// 创建地图
// ============================================================================

void CreateSampleMap() {
    g_tilemap = std::make_unique<Tilemap>();
    g_tilemap->Create(40, 20, 32);
    
    int bgLayer = g_tilemap->AddLayer("background", false);
    int groundLayer = g_tilemap->AddLayer("ground", true);
    int decorLayer = g_tilemap->AddLayer("decoration", false);
    
    // 背景
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 40; x++) {
            g_tilemap->SetTile(bgLayer, x, y, 1 + std::min(y / 5, 7));
        }
    }
    
    // 地面
    for (int x = 0; x < 40; x++) {
        g_tilemap->SetTile(groundLayer, x, 0, 11);
        g_tilemap->SetTile(groundLayer, x, 1, 9);
    }
    
    // 平台
    for (int x = 5; x < 10; x++) g_tilemap->SetTile(groundLayer, x, 5, 9);
    for (int x = 14; x < 20; x++) g_tilemap->SetTile(groundLayer, x, 8, 9);
    for (int x = 24; x < 32; x++) g_tilemap->SetTile(groundLayer, x, 11, 9);
    
    // 楼梯
    for (int i = 0; i < 5; i++) {
        g_tilemap->SetTile(groundLayer, 10 + i, 2 + i, 17);
    }
    
    // 墙壁
    for (int y = 2; y < 18; y++) {
        g_tilemap->SetTile(groundLayer, 0, y, 17);
        g_tilemap->SetTile(groundLayer, 39, y, 17);
    }
    
    // 装饰
    g_tilemap->SetTile(decorLayer, 7, 6, 29);
    g_tilemap->SetTile(decorLayer, 17, 9, 30);
    g_tilemap->SetTile(decorLayer, 28, 12, 29);
    g_tilemap->SetTile(decorLayer, 15, 2, 27);
    g_tilemap->SetTile(decorLayer, 25, 2, 27);
    
    MapObject spawn;
    spawn.name = "player_spawn";
    spawn.x = 100; spawn.y = 100;
    g_tilemap->AddObject(spawn);
    
    std::cout << "[Example10] Map created: " << g_tilemap->GetWidth() << "x" 
              << g_tilemap->GetHeight() << std::endl;
}

// ============================================================================
// Tilemap碰撞
// ============================================================================

struct CollisionResult {
    bool ground = false, ceiling = false, wallLeft = false, wallRight = false;
};

CollisionResult MoveWithCollision(PhysicsBody2D* body, Vec2 vel, float dt) {
    CollisionResult result;
    
    vel.y += -1500.0f * dt;
    if (vel.y < -800.0f) vel.y = -800.0f;
    body->velocity = vel;
    
    // X轴
    float dx = vel.x * dt;
    if (std::abs(dx) > 0.001f) {
        body->position.x += dx;
        AABB box = body->GetAABB();
        for (const AABB& tile : g_tilemap->GetCollidingTiles(box)) {
            if (box.Intersects(tile)) {
                Vec2 pen = box.GetPenetration(tile);
                if (dx > 0) { body->position.x -= std::abs(pen.x); result.wallRight = true; }
                else { body->position.x += std::abs(pen.x); result.wallLeft = true; }
                body->velocity.x = 0;
                box = body->GetAABB();
            }
        }
    }
    
    // Y轴
    float dy = vel.y * dt;
    if (std::abs(dy) > 0.001f) {
        body->position.y += dy;
        AABB box = body->GetAABB();
        for (const AABB& tile : g_tilemap->GetCollidingTiles(box)) {
            if (box.Intersects(tile)) {
                Vec2 pen = box.GetPenetration(tile);
                if (dy < 0) { body->position.y += std::abs(pen.y); result.ground = true; }
                else { body->position.y -= std::abs(pen.y); result.ceiling = true; }
                body->velocity.y = 0;
                box = body->GetAABB();
            }
        }
    }
    
    return result;
}

// ============================================================================
// 初始化
// ============================================================================

bool Initialize() {
    std::cout << "[Example10] Initializing..." << std::endl;
    
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) return false;
    
    g_spriteRenderer = std::make_unique<SpriteRenderer>();
    if (!g_spriteRenderer->Initialize()) return false;
    
    // 加载tileset
    g_tilesetTexture = std::make_shared<Texture>();
    const char* paths[] = {
        "data/texture/tileset.png",
        "../data/texture/tileset.png",
        "../../data/texture/tileset.png"
    };
    
    bool loaded = false;
    for (const char* path : paths) {
        if (g_tilesetTexture->Load(path)) {
            std::cout << "[Example10] Loaded: " << path << " ("
                      << g_tilesetTexture->GetWidth() << "x"
                      << g_tilesetTexture->GetHeight() << ")" << std::endl;
            loaded = true;
            break;
        }
    }
    
    if (!loaded) {
        std::cerr << "[Example10] WARNING: tileset.png not found!" << std::endl;
        std::cerr << "[Example10] Place tileset.png (256x128, 8x4 tiles) in data/texture/" << std::endl;
        g_useTextureRendering = false;
        g_tilesetTexture->CreateSolidColor(Vec4(1, 1, 1, 1));
    }
    
    // 初始化tileset
    // 注意：tileset.png是256x160像素，8列x5行，每个瓦片32x32
    g_tileset = std::make_unique<Tileset>();
    g_tileset->Initialize(g_tilesetTexture, 32, 32);
    g_tileset->SetTileCount(8, 5);  // 8列x5行 = 40个瓦片
    
    CreateSampleMap();
    g_tilemapRenderer = std::make_unique<TilemapRenderer>();
    g_physics = std::make_unique<PlatformPhysics>();
    
    // 玩家
    g_player.position = Vec2(100, 100);
    g_player.size = Vec2(28, 44);
    g_player.velocity = Vec2::Zero();
    
    // 投影矩阵
    Engine& engine = Engine::Instance();
    g_spriteRenderer->SetProjection(Mat4::Ortho(
        0, (float)engine.GetWindowWidth(),
        0, (float)engine.GetWindowHeight(),
        -1, 1
    ));
    
    std::cout << "[Example10] Ready! Press T to toggle texture mode." << std::endl;
    return true;
}

// ============================================================================
// 更新
// ============================================================================

void Update(float dt) {
    Engine& engine = Engine::Instance();
    GLFWwindow* window = engine.GetWindow();
    
    if (dt > 0.1f) dt = 0.1f;
    
    // 快捷键
    static bool tLast = false, gLast = false, cLast = false, eLast = false;
    static bool k1Last = false, k2Last = false, k3Last = false;
    
    bool t = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
    bool g = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
    bool c = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
    bool e = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    bool k1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    bool k2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
    bool k3 = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
    
    if (t && !tLast) g_useTextureRendering = !g_useTextureRendering;
    if (g && !gLast) g_tilemapRenderer->showGrid = !g_tilemapRenderer->showGrid;
    if (c && !cLast) g_tilemapRenderer->showCollision = !g_tilemapRenderer->showCollision;
    if (e && !eLast) g_editMode = !g_editMode;
    
    if (k1 && !k1Last && g_tilemap->GetLayerCount() > 0) {
        auto* layer = g_tilemap->GetLayer(0);
        if (layer) layer->visible = !layer->visible;
    }
    if (k2 && !k2Last && g_tilemap->GetLayerCount() > 1) {
        auto* layer = g_tilemap->GetLayer(1);
        if (layer) layer->visible = !layer->visible;
    }
    if (k3 && !k3Last && g_tilemap->GetLayerCount() > 2) {
        auto* layer = g_tilemap->GetLayer(2);
        if (layer) layer->visible = !layer->visible;
    }
    
    tLast = t; gLast = g; cLast = c; eLast = e;
    k1Last = k1; k2Last = k2; k3Last = k3;
    
    // 玩家移动
    Vec2 vel = g_player.velocity;
    
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) vel.x = -350.0f;
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) vel.x = 350.0f;
    else { vel.x *= 0.85f; if (std::abs(vel.x) < 10) vel.x = 0; }
    
    bool jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (jump && !g_jumpPressedLastFrame && g_isGrounded) {
        vel.y = 600.0f;
        g_isGrounded = false;
    }
    g_jumpPressedLastFrame = jump;
    
    CollisionResult col = MoveWithCollision(&g_player, vel, dt);
    g_isGrounded = col.ground;
    
    // 重置
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        g_player.position = Vec2(100, 100);
        g_player.velocity = Vec2::Zero();
    }
    
    // 摄像机
    int screenW = engine.GetWindowWidth();
    int screenH = engine.GetWindowHeight();
    
    if (g_cameraFollowPlayer) {
        float targetX = g_player.position.x - screenW * 0.5f;
        float targetY = g_player.position.y - screenH * 0.5f;
        g_cameraPos.x += (targetX - g_cameraPos.x) * 5.0f * dt;
        g_cameraPos.y += (targetY - g_cameraPos.y) * 5.0f * dt;
    } else {
        float spd = 500.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) g_cameraPos.x -= spd;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) g_cameraPos.x += spd;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) g_cameraPos.y -= spd;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) g_cameraPos.y += spd;
    }
    
    float mapW = (float)g_tilemap->GetPixelWidth();
    float mapH = (float)g_tilemap->GetPixelHeight();
    g_cameraPos.x = std::max(0.0f, std::min(g_cameraPos.x, mapW - screenW));
    g_cameraPos.y = std::max(0.0f, std::min(g_cameraPos.y, mapH - screenH));
    
    // 编辑模式
    if (g_editMode) {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        float worldX = (float)mx + g_cameraPos.x;
        float worldY = (float)(screenH - my) + g_cameraPos.y;
        
        int tileX, tileY;
        g_tilemap->WorldToTile(worldX, worldY, tileX, tileY);
        
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (tileX >= 0 && tileX < g_tilemap->GetWidth() &&
                tileY >= 0 && tileY < g_tilemap->GetHeight()) {
                g_tilemap->SetTile(g_selectedLayer, tileX, tileY, g_selectedTile);
            }
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            if (tileX >= 0 && tileX < g_tilemap->GetWidth() &&
                tileY >= 0 && tileY < g_tilemap->GetHeight()) {
                g_tilemap->SetTile(g_selectedLayer, tileX, tileY, 0);
            }
        }
    }
}

// ============================================================================
// 渲染
// ============================================================================

void Render() {
    Engine& engine = Engine::Instance();
    int w = engine.GetWindowWidth();
    int h = engine.GetWindowHeight();
    
    // 瓦片地图
    if (g_useTextureRendering && g_tilesetTexture->IsValid()) {
        RenderTexturedTilemap(g_cameraPos.x, g_cameraPos.y, w, h);
    } else {
        g_tilemapRenderer->Render(g_renderer.get(), g_tilemap.get(),
                                   g_cameraPos.x, g_cameraPos.y, w, h);
    }
    
    // 网格和碰撞
    if (g_tilemapRenderer->showGrid || g_tilemapRenderer->showCollision) {
        int tileW = g_tilemap->GetTileWidth();
        int tileH = g_tilemap->GetTileHeight();
        
        if (g_tilemapRenderer->showGrid) {
            int startX = (int)(g_cameraPos.x / tileW);
            int endX = (int)((g_cameraPos.x + w) / tileW) + 2;
            int startY = (int)(g_cameraPos.y / tileH);
            int endY = (int)((g_cameraPos.y + h) / tileH) + 2;
            
            for (int x = startX; x <= endX; x++) {
                float sx = x * tileW - g_cameraPos.x;
                float ndcX = (sx / w) * 2.0f - 1.0f;
                g_renderer->DrawLine(Vec2(ndcX, -1), Vec2(ndcX, 1), Vec4(0.3f, 0.3f, 0.3f, 0.5f), 1);
            }
            for (int y = startY; y <= endY; y++) {
                float sy = y * tileH - g_cameraPos.y;
                float ndcY = (sy / h) * 2.0f - 1.0f;
                g_renderer->DrawLine(Vec2(-1, ndcY), Vec2(1, ndcY), Vec4(0.3f, 0.3f, 0.3f, 0.5f), 1);
            }
        }
        
        if (g_tilemapRenderer->showCollision) {
            int startX = std::max(0, (int)(g_cameraPos.x / tileW));
            int endX = std::min(g_tilemap->GetWidth() - 1, (int)((g_cameraPos.x + w) / tileW) + 1);
            int startY = std::max(0, (int)(g_cameraPos.y / tileH));
            int endY = std::min(g_tilemap->GetHeight() - 1, (int)((g_cameraPos.y + h) / tileH) + 1);
            
            for (int y = startY; y <= endY; y++) {
                for (int x = startX; x <= endX; x++) {
                    if (g_tilemap->IsSolid(x, y)) {
                        float sx = x * tileW - g_cameraPos.x;
                        float sy = y * tileH - g_cameraPos.y;
                        float ndcX = (sx / w) * 2.0f - 1.0f;
                        float ndcY = (sy / h) * 2.0f - 1.0f;
                        float ndcW = (float)tileW / w * 2.0f;
                        float ndcH = (float)tileH / h * 2.0f;
                        
                        Vec4 col(1, 0.3f, 0.3f, 0.3f);
                        g_renderer->DrawTriangle(Vec2(ndcX, ndcY), Vec2(ndcX + ndcW, ndcY), 
                                                  Vec2(ndcX + ndcW, ndcY + ndcH), col);
                        g_renderer->DrawTriangle(Vec2(ndcX, ndcY), Vec2(ndcX + ndcW, ndcY + ndcH), 
                                                  Vec2(ndcX, ndcY + ndcH), col);
                    }
                }
            }
        }
    }
    
    // 玩家
    float px = g_player.position.x - g_cameraPos.x;
    float py = g_player.position.y - g_cameraPos.y;
    float ndcX = (px / w) * 2.0f - 1.0f - (g_player.size.x / w);
    float ndcY = (py / h) * 2.0f - 1.0f - (g_player.size.y / h);
    float ndcW = g_player.size.x / w * 2.0f;
    float ndcH = g_player.size.y / h * 2.0f;
    
    Vec4 playerCol(0.2f, 0.6f, 0.9f, 1.0f);
    g_renderer->DrawTriangle(Vec2(ndcX, ndcY), Vec2(ndcX + ndcW, ndcY), 
                              Vec2(ndcX + ndcW, ndcY + ndcH), playerCol);
    g_renderer->DrawTriangle(Vec2(ndcX, ndcY), Vec2(ndcX + ndcW, ndcY + ndcH), 
                              Vec2(ndcX, ndcY + ndcH), playerCol);
    
    // 眼睛
    float eyeX = ndcX + ndcW * 0.65f;
    float eyeY = ndcY + ndcH * 0.7f;
    float eyeW = ndcW * 0.2f;
    float eyeH = ndcH * 0.15f;
    g_renderer->DrawTriangle(Vec2(eyeX, eyeY), Vec2(eyeX + eyeW, eyeY), 
                              Vec2(eyeX + eyeW, eyeY + eyeH), Vec4(1, 1, 1, 1));
    g_renderer->DrawTriangle(Vec2(eyeX, eyeY), Vec2(eyeX + eyeW, eyeY + eyeH), 
                              Vec2(eyeX, eyeY + eyeH), Vec4(1, 1, 1, 1));
}

// ============================================================================
// ImGui
// ============================================================================

void RenderImGui() {
    Engine& engine = Engine::Instance();
    
    ImGui::Begin("Textured Tilemap Demo");
    ImGui::Text("FPS: %.1f", engine.GetFPS());
    ImGui::Separator();
    
    ImGui::Text("Rendering: %s", g_useTextureRendering ? "TEXTURE" : "COLOR");
    ImGui::Text("Tileset: %dx%d tiles", g_tileset->GetColumns(), g_tileset->GetRows());
    
    ImGui::Separator();
    ImGui::Text("Player: (%.0f, %.0f)", g_player.position.x, g_player.position.y);
    ImGui::Text("Grounded: %s", g_isGrounded ? "Yes" : "No");
    
    int tx, ty;
    g_tilemap->WorldToTile(g_player.position.x, g_player.position.y, tx, ty);
    ImGui::Text("Tile: (%d, %d)", tx, ty);
    
    ImGui::Separator();
    ImGui::Text("Camera: (%.0f, %.0f)", g_cameraPos.x, g_cameraPos.y);
    ImGui::Checkbox("Follow Player", &g_cameraFollowPlayer);
    ImGui::End();
    
    ImGui::Begin("Display Options");
    ImGui::Checkbox("[T] Texture Mode", &g_useTextureRendering);
    ImGui::Checkbox("[G] Show Grid", &g_tilemapRenderer->showGrid);
    ImGui::Checkbox("[C] Show Collision", &g_tilemapRenderer->showCollision);
    
    ImGui::Separator();
    ImGui::Text("Layers:");
    for (int i = 0; i < g_tilemap->GetLayerCount(); i++) {
        auto* layer = g_tilemap->GetLayer(i);
        if (layer) {
            char label[64];
            snprintf(label, sizeof(label), "[%d] %s%s", i + 1, layer->name.c_str(),
                     layer->collision ? " (solid)" : "");
            ImGui::Checkbox(label, &layer->visible);
        }
    }
    ImGui::End();
    
    ImGui::Begin("Edit Mode");
    ImGui::Checkbox("[E] Edit Mode", &g_editMode);
    if (g_editMode) {
        ImGui::Separator();
        ImGui::SliderInt("Tile ID", &g_selectedTile, 1, 40);
        ImGui::SliderInt("Layer", &g_selectedLayer, 0, g_tilemap->GetLayerCount() - 1);
        ImGui::Text("Left Click: Place");
        ImGui::Text("Right Click: Delete");
    }
    ImGui::End();
    
    ImGui::Begin("Controls");
    ImGui::Text("A/D - Move");
    ImGui::Text("SPACE - Jump");
    ImGui::Text("Arrows - Camera (manual)");
    ImGui::Text("T - Toggle texture");
    ImGui::Text("G - Grid | C - Collision");
    ImGui::Text("1/2/3 - Toggle layers");
    ImGui::Text("E - Edit mode");
    ImGui::Text("R - Reset player");
    ImGui::End();
    
    // Tileset预览
    if (g_useTextureRendering && g_tilesetTexture->IsValid()) {
        ImGui::Begin("Tileset Preview");
        ImGui::Text("Size: %dx%d", g_tilesetTexture->GetWidth(), g_tilesetTexture->GetHeight());
        ImGui::Text("Tiles: %d (%dx%d)", g_tileset->GetTileCount(), 
                    g_tileset->GetColumns(), g_tileset->GetRows());
        
        // 显示tileset图片 (256x160)
        ImTextureID texID = (ImTextureID)(intptr_t)g_tilesetTexture->GetID();
        ImGui::Image(texID, ImVec2(256, 160));
        
        if (g_editMode) {
            ImGui::Text("Selected: Tile %d", g_selectedTile);
            int col = (g_selectedTile - 1) % g_tileset->GetColumns();
            int row = (g_selectedTile - 1) / g_tileset->GetColumns();
            ImGui::Text("Position: Col %d, Row %d", col, row);
        }
        ImGui::End();
    }
}

// ============================================================================
// 清理
// ============================================================================

void Cleanup() {
    g_physics.reset();
    g_tilemapRenderer.reset();
    g_tileset.reset();
    g_tilemap.reset();
    g_tilesetTexture.reset();
    g_spriteRenderer.reset();
    g_renderer.reset();
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Mini Game Engine - Example 10" << std::endl;
    std::cout << "  Textured Tilemap Rendering" << std::endl;
    std::cout << "========================================" << std::endl;
    
    Engine& engine = Engine::Instance();
    
    EngineConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "Mini Engine - Textured Tilemap";
    
    if (!engine.Initialize(config)) return -1;
    if (!Initialize()) { engine.Shutdown(); return -1; }
    
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    
    Cleanup();
    engine.Shutdown();
    return 0;
}
