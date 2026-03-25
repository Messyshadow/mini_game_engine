/**
 * 示例09：瓦片地图系统
 * 
 * 演示2D瓦片地图的核心功能：
 * - 多图层渲染
 * - 瓦片碰撞检测
 * - 相机跟随
 * - 地图编辑（简化版）
 * 
 * 操作说明：
 * - A/D: 左右移动
 * - 空格: 跳跃
 * - 方向键: 移动相机
 * - G: 显示/隐藏网格
 * - C: 显示/隐藏碰撞框
 * - 1-3: 切换图层可见性
 * - R: 重置位置
 * - 鼠标左键: 放置瓦片
 * - 鼠标右键: 删除瓦片
 * - 滚轮: 切换瓦片类型
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "physics/Physics.h"
#include "game/Game.h"
#include "tilemap/TilemapSystem.h"
#include "math/Math.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <vector>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 全局变量
// ============================================================================

std::unique_ptr<Renderer2D> g_renderer;
std::unique_ptr<Tilemap> g_tilemap;
std::unique_ptr<TilemapRenderer> g_tilemapRenderer;
std::unique_ptr<PlatformPhysics> g_physics;
std::unique_ptr<CharacterController2D> g_controller;

// 玩家
PhysicsBody2D g_player;

// 相机
Vec2 g_cameraPos(0, 0);
bool g_cameraFollowPlayer = true;

// 编辑模式
bool g_editMode = false;
int g_selectedTile = 1;
int g_selectedLayer = 0;

// 输入
bool g_jumpPressedLastFrame = false;

// ============================================================================
// 创建示例地图
// ============================================================================

void CreateSampleMap() {
    // 创建40x20的地图，每个瓦片32像素
    g_tilemap = std::make_unique<Tilemap>();
    g_tilemap->Create(40, 20, 32);
    
    // 添加图层
    int bgLayer = g_tilemap->AddLayer("background", false);    // 背景层（无碰撞）
    int groundLayer = g_tilemap->AddLayer("ground", true);     // 地面层（有碰撞）
    int decorLayer = g_tilemap->AddLayer("decoration", false); // 装饰层（无碰撞）
    
    // 填充背景（瓦片ID 1-3）
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 40; x++) {
            // 渐变背景
            int bgTile = 1 + (y / 7);
            g_tilemap->SetTile(bgLayer, x, y, bgTile);
        }
    }
    
    // 创建地面（瓦片ID 4-6）
    for (int x = 0; x < 40; x++) {
        // 底部地面
        g_tilemap->SetTile(groundLayer, x, 0, 4);
        g_tilemap->SetTile(groundLayer, x, 1, 5);
    }
    
    // 创建平台
    // 平台1
    for (int x = 5; x < 10; x++) {
        g_tilemap->SetTile(groundLayer, x, 5, 6);
    }
    
    // 平台2
    for (int x = 12; x < 18; x++) {
        g_tilemap->SetTile(groundLayer, x, 8, 6);
    }
    
    // 平台3
    for (int x = 20; x < 26; x++) {
        g_tilemap->SetTile(groundLayer, x, 6, 6);
    }
    
    // 平台4（高处）
    for (int x = 28; x < 35; x++) {
        g_tilemap->SetTile(groundLayer, x, 11, 6);
    }
    
    // 楼梯
    for (int i = 0; i < 5; i++) {
        g_tilemap->SetTile(groundLayer, 35 + i, 2 + i, 6);
    }
    
    // 左墙
    for (int y = 2; y < 15; y++) {
        g_tilemap->SetTile(groundLayer, 0, y, 4);
    }
    
    // 右墙
    for (int y = 2; y < 15; y++) {
        g_tilemap->SetTile(groundLayer, 39, y, 4);
    }
    
    // 添加一些装饰（瓦片ID 7-9）
    g_tilemap->SetTile(decorLayer, 7, 6, 7);
    g_tilemap->SetTile(decorLayer, 15, 9, 8);
    g_tilemap->SetTile(decorLayer, 23, 7, 7);
    g_tilemap->SetTile(decorLayer, 31, 12, 9);
    
    // 创建地图对象（玩家出生点）
    MapObject playerSpawn;
    playerSpawn.name = "player_spawn";
    playerSpawn.type = "spawn";
    playerSpawn.x = 100;
    playerSpawn.y = 100;
    g_tilemap->AddObject(playerSpawn);
    
    std::cout << "[Example09] Created sample tilemap: " 
              << g_tilemap->GetWidth() << "x" << g_tilemap->GetHeight() 
              << " tiles" << std::endl;
}

// ============================================================================
// 与Tilemap的碰撞检测
// ============================================================================

/**
 * 将Tilemap的碰撞瓦片转换为物理系统可用的AABB列表
 */
