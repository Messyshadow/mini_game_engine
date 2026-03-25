/**
 * 示例07：平台物理系统
 * 
 * 演示2D平台跳跃游戏的核心物理：
 * - 重力和下落
 * - 可变高度跳跃
 * - 地面/墙壁碰撞
 * - 单向平台
 * 
 * 操作说明：
 * - A/D: 左右移动
 * - 空格: 跳跃（长按跳更高）
 * - S+空格: 从单向平台下跳
 * - R: 重置位置
 * - 1/2/3: 调整重力强度
 * - G: 开关重力
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "physics/Physics.h"
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
std::unique_ptr<PlatformPhysics> g_physics;

// 玩家
PhysicsBody2D g_player;

// 静态地形
std::vector<PhysicsBody2D> g_terrain;

// 玩家控制参数
struct PlayerController {
    float moveSpeed = 300.0f;           // 移动速度
    float jumpForce = 550.0f;           // 跳跃力
    float jumpCutMultiplier = 0.5f;     // 松开跳跃键时的速度衰减
    
    bool isJumping = false;             // 是否在跳跃中
    bool jumpKeyHeld = false;           // 跳跃键是否按住
    bool wantDropDown = false;          // 是否想要下跳
    
    // 可变高度跳跃
    bool variableJumpEnabled = true;
    float lowJumpGravityMultiplier = 2.5f;  // 轻跳时额外重力倍数
} g_controller;

// 调试信息
struct DebugInfo {
    bool onGround = false;
    bool onWallLeft = false;
    bool onWallRight = false;
    bool hitCeiling = false;
    Vec2 lastVelocity;
    int collisionCount = 0;
} g_debug;

// 物理参数
bool g_gravityEnabled = true;
int g_gravityPreset = 1;  // 0=轻, 1=正常, 2=重

// ============================================================================
// 初始化
// ============================================================================

bool Initialize() {
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) {
        return false;
    }
    
    g_physics = std::make_unique<PlatformPhysics>();
    
    // 设置重力
    g_physics->gravity = Vec2(0, -1500.0f);
    g_physics->maxFallSpeed = 800.0f;
    
    // 创建玩家
    g_player.position = Vec2(200, 300);
    g_player.size = Vec2(40, 60);
    g_player.bodyType = BodyType::Dynamic;
    g_player.gravityScale = 1.0f;
    g_player.layer = LAYER_PLAYER;
    g_player.collisionMask = LAYER_GROUND | LAYER_WALL | LAYER_PLATFORM;
    
    g_physics->AddBody(&g_player);
    
    // ========================================================================
    // 创建地形
    // ========================================================================
    
    // 主地面
    g_terrain.push_back(PlatformPhysics::CreateGround(0, 0, 1280, 40));
    
    // 左墙
    g_terrain.push_back(PlatformPhysics::CreateWall(0, 0, 30, 720));
    
    // 右墙
    g_terrain.push_back(PlatformPhysics::CreateWall(1250, 0, 30, 720));
    
    // 实心平台（从下面穿不过）
    g_terrain.push_back(PlatformPhysics::CreateGround(200, 150, 200, 30));
    g_terrain.push_back(PlatformPhysics::CreateGround(500, 250, 250, 30));
    g_terrain.push_back(PlatformPhysics::CreateGround(850, 180, 180, 30));
    
    // 单向平台（可以从下面跳上去）
    PhysicsBody2D platform1 = PlatformPhysics::CreateOneWayPlatform(150, 350, 150, 15);
    g_terrain.push_back(platform1);
    
    PhysicsBody2D platform2 = PlatformPhysics::CreateOneWayPlatform(400, 420, 200, 15);
    g_terrain.push_back(platform2);
    
    PhysicsBody2D platform3 = PlatformPhysics::CreateOneWayPlatform(700, 350, 150, 15);
    g_terrain.push_back(platform3);
    
    PhysicsBody2D platform4 = PlatformPhysics::CreateOneWayPlatform(950, 450, 180, 15);
    g_terrain.push_back(platform4);
    
    // 高处平台
    g_terrain.push_back(PlatformPhysics::CreateGround(300, 550, 300, 30));
    
    PhysicsBody2D platform5 = PlatformPhysics::CreateOneWayPlatform(700, 550, 200, 15);
    g_terrain.push_back(platform5);
    
    // 天花板测试
    g_terrain.push_back(PlatformPhysics::CreateGround(100, 500, 150, 20));
    
    // 添加所有地形到物理系统
    for (auto& terrain : g_terrain) {
        g_physics->AddBody(&terrain);
    }
    
    std::cout << "[Example07] Initialized with " << g_terrain.size() << " terrain pieces." << std::endl;
    std::cout << "  - Use A/D to move" << std::endl;
    std::cout << "  - Use SPACE to jump (hold for higher jump)" << std::endl;
    std::cout << "  - Use S+SPACE to drop through one-way platforms" << std::endl;
    
    return true;
}

// ============================================================================
// 更新
// ============================================================================

void Update(float deltaTime) {
    Engine& engine = Engine::Instance();
    GLFWwindow* window = engine.GetWindow();
    
    // ========================================================================
    // 输入处理
    // ========================================================================
    
    bool moveLeft = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool moveRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    bool jumpKey = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    bool downKey = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    
    // 检测跳跃键刚按下
    static bool jumpKeyWasPressed = false;
    bool jumpJustPressed = jumpKey && !jumpKeyWasPressed;
    bool jumpJustReleased = !jumpKey && jumpKeyWasPressed;
    jumpKeyWasPressed = jumpKey;
    
    // 重力调节
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        g_gravityPreset = 0;
        g_physics->gravity = Vec2(0, -800.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        g_gravityPreset = 1;
        g_physics->gravity = Vec2(0, -1500.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        g_gravityPreset = 2;
        g_physics->gravity = Vec2(0, -2500.0f);
    }
    
    // 开关重力
    static bool gKeyWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !gKeyWasPressed) {
        g_gravityEnabled = !g_gravityEnabled;
        g_player.gravityScale = g_gravityEnabled ? 1.0f : 0.0f;
    }
    gKeyWasPressed = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
    
    // 重置位置
    static bool rKeyWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rKeyWasPressed) {
        g_player.position = Vec2(200, 300);
        g_player.velocity = Vec2::Zero();
    }
    rKeyWasPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    
    // ========================================================================
    // 地面检测
    // ========================================================================
    
    g_debug.onGround = g_physics->IsOnGround(&g_player);
    g_debug.onWallLeft = g_physics->IsOnWall(&g_player, true);
    g_debug.onWallRight = g_physics->IsOnWall(&g_player, false);
    
    // ========================================================================
    // 水平移动
    // ========================================================================
    
    float targetVelocityX = 0.0f;
    if (moveLeft) targetVelocityX -= g_controller.moveSpeed;
    if (moveRight) targetVelocityX += g_controller.moveSpeed;
    
    // 直接设置速度（后续章节会改为加速度曲线）
    g_player.velocity.x = targetVelocityX;
    
    // ========================================================================
    // 跳跃处理
    // ========================================================================
    
    // 下跳穿过单向平台
    if (downKey && jumpJustPressed && g_debug.onGround) {
        // 检查是否站在单向平台上
        PhysicsBody2D* ground = g_physics->GetGroundBelow(g_player.position, 10.0f);
        if (ground && ground->isOneWayPlatform) {
            // 向下移动一点，脱离平台
            g_player.position.y -= 5.0f;
            g_controller.wantDropDown = true;
        }
    } else {
        g_controller.wantDropDown = false;
    }
    
    // 正常跳跃
    if (jumpJustPressed && g_debug.onGround && !g_controller.wantDropDown) {
        g_player.velocity.y = g_controller.jumpForce;
        g_controller.isJumping = true;
    }
    
    // 可变高度跳跃：松开跳跃键时减速
    if (g_controller.variableJumpEnabled) {
        if (jumpJustReleased && g_player.velocity.y > 0) {
            g_player.velocity.y *= g_controller.jumpCutMultiplier;
        }
        
        // 上升时如果不按跳跃键，增加额外重力
        if (g_player.velocity.y > 0 && !jumpKey && g_gravityEnabled) {
            g_player.velocity.y += g_physics->gravity.y * g_controller.lowJumpGravityMultiplier * deltaTime;
        }
    }
    
    g_controller.jumpKeyHeld = jumpKey;
    
    // 落地时重置跳跃状态
    if (g_debug.onGround && g_player.velocity.y <= 0) {
        g_controller.isJumping = false;
    }
    
    // ========================================================================
    // 物理更新
    // ========================================================================
    
    g_debug.lastVelocity = g_player.velocity;
    
    MoveResult result = g_physics->MoveAndCollide(&g_player, deltaTime);
    
    g_debug.hitCeiling = result.hitCeiling;
    g_debug.collisionCount = (int)result.collisions.size();
    
    // ========================================================================
    // 边界检查
    // ========================================================================
    
    if (g_player.position.y < -100) {
        // 掉出世界，重置
        g_player.position = Vec2(200, 300);
        g_player.velocity = Vec2::Zero();
    }
}

// ============================================================================
// 渲染辅助
// ============================================================================

Vec2 PixelToNDC(const Vec2& pixel, int width, int height) {
    return Vec2(
        (pixel.x / width) * 2.0f - 1.0f,
        (pixel.y / height) * 2.0f - 1.0f
    );
}

void DrawRect(Renderer2D* renderer, const Vec2& pos, const Vec2& size, const Vec4& color, int w, int h) {
    Vec2 min = PixelToNDC(pos - size * 0.5f, w, h);
    Vec2 max = PixelToNDC(pos + size * 0.5f, w, h);
    
    renderer->DrawTriangle(Vec2(min.x, min.y), Vec2(max.x, min.y), Vec2(max.x, max.y), color);
    renderer->DrawTriangle(Vec2(min.x, min.y), Vec2(max.x, max.y), Vec2(min.x, max.y), color);
}

// ============================================================================
// 渲染
// ============================================================================

void Render() {
    Engine& engine = Engine::Instance();
    int width = engine.GetWindowWidth();
    int height = engine.GetWindowHeight();
    
    // 绘制地形
    for (const auto& terrain : g_terrain) {
        Vec4 color;
        
        if (terrain.isOneWayPlatform) {
            // 单向平台：黄色，带虚线效果
            color = Vec4(0.9f, 0.8f, 0.2f, 1.0f);
        } else if (terrain.layer == LAYER_WALL) {
            // 墙壁：深灰色
            color = Vec4(0.4f, 0.4f, 0.45f, 1.0f);
        } else {
            // 地面：棕色
            color = Vec4(0.5f, 0.35f, 0.2f, 1.0f);
        }
        
        DrawRect(g_renderer.get(), terrain.position, terrain.size, color, width, height);
        
        // 单向平台顶部高亮
        if (terrain.isOneWayPlatform) {
            Vec2 topPos = Vec2(terrain.position.x, terrain.GetTop() - 2);
            Vec2 topSize = Vec2(terrain.size.x, 4);
            DrawRect(g_renderer.get(), topPos, topSize, Vec4(1.0f, 0.9f, 0.3f, 1.0f), width, height);
        }
    }
    
    // 绘制玩家
    Vec4 playerColor = Vec4(0.2f, 0.6f, 0.9f, 1.0f);
    
    // 根据状态改变颜色
    if (!g_debug.onGround) {
        if (g_player.velocity.y > 0) {
            playerColor = Vec4(0.3f, 0.8f, 0.4f, 1.0f);  // 上升：绿色
        } else {
            playerColor = Vec4(0.9f, 0.4f, 0.3f, 1.0f);  // 下落：红色
        }
    }
    
    if (g_debug.onWallLeft || g_debug.onWallRight) {
        playerColor = Vec4(0.8f, 0.5f, 0.9f, 1.0f);  // 贴墙：紫色
    }
    
    DrawRect(g_renderer.get(), g_player.position, g_player.size, playerColor, width, height);
    
    // 绘制玩家方向指示
    Vec2 eyeOffset = Vec2(g_player.velocity.x > 0 ? 8 : -8, 10);
    Vec2 eyePos = g_player.position + eyeOffset;
    DrawRect(g_renderer.get(), eyePos, Vec2(8, 8), Vec4(1, 1, 1, 1), width, height);
}

// ============================================================================
// ImGui界面
// ============================================================================

void RenderImGui() {
    Engine& engine = Engine::Instance();
    
    ImGui::Begin("Platformer Physics");
    {
        ImGui::Text("FPS: %.1f", engine.GetFPS());
        
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::BulletText("A/D - Move");
        ImGui::BulletText("SPACE - Jump (hold for higher)");
        ImGui::BulletText("S+SPACE - Drop through platform");
        ImGui::BulletText("R - Reset position");
        ImGui::BulletText("1/2/3 - Gravity presets");
        ImGui::BulletText("G - Toggle gravity");
        
        ImGui::Separator();
        ImGui::Text("Player State:");
        ImGui::Text("  Position: (%.1f, %.1f)", g_player.position.x, g_player.position.y);
        ImGui::Text("  Velocity: (%.1f, %.1f)", g_player.velocity.x, g_player.velocity.y);
        
        ImGui::Separator();
        ImGui::Text("Ground Detection:");
        ImGui::TextColored(g_debug.onGround ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1),
            "  On Ground: %s", g_debug.onGround ? "YES" : "NO");
        ImGui::TextColored(g_debug.onWallLeft ? ImVec4(0,1,0,1) : ImVec4(0.5f,0.5f,0.5f,1),
            "  Wall Left: %s", g_debug.onWallLeft ? "YES" : "NO");
        ImGui::TextColored(g_debug.onWallRight ? ImVec4(0,1,0,1) : ImVec4(0.5f,0.5f,0.5f,1),
            "  Wall Right: %s", g_debug.onWallRight ? "YES" : "NO");
        ImGui::TextColored(g_debug.hitCeiling ? ImVec4(1,1,0,1) : ImVec4(0.5f,0.5f,0.5f,1),
            "  Hit Ceiling: %s", g_debug.hitCeiling ? "YES" : "NO");
        
        ImGui::Separator();
        ImGui::Text("Physics Settings:");
        const char* gravityNames[] = {"Light (-800)", "Normal (-1500)", "Heavy (-2500)"};
        ImGui::Text("  Gravity: %s", gravityNames[g_gravityPreset]);
        ImGui::Text("  Gravity Enabled: %s", g_gravityEnabled ? "YES" : "NO");
        ImGui::Text("  Max Fall Speed: %.0f", g_physics->maxFallSpeed);
        
        ImGui::Separator();
        ImGui::Text("Jump Settings:");
        ImGui::SliderFloat("Jump Force", &g_controller.jumpForce, 300, 800);
        ImGui::SliderFloat("Move Speed", &g_controller.moveSpeed, 100, 500);
        ImGui::Checkbox("Variable Jump", &g_controller.variableJumpEnabled);
        if (g_controller.variableJumpEnabled) {
            ImGui::SliderFloat("Jump Cut", &g_controller.jumpCutMultiplier, 0.1f, 1.0f);
            ImGui::SliderFloat("Low Jump Gravity", &g_controller.lowJumpGravityMultiplier, 1.0f, 5.0f);
        }
    }
    ImGui::End();
    
    // 图例
    ImGui::Begin("Legend");
    {
        ImGui::TextColored(ImVec4(0.5f, 0.35f, 0.2f, 1.0f), "Brown: Solid Ground");
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.45f, 1.0f), "Gray: Walls");
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "Yellow: One-Way Platform");
        ImGui::Text("  (Can jump through from below)");
        
        ImGui::Separator();
        ImGui::Text("Player Colors:");
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 0.9f, 1.0f), "Blue: On Ground");
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.4f, 1.0f), "Green: Rising");
        ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.3f, 1.0f), "Red: Falling");
        ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.9f, 1.0f), "Purple: On Wall");
    }
    ImGui::End();
}

// ============================================================================
// 清理
// ============================================================================

void Cleanup() {
    g_terrain.clear();
    g_physics->Clear();
    g_physics.reset();
    g_renderer->Shutdown();
    g_renderer.reset();
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Mini Game Engine - Example 07" << std::endl;
    std::cout << "  Platformer Physics System" << std::endl;
    std::cout << "========================================" << std::endl;
    
    Engine& engine = Engine::Instance();
    
    EngineConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "Mini Engine - Platformer Physics";
    
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
