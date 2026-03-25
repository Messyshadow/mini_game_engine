/**
 * 示例08：角色控制器
 * 
 * 演示专业级的2D平台游戏角色控制：
 * - 土狼时间（Coyote Time）
 * - 跳跃缓冲（Jump Buffer）
 * - 加速度曲线
 * - 可变高度跳跃
 * - 蹬墙跳
 * 
 * 操作说明：
 * - A/D: 左右移动
 * - 空格: 跳跃（长按跳更高）
 * - 贴墙+空格: 蹬墙跳
 * - 1: 开关土狼时间
 * - 2: 开关跳跃缓冲
 * - 3: 开关加速度曲线
 * - 4: 开关蹬墙跳
 * - R: 重置位置
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "physics/Physics.h"
#include "game/Game.h"
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
std::unique_ptr<CharacterController2D> g_controller;

// 玩家
PhysicsBody2D g_player;

// 地形
std::vector<PhysicsBody2D> g_terrain;

// 输入状态
bool g_jumpPressedLastFrame = false;

// 调试显示
bool g_showDebugTimers = true;
bool g_showDebugTrail = true;

// 轨迹记录（用于可视化）
struct TrailPoint {
    Vec2 position;
    float time;
    CharacterState state;
};
std::vector<TrailPoint> g_trail;
float g_trailTimer = 0.0f;

// ============================================================================
// 初始化
// ============================================================================

bool Initialize() {
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) {
        return false;
    }
    
    g_physics = std::make_unique<PlatformPhysics>();
    g_physics->gravity = Vec2(0, -1500.0f);
    g_physics->maxFallSpeed = 800.0f;
    
    // 创建玩家
    g_player.position = Vec2(200, 200);
    g_player.size = Vec2(36, 56);
    g_player.bodyType = BodyType::Dynamic;
    g_player.gravityScale = 1.0f;
    g_player.layer = LAYER_PLAYER;
    g_player.collisionMask = LAYER_GROUND | LAYER_WALL | LAYER_PLATFORM;
    
    g_physics->AddBody(&g_player);
    
    // 创建角色控制器
    g_controller = std::make_unique<CharacterController2D>();
    g_controller->Initialize(&g_player, g_physics.get());
    
    // ========================================================================
    // 创建地形 - 设计用于测试各种跳跃技巧
    // ========================================================================
    
    // 主地面
    g_terrain.push_back(PlatformPhysics::CreateGround(0, 0, 1280, 40));
    
    // 左右墙壁
    g_terrain.push_back(PlatformPhysics::CreateWall(0, 0, 30, 720));
    g_terrain.push_back(PlatformPhysics::CreateWall(1250, 0, 30, 720));
    
    // 测试土狼时间的平台（有间隙）
    g_terrain.push_back(PlatformPhysics::CreateGround(100, 150, 180, 25));
    g_terrain.push_back(PlatformPhysics::CreateGround(350, 150, 180, 25));
    // 间隙约70像素，需要土狼时间才能轻松跳过
    
    // 测试跳跃缓冲的平台（需要精确落地跳）
    g_terrain.push_back(PlatformPhysics::CreateGround(600, 100, 100, 25));
    g_terrain.push_back(PlatformPhysics::CreateGround(750, 180, 100, 25));
    g_terrain.push_back(PlatformPhysics::CreateGround(900, 260, 100, 25));
    
    // 测试蹬墙跳的垂直通道
    g_terrain.push_back(PlatformPhysics::CreateWall(1050, 40, 30, 400));
    g_terrain.push_back(PlatformPhysics::CreateWall(1150, 40, 30, 400));
    // 两墙之间可以蹬墙跳上去
    
    // 高处平台
    g_terrain.push_back(PlatformPhysics::CreateGround(1000, 450, 220, 25));
    
    // 单向平台
    PhysicsBody2D oneWay1 = PlatformPhysics::CreateOneWayPlatform(200, 300, 150, 15);
    g_terrain.push_back(oneWay1);
    
    PhysicsBody2D oneWay2 = PlatformPhysics::CreateOneWayPlatform(450, 380, 150, 15);
    g_terrain.push_back(oneWay2);
    
    PhysicsBody2D oneWay3 = PlatformPhysics::CreateOneWayPlatform(700, 450, 150, 15);
    g_terrain.push_back(oneWay3);
    
    // 左侧蹬墙跳练习区
    g_terrain.push_back(PlatformPhysics::CreateWall(50, 300, 25, 300));
    g_terrain.push_back(PlatformPhysics::CreateGround(50, 600, 200, 25));
    
    // 添加所有地形到物理系统
    for (auto& terrain : g_terrain) {
        g_physics->AddBody(&terrain);
    }
    
    std::cout << "[Example08] Initialized - Character Controller Demo" << std::endl;
    std::cout << "  - Test Coyote Time: Jump after leaving platform" << std::endl;
    std::cout << "  - Test Jump Buffer: Jump before landing" << std::endl;
    std::cout << "  - Test Wall Jump: Jump between walls on the right" << std::endl;
    
    return true;
}

// ============================================================================
// 更新
// ============================================================================

void Update(float deltaTime) {
    Engine& engine = Engine::Instance();
    GLFWwindow* window = engine.GetWindow();
    
    // ========================================================================
    // 收集输入
    // ========================================================================
    
    CharacterInput input;
    
    // 水平输入
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) input.moveX = -1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) input.moveX = 1.0f;
    
    // 垂直输入
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) input.moveY = -1.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) input.moveY = 1.0f;
    
    // 跳跃输入
    bool jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    input.jumpPressed = jumpPressed && !g_jumpPressedLastFrame;
    input.jumpReleased = !jumpPressed && g_jumpPressedLastFrame;
    input.jumpHeld = jumpPressed;
    g_jumpPressedLastFrame = jumpPressed;
    
    // ========================================================================
    // 功能开关
    // ========================================================================
    
    static bool key1Last = false, key2Last = false, key3Last = false, key4Last = false;
    
    bool key1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    bool key2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
    bool key3 = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
    bool key4 = glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS;
    
    if (key1 && !key1Last) {
        g_controller->GetSettings().coyoteTimeEnabled = !g_controller->GetSettings().coyoteTimeEnabled;
    }
    if (key2 && !key2Last) {
        g_controller->GetSettings().jumpBufferEnabled = !g_controller->GetSettings().jumpBufferEnabled;
    }
    if (key3 && !key3Last) {
        g_controller->GetSettings().accelerationEnabled = !g_controller->GetSettings().accelerationEnabled;
    }
    if (key4 && !key4Last) {
        g_controller->GetSettings().wallJumpEnabled = !g_controller->GetSettings().wallJumpEnabled;
    }
    
    key1Last = key1; key2Last = key2; key3Last = key3; key4Last = key4;
    
    // 重置
    static bool rKeyLast = false;
    bool rKey = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (rKey && !rKeyLast) {
        g_player.position = Vec2(200, 200);
        g_player.velocity = Vec2::Zero();
        g_trail.clear();
    }
    rKeyLast = rKey;
    
    // ========================================================================
    // 更新控制器
    // ========================================================================
    
    g_controller->Update(input, deltaTime);
    
    // ========================================================================
    // 记录轨迹
    // ========================================================================
    
    g_trailTimer += deltaTime;
    if (g_trailTimer >= 0.05f) {
        g_trailTimer = 0.0f;
        
        TrailPoint point;
        point.position = g_player.position;
        point.time = (float)glfwGetTime();
        point.state = g_controller->GetState();
        g_trail.push_back(point);
        
        // 限制轨迹长度
        while (g_trail.size() > 100) {
            g_trail.erase(g_trail.begin());
        }
    }
    
    // ========================================================================
    // 边界检查
    // ========================================================================
    
    if (g_player.position.y < -100) {
        g_player.position = Vec2(200, 200);
        g_player.velocity = Vec2::Zero();
        g_trail.clear();
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
    
    // 绘制轨迹
    if (g_showDebugTrail && g_trail.size() > 1) {
        for (size_t i = 0; i < g_trail.size(); i++) {
            float alpha = (float)i / g_trail.size() * 0.5f;
            Vec4 color;
            
            switch (g_trail[i].state) {
                case CharacterState::Grounded:    color = Vec4(0.3f, 0.6f, 0.3f, alpha); break;
                case CharacterState::Jumping:     color = Vec4(0.3f, 0.8f, 0.3f, alpha); break;
                case CharacterState::Falling:     color = Vec4(0.8f, 0.3f, 0.3f, alpha); break;
                case CharacterState::WallSliding: color = Vec4(0.8f, 0.8f, 0.3f, alpha); break;
                case CharacterState::WallJumping: color = Vec4(0.8f, 0.3f, 0.8f, alpha); break;
                default: color = Vec4(0.5f, 0.5f, 0.5f, alpha);
            }
            
            DrawRect(g_renderer.get(), g_trail[i].position, Vec2(8, 8), color, width, height);
        }
    }
    
    // 绘制地形
    for (const auto& terrain : g_terrain) {
        Vec4 color;
        
        if (terrain.isOneWayPlatform) {
            color = Vec4(0.9f, 0.8f, 0.2f, 1.0f);
        } else if (terrain.layer == LAYER_WALL) {
            color = Vec4(0.4f, 0.4f, 0.5f, 1.0f);
        } else {
            color = Vec4(0.5f, 0.4f, 0.3f, 1.0f);
        }
        
        DrawRect(g_renderer.get(), terrain.position, terrain.size, color, width, height);
        
        // 单向平台顶部高亮
        if (terrain.isOneWayPlatform) {
            Vec2 topPos = Vec2(terrain.position.x, terrain.GetTop() - 2);
            Vec2 topSize = Vec2(terrain.size.x, 4);
            DrawRect(g_renderer.get(), topPos, topSize, Vec4(1.0f, 0.9f, 0.4f, 1.0f), width, height);
        }
    }
    
    // 绘制玩家
    Vec4 playerColor;
    switch (g_controller->GetState()) {
        case CharacterState::Grounded:    playerColor = Vec4(0.2f, 0.6f, 0.9f, 1.0f); break;
        case CharacterState::Jumping:     playerColor = Vec4(0.3f, 0.9f, 0.4f, 1.0f); break;
        case CharacterState::Falling:     playerColor = Vec4(0.9f, 0.4f, 0.3f, 1.0f); break;
        case CharacterState::WallSliding: playerColor = Vec4(0.9f, 0.8f, 0.2f, 1.0f); break;
        case CharacterState::WallJumping: playerColor = Vec4(0.9f, 0.3f, 0.9f, 1.0f); break;
        default: playerColor = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
    
    DrawRect(g_renderer.get(), g_player.position, g_player.size, playerColor, width, height);
    
    // 绘制面朝方向指示
    float eyeX = g_player.position.x + g_controller->GetFacingDirection() * 8;
    float eyeY = g_player.position.y + 12;
    DrawRect(g_renderer.get(), Vec2(eyeX, eyeY), Vec2(8, 10), Vec4(1, 1, 1, 1), width, height);
    
    // 绘制土狼时间/跳跃缓冲可视化
    if (g_showDebugTimers) {
        // 土狼时间条
        float coyoteRatio = g_controller->GetCoyoteTimeLeft() / g_controller->GetSettings().coyoteTime;
        if (coyoteRatio > 0) {
            Vec2 barPos = Vec2(g_player.position.x, g_player.GetTop() + 15);
            Vec2 barSize = Vec2(g_player.size.x * coyoteRatio, 4);
            DrawRect(g_renderer.get(), barPos, barSize, Vec4(0.2f, 0.9f, 0.9f, 0.8f), width, height);
        }
        
        // 跳跃缓冲条
        float bufferRatio = g_controller->GetJumpBufferLeft() / g_controller->GetSettings().jumpBufferTime;
        if (bufferRatio > 0) {
            Vec2 barPos = Vec2(g_player.position.x, g_player.GetTop() + 22);
            Vec2 barSize = Vec2(g_player.size.x * bufferRatio, 4);
            DrawRect(g_renderer.get(), barPos, barSize, Vec4(0.9f, 0.9f, 0.2f, 0.8f), width, height);
        }
    }
}

// ============================================================================
// ImGui界面
// ============================================================================

void RenderImGui() {
    Engine& engine = Engine::Instance();
    auto& settings = g_controller->GetSettings();
    
    ImGui::Begin("Character Controller");
    {
        ImGui::Text("FPS: %.1f", engine.GetFPS());
        
        ImGui::Separator();
        ImGui::Text("State: %s", g_controller->GetStateName());
        ImGui::Text("Last Jump: %s", g_controller->GetLastJumpType());
        
        ImGui::Separator();
        ImGui::Text("Velocity: (%.1f, %.1f)", g_player.velocity.x, g_player.velocity.y);
        ImGui::Text("Position: (%.1f, %.1f)", g_player.position.x, g_player.position.y);
        
        ImGui::Separator();
        ImGui::Text("Timers:");
        
        // 土狼时间进度条
        float coyoteRatio = g_controller->GetCoyoteTimeLeft() / settings.coyoteTime;
        ImGui::ProgressBar(coyoteRatio, ImVec2(-1, 0), "Coyote Time");
        
        // 跳跃缓冲进度条
        float bufferRatio = g_controller->GetJumpBufferLeft() / settings.jumpBufferTime;
        ImGui::ProgressBar(bufferRatio, ImVec2(-1, 0), "Jump Buffer");
        
        // 墙壁土狼时间
        if (settings.wallJumpEnabled) {
            float wallCoyoteRatio = g_controller->GetWallCoyoteTimeLeft() / settings.wallCoyoteTime;
            ImGui::ProgressBar(wallCoyoteRatio, ImVec2(-1, 0), "Wall Coyote");
        }
    }
    ImGui::End();
    
    ImGui::Begin("Feature Toggles");
    {
        ImGui::Text("Press number keys to toggle:");
        
        bool coyote = settings.coyoteTimeEnabled;
        bool buffer = settings.jumpBufferEnabled;
        bool accel = settings.accelerationEnabled;
        bool wall = settings.wallJumpEnabled;
        
        ImGui::Checkbox("[1] Coyote Time", &coyote);
        ImGui::Checkbox("[2] Jump Buffer", &buffer);
        ImGui::Checkbox("[3] Acceleration Curves", &accel);
        ImGui::Checkbox("[4] Wall Jump", &wall);
        
        settings.coyoteTimeEnabled = coyote;
        settings.jumpBufferEnabled = buffer;
        settings.accelerationEnabled = accel;
        settings.wallJumpEnabled = wall;
        
        ImGui::Separator();
        ImGui::Checkbox("Show Timer Bars", &g_showDebugTimers);
        ImGui::Checkbox("Show Trail", &g_showDebugTrail);
        
        ImGui::Separator();
        if (ImGui::Button("Reset Position [R]")) {
            g_player.position = Vec2(200, 200);
            g_player.velocity = Vec2::Zero();
            g_trail.clear();
        }
    }
    ImGui::End();
    
    ImGui::Begin("Parameter Tuning");
    {
        if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Max Speed", &settings.maxSpeed, 100, 600);
            ImGui::SliderFloat("Ground Accel", &settings.groundAcceleration, 500, 5000);
            ImGui::SliderFloat("Ground Decel", &settings.groundDeceleration, 500, 5000);
            ImGui::SliderFloat("Air Accel", &settings.airAcceleration, 200, 3000);
            ImGui::SliderFloat("Air Decel", &settings.airDeceleration, 100, 2000);
        }
        
        if (ImGui::CollapsingHeader("Jump", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Jump Force", &settings.jumpForce, 300, 800);
            ImGui::SliderFloat("Jump Cut", &settings.jumpCutMultiplier, 0.1f, 1.0f);
            ImGui::SliderFloat("Fall Multiplier", &settings.fallMultiplier, 1.0f, 4.0f);
            ImGui::SliderFloat("Low Jump Multi", &settings.lowJumpMultiplier, 1.0f, 5.0f);
        }
        
        if (ImGui::CollapsingHeader("Timing")) {
            ImGui::SliderFloat("Coyote Time", &settings.coyoteTime, 0.0f, 0.3f);
            ImGui::SliderFloat("Jump Buffer", &settings.jumpBufferTime, 0.0f, 0.3f);
        }
        
        if (ImGui::CollapsingHeader("Wall Jump")) {
            ImGui::SliderFloat("Wall Slide Speed", &settings.wallSlideSpeed, 50, 300);
            ImGui::SliderFloat("Wall Jump X", &settings.wallJumpForceX, 200, 600);
            ImGui::SliderFloat("Wall Jump Y", &settings.wallJumpForceY, 300, 700);
            ImGui::SliderFloat("Wall Coyote", &settings.wallCoyoteTime, 0.0f, 0.2f);
        }
    }
    ImGui::End();
    
    ImGui::Begin("Controls");
    {
        ImGui::Text("A/D - Move");
        ImGui::Text("SPACE - Jump (hold for higher)");
        ImGui::Text("Wall + SPACE - Wall Jump");
        ImGui::Separator();
        ImGui::Text("1 - Toggle Coyote Time");
        ImGui::Text("2 - Toggle Jump Buffer");
        ImGui::Text("3 - Toggle Acceleration");
        ImGui::Text("4 - Toggle Wall Jump");
        ImGui::Text("R - Reset Position");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.9f, 1), "Cyan bar = Coyote Time");
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1), "Yellow bar = Jump Buffer");
    }
    ImGui::End();
}

// ============================================================================
// 清理
// ============================================================================

void Cleanup() {
    g_terrain.clear();
    g_trail.clear();
    g_controller.reset();
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
    std::cout << "  Mini Game Engine - Example 08" << std::endl;
    std::cout << "  Character Controller" << std::endl;
    std::cout << "========================================" << std::endl;
    
    Engine& engine = Engine::Instance();
    
    EngineConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "Mini Engine - Character Controller";
    
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