void UpdatePhysicsFromTilemap() {
    // 清除旧的静态碰撞体（除了玩家）
    g_physics->Clear();
    g_physics->AddBody(&g_player);
    
    // 这里我们不逐个添加每个瓦片作为物理体
    // 而是在角色移动时动态查询附近的碰撞瓦片
    // 这样更高效
}

/**
 * 自定义的Tilemap碰撞检测
 * 替代PlatformPhysics的碰撞检测
 */
struct TilemapCollisionResult {
    bool hitGround = false;
    bool hitCeiling = false;
    bool hitWallLeft = false;
    bool hitWallRight = false;
};

TilemapCollisionResult MoveWithTilemapCollision(PhysicsBody2D* body, 
                                                  const Tilemap* tilemap,
                                                  Vec2 velocity,
                                                  float deltaTime) {
    TilemapCollisionResult result;
    
    int tileSize = tilemap->GetTileWidth();
    
    // 应用重力
    velocity.y += -1500.0f * deltaTime;
    if (velocity.y < -800.0f) velocity.y = -800.0f;
    
    body->velocity = velocity;
    
    // 分轴移动
    
    // X轴移动
    float dx = velocity.x * deltaTime;
    if (std::abs(dx) > 0.001f) {
        body->position.x += dx;
        
        AABB bodyAABB = body->GetAABB();
        auto tiles = tilemap->GetCollidingTiles(bodyAABB);
        
        for (const AABB& tile : tiles) {
            if (bodyAABB.Intersects(tile)) {
                Vec2 pen = bodyAABB.GetPenetration(tile);
                
                if (dx > 0) {
                    body->position.x -= std::abs(pen.x);
                    result.hitWallRight = true;
                } else {
                    body->position.x += std::abs(pen.x);
                    result.hitWallLeft = true;
                }
                body->velocity.x = 0;
                bodyAABB = body->GetAABB();
            }
        }
    }
    
    // Y轴移动
    float dy = velocity.y * deltaTime;
    if (std::abs(dy) > 0.001f) {
        body->position.y += dy;
        
        AABB bodyAABB = body->GetAABB();
        auto tiles = tilemap->GetCollidingTiles(bodyAABB);
        
        for (const AABB& tile : tiles) {
            if (bodyAABB.Intersects(tile)) {
                Vec2 pen = bodyAABB.GetPenetration(tile);
                
                if (dy < 0) {
                    body->position.y += std::abs(pen.y);
                    result.hitGround = true;
                } else {
                    body->position.y -= std::abs(pen.y);
                    result.hitCeiling = true;
                }
                body->velocity.y = 0;
                bodyAABB = body->GetAABB();
            }
        }
    }
    
    return result;
}

// ============================================================================
// 初始化
// ============================================================================

bool Initialize() {
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) {
        return false;
    }
    
    // 创建示例地图
    CreateSampleMap();
    
    // 创建地图渲染器
    g_tilemapRenderer = std::make_unique<TilemapRenderer>();
    
    // 创建物理系统（用于角色控制器的部分功能）
    g_physics = std::make_unique<PlatformPhysics>();
    g_physics->gravity = Vec2(0, -1500.0f);
    
    // 创建玩家
    const MapObject* spawn = g_tilemap->GetObject("player_spawn");
    float spawnX = spawn ? spawn->x : 100.0f;
    float spawnY = spawn ? spawn->y : 100.0f;
    
    g_player.position = Vec2(spawnX, spawnY);
    g_player.size = Vec2(28, 48);
    g_player.bodyType = BodyType::Dynamic;
    g_player.gravityScale = 1.0f;
    g_player.layer = LAYER_PLAYER;
    
    g_physics->AddBody(&g_player);
    
    // 创建角色控制器
    g_controller = std::make_unique<CharacterController2D>();
    g_controller->Initialize(&g_player, g_physics.get());
    
    std::cout << "[Example09] Tilemap Demo Initialized" << std::endl;
    std::cout << "  - Use A/D to move, SPACE to jump" << std::endl;
    std::cout << "  - Press G to toggle grid" << std::endl;
    std::cout << "  - Press C to toggle collision display" << std::endl;
    std::cout << "  - Press E to toggle edit mode" << std::endl;
    
    return true;
}

// ============================================================================
// 更新
// ============================================================================

void Update(float deltaTime) {
    Engine& engine = Engine::Instance();
    GLFWwindow* window = engine.GetWindow();
    
    // ========================================================================
    // 功能开关
    // ========================================================================
    
    static bool gLast = false, cLast = false, eLast = false, rLast = false;
    
    bool gKey = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
    bool cKey = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
    bool eKey = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    bool rKey = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    
    if (gKey && !gLast) g_tilemapRenderer->showGrid = !g_tilemapRenderer->showGrid;
    if (cKey && !cLast) g_tilemapRenderer->showCollision = !g_tilemapRenderer->showCollision;
    if (eKey && !eLast) g_editMode = !g_editMode;
    if (rKey && !rLast) {
        g_player.position = Vec2(100, 100);
        g_player.velocity = Vec2::Zero();
    }
    
    gLast = gKey; cLast = cKey; eLast = eKey; rLast = rKey;
    
    // 图层可见性
    static bool k1Last = false, k2Last = false, k3Last = false;
    bool k1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    bool k2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
    bool k3 = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
    
    if (k1 && !k1Last && g_tilemap->GetLayer(0)) {
        TilemapLayer* layer = g_tilemap->GetLayer(0);
        layer->visible = !layer->visible;
    }
    if (k2 && !k2Last && g_tilemap->GetLayer(1)) {
        TilemapLayer* layer = g_tilemap->GetLayer(1);
        layer->visible = !layer->visible;
    }
    if (k3 && !k3Last && g_tilemap->GetLayer(2)) {
        TilemapLayer* layer = g_tilemap->GetLayer(2);
        layer->visible = !layer->visible;
    }
    
    k1Last = k1; k2Last = k2; k3Last = k3;
    
    // ========================================================================
    // 玩家输入和移动
    // ========================================================================
    
    if (!g_editMode) {
        float moveX = 0;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveX = -1;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveX = 1;
        
        bool jumpKey = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool jumpPressed = jumpKey && !g_jumpPressedLastFrame;
        g_jumpPressedLastFrame = jumpKey;
        
        // 地面检测（简化版）
        AABB footBox;
        footBox.center = Vec2(g_player.position.x, g_player.GetBottom() - 2);
        footBox.halfSize = Vec2(g_player.size.x * 0.4f, 2);
        bool onGround = g_tilemap->CheckCollision(footBox);
        
        // 应用移动
        Vec2 velocity = g_player.velocity;
        velocity.x = moveX * 280.0f;
        
        // 跳跃
        if (jumpPressed && onGround) {
            velocity.y = 500.0f;
        }
        
        // 与Tilemap碰撞
        TilemapCollisionResult collision = MoveWithTilemapCollision(
            &g_player, g_tilemap.get(), velocity, deltaTime);
        
        // 边界检查
        if (g_player.position.y < -100) {
            g_player.position = Vec2(100, 100);
            g_player.velocity = Vec2::Zero();
        }
    }
    
    // ========================================================================
    // 相机跟随
    // ========================================================================
    
    int screenW = engine.GetWindowWidth();
    int screenH = engine.GetWindowHeight();
    
    if (g_cameraFollowPlayer) {
        // 相机跟随玩家，保持玩家在屏幕中央
        float targetX = g_player.position.x - screenW * 0.5f;
        float targetY = g_player.position.y - screenH * 0.5f;
        
        // 平滑跟随
        g_cameraPos.x += (targetX - g_cameraPos.x) * 5.0f * deltaTime;
        g_cameraPos.y += (targetY - g_cameraPos.y) * 5.0f * deltaTime;
    } else {
        // 手动相机控制
        float camSpeed = 500.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) g_cameraPos.x -= camSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) g_cameraPos.x += camSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) g_cameraPos.y -= camSpeed;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) g_cameraPos.y += camSpeed;
    }
    
    // 相机边界限制
    float mapPixelW = (float)g_tilemap->GetPixelWidth();
    float mapPixelH = (float)g_tilemap->GetPixelHeight();
    
    g_cameraPos.x = std::max(0.0f, std::min(g_cameraPos.x, mapPixelW - screenW));
    g_cameraPos.y = std::max(0.0f, std::min(g_cameraPos.y, mapPixelH - screenH));
    
    // ========================================================================
    // 编辑模式
    // ========================================================================
    
    if (g_editMode) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        
        // 转换为世界坐标
        float worldX = (float)mouseX + g_cameraPos.x;
        float worldY = (float)(screenH - mouseY) + g_cameraPos.y;  // 翻转Y
        
        // 转换为瓦片坐标
        int tileX, tileY;
        g_tilemap->WorldToTile(worldX, worldY, tileX, tileY);
        
        // 鼠标左键放置瓦片
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (tileX >= 0 && tileX < g_tilemap->GetWidth() &&
                tileY >= 0 && tileY < g_tilemap->GetHeight()) {
                g_tilemap->SetTile(g_selectedLayer, tileX, tileY, g_selectedTile);
            }
        }
        
        // 鼠标右键删除瓦片
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
    int width = engine.GetWindowWidth();
    int height = engine.GetWindowHeight();
    
    // 渲染瓦片地图
    g_tilemapRenderer->Render(g_renderer.get(), g_tilemap.get(),
                               g_cameraPos.x, g_cameraPos.y,
                               width, height);
    
    // 渲染玩家
    float playerScreenX = g_player.position.x - g_cameraPos.x;
    float playerScreenY = g_player.position.y - g_cameraPos.y;
    
    float ndcX = (playerScreenX / width) * 2.0f - 1.0f;
    float ndcY = (playerScreenY / height) * 2.0f - 1.0f;
    float ndcW = g_player.size.x / width * 2.0f;
    float ndcH = g_player.size.y / height * 2.0f;
    
    // 玩家碰撞盒中心偏移
    ndcX -= ndcW * 0.5f;
    ndcY -= ndcH * 0.5f;
    
    Vec4 playerColor(0.2f, 0.6f, 0.9f, 1.0f);
    
    Vec2 p1(ndcX, ndcY);
    Vec2 p2(ndcX + ndcW, ndcY);
    Vec2 p3(ndcX + ndcW, ndcY + ndcH);
    Vec2 p4(ndcX, ndcY + ndcH);
    
    g_renderer->DrawTriangle(p1, p2, p3, playerColor);
    g_renderer->DrawTriangle(p1, p3, p4, playerColor);
    
    // 玩家眼睛
    float eyeNdcX = ndcX + ndcW * 0.65f;
    float eyeNdcY = ndcY + ndcH * 0.7f;
    float eyeW = ndcW * 0.2f;
    float eyeH = ndcH * 0.15f;
    
    g_renderer->DrawTriangle(
        Vec2(eyeNdcX, eyeNdcY),
        Vec2(eyeNdcX + eyeW, eyeNdcY),
        Vec2(eyeNdcX + eyeW, eyeNdcY + eyeH),
        Vec4(1, 1, 1, 1));
    g_renderer->DrawTriangle(
        Vec2(eyeNdcX, eyeNdcY),
        Vec2(eyeNdcX + eyeW, eyeNdcY + eyeH),
        Vec2(eyeNdcX, eyeNdcY + eyeH),
        Vec4(1, 1, 1, 1));
}

// ============================================================================
// ImGui界面
// ============================================================================

void RenderImGui() {
    Engine& engine = Engine::Instance();
    
    ImGui::Begin("Tilemap Demo");
    {
        ImGui::Text("FPS: %.1f", engine.GetFPS());
        
        ImGui::Separator();
        ImGui::Text("Map: %dx%d tiles", g_tilemap->GetWidth(), g_tilemap->GetHeight());
        ImGui::Text("Tile Size: %d px", g_tilemap->GetTileWidth());
        ImGui::Text("Layers: %d", g_tilemap->GetLayerCount());
        
        ImGui::Separator();
        ImGui::Text("Player:");
        ImGui::Text("  Position: (%.1f, %.1f)", g_player.position.x, g_player.position.y);
        ImGui::Text("  Velocity: (%.1f, %.1f)", g_player.velocity.x, g_player.velocity.y);
        
        int tileX, tileY;
        g_tilemap->WorldToTile(g_player.position.x, g_player.position.y, tileX, tileY);
        ImGui::Text("  Tile: (%d, %d)", tileX, tileY);
        
        ImGui::Separator();
        ImGui::Text("Camera: (%.1f, %.1f)", g_cameraPos.x, g_cameraPos.y);
        ImGui::Checkbox("Follow Player", &g_cameraFollowPlayer);
    }
    ImGui::End();
    
    ImGui::Begin("Display Options");
    {
        ImGui::Checkbox("[G] Show Grid", &g_tilemapRenderer->showGrid);
        ImGui::Checkbox("[C] Show Collision", &g_tilemapRenderer->showCollision);
        
        ImGui::Separator();
        ImGui::Text("Layer Visibility:");
        
        for (int i = 0; i < g_tilemap->GetLayerCount(); i++) {
            TilemapLayer* layer = g_tilemap->GetLayer(i);
            if (layer) {
                char label[64];
                snprintf(label, sizeof(label), "[%d] %s", i + 1, layer->name.c_str());
                ImGui::Checkbox(label, &layer->visible);
                
                if (layer->collision) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "(collision)");
                }
            }
        }
    }
    ImGui::End();
    
    ImGui::Begin("Edit Mode");
    {
        ImGui::Checkbox("[E] Edit Mode", &g_editMode);
        
        if (g_editMode) {
            ImGui::Separator();
            ImGui::Text("Left Click: Place tile");
            ImGui::Text("Right Click: Delete tile");
            
            ImGui::Separator();
            ImGui::SliderInt("Tile ID", &g_selectedTile, 1, 12);
            ImGui::SliderInt("Layer", &g_selectedLayer, 0, g_tilemap->GetLayerCount() - 1);
            
            TilemapLayer* layer = g_tilemap->GetLayer(g_selectedLayer);
            if (layer) {
                ImGui::Text("Editing: %s", layer->name.c_str());
            }
        }
    }
    ImGui::End();
    
    ImGui::Begin("Controls");
    {
        ImGui::Text("Movement: A/D");
        ImGui::Text("Jump: SPACE");
        ImGui::Text("Camera: Arrow keys (when not following)");
        ImGui::Separator();
        ImGui::Text("G - Toggle grid");
        ImGui::Text("C - Toggle collision");
        ImGui::Text("E - Toggle edit mode");
        ImGui::Text("1/2/3 - Toggle layers");
        ImGui::Text("R - Reset player");
    }
    ImGui::End();
}

// ============================================================================
// 清理
// ============================================================================

void Cleanup() {
    g_controller.reset();
    g_physics->Clear();
    g_physics.reset();
    g_tilemapRenderer.reset();
    g_tilemap.reset();
    g_renderer->Shutdown();
    g_renderer.reset();
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Mini Game Engine - Example 09" << std::endl;
    std::cout << "  Tilemap System" << std::endl;
    std::cout << "========================================" << std::endl;
    
    Engine& engine = Engine::Instance();
    
    EngineConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "Mini Engine - Tilemap System";
    
    if (!engine.Initialize(config)) {
        return -1;
    }
    
    if (!Initialize()) {
        engine.Shutdown();
        return -1;
    }
    
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    
    Cleanup();
    engine.Shutdown();
    
    return 0;
}
