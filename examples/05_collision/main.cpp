/**
 * 示例05：2D碰撞检测
 * 
 * ============================================================================
 * 学习目标
 * ============================================================================
 * 
 * 1. 理解AABB和圆形碰撞体
 * 2. 实现碰撞检测和响应
 * 3. 理解穿透深度和法线
 * 
 * ============================================================================
 * 操作说明
 * ============================================================================
 * 
 * WASD        - 移动玩家（蓝色方块）
 * 空格        - 切换玩家碰撞形状（方块/圆形）
 * R           - 重置位置
 * ESC         - 退出
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
// 游戏对象
// ============================================================================

enum class ColliderType {
    Box,
    Circle
};

struct GameObject {
    Vec2 position;
    Vec2 size;
    float radius;
    Vec4 color;
    ColliderType type;
    bool isStatic;
    bool isColliding;
    
    AABB GetAABB() const {
        return AABB(position, size * 0.5f);
    }
    
    Circle GetCircle() const {
        return Circle(position, radius);
    }
};

// ============================================================================
// 全局变量
// ============================================================================

std::unique_ptr<Renderer2D> g_renderer;

// 玩家
GameObject g_player;

// 障碍物
std::vector<GameObject> g_obstacles;

// 调试信息
std::string g_collisionInfo;

// ============================================================================
// 初始化
// ============================================================================

bool Initialize() {
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) {
        return false;
    }
    
    // 初始化玩家
    g_player.position = Vec2(200, 360);
    g_player.size = Vec2(60, 60);
    g_player.radius = 30;
    g_player.color = Vec4(0.2f, 0.4f, 0.8f, 1.0f);
    g_player.type = ColliderType::Box;
    g_player.isStatic = false;
    g_player.isColliding = false;
    
    // 创建障碍物
    
    // 静态AABB
    GameObject box1;
    box1.position = Vec2(500, 300);
    box1.size = Vec2(100, 200);
    box1.radius = 50;
    box1.color = Vec4(0.8f, 0.3f, 0.2f, 1.0f);
    box1.type = ColliderType::Box;
    box1.isStatic = true;
    box1.isColliding = false;
    g_obstacles.push_back(box1);
    
    // 静态圆形
    GameObject circle1;
    circle1.position = Vec2(800, 400);
    circle1.size = Vec2(80, 80);
    circle1.radius = 50;
    circle1.color = Vec4(0.2f, 0.8f, 0.3f, 1.0f);
    circle1.type = ColliderType::Circle;
    circle1.isStatic = true;
    circle1.isColliding = false;
    g_obstacles.push_back(circle1);
    
    // 更多障碍物
    GameObject box2;
    box2.position = Vec2(640, 550);
    box2.size = Vec2(200, 40);
    box2.radius = 40;
    box2.color = Vec4(0.8f, 0.6f, 0.2f, 1.0f);
    box2.type = ColliderType::Box;
    box2.isStatic = true;
    g_obstacles.push_back(box2);
    
    GameObject circle2;
    circle2.position = Vec2(1000, 250);
    circle2.size = Vec2(60, 60);
    circle2.radius = 40;
    circle2.color = Vec4(0.8f, 0.2f, 0.8f, 1.0f);
    circle2.type = ColliderType::Circle;
    circle2.isStatic = true;
    g_obstacles.push_back(circle2);
    
    // 边界墙壁
    GameObject ground;
    ground.position = Vec2(640, 30);
    ground.size = Vec2(1200, 60);
    ground.color = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
    ground.type = ColliderType::Box;
    ground.isStatic = true;
    g_obstacles.push_back(ground);
    
    GameObject ceiling;
    ceiling.position = Vec2(640, 690);
    ceiling.size = Vec2(1200, 60);
    ceiling.color = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
    ceiling.type = ColliderType::Box;
    ceiling.isStatic = true;
    g_obstacles.push_back(ceiling);
    
    GameObject leftWall;
    leftWall.position = Vec2(30, 360);
    leftWall.size = Vec2(60, 600);
    leftWall.color = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
    leftWall.type = ColliderType::Box;
    leftWall.isStatic = true;
    g_obstacles.push_back(leftWall);
    
    GameObject rightWall;
    rightWall.position = Vec2(1250, 360);
    rightWall.size = Vec2(60, 600);
    rightWall.color = Vec4(0.4f, 0.4f, 0.4f, 1.0f);
    rightWall.type = ColliderType::Box;
    rightWall.isStatic = true;
    g_obstacles.push_back(rightWall);
    
    std::cout << "[Example05] Initialized with " << g_obstacles.size() << " obstacles." << std::endl;
    return true;
}

// ============================================================================
// 碰撞检测和响应
// ============================================================================

void ResolveCollisions() {
    g_player.isColliding = false;
    g_collisionInfo.clear();
    
    for (auto& obs : g_obstacles) {
        obs.isColliding = false;
    }
    
    for (auto& obs : g_obstacles) {
        HitResult hit;
        
        if (g_player.type == ColliderType::Box && obs.type == ColliderType::Box) {
            hit = Collision::AABBvsAABB(g_player.GetAABB(), obs.GetAABB());
        }
        else if (g_player.type == ColliderType::Circle && obs.type == ColliderType::Circle) {
            hit = Collision::CirclevsCircle(g_player.GetCircle(), obs.GetCircle());
        }
        else if (g_player.type == ColliderType::Circle && obs.type == ColliderType::Box) {
            hit = Collision::CirclevsAABB(g_player.GetCircle(), obs.GetAABB());
        }
        else if (g_player.type == ColliderType::Box && obs.type == ColliderType::Circle) {
            hit = Collision::CirclevsAABB(obs.GetCircle(), g_player.GetAABB());
            if (hit.hit) {
                hit.normal = hit.normal * -1.0f;
            }
        }
        
        if (hit.hit) {
            g_player.isColliding = true;
            obs.isColliding = true;
            
            g_player.position += hit.normal * hit.penetration;
            
            char buf[256];
            snprintf(buf, sizeof(buf), 
                "Hit! Penetration: %.2f, Normal: (%.2f, %.2f)", 
                hit.penetration, hit.normal.x, hit.normal.y);
            g_collisionInfo = buf;
        }
    }
}

// ============================================================================
// 更新
// ============================================================================

void Update(float deltaTime) {
    Engine& engine = Engine::Instance();
    GLFWwindow* window = engine.GetWindow();
    
    float speed = 300.0f * deltaTime;
    Vec2 movement(0, 0);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement.y += speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement.y -= speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement.x -= speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement.x += speed;
    
    if (movement.x != 0 || movement.y != 0) {
        g_player.position += movement;
    }
    
    ResolveCollisions();
    
    static bool spaceWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spaceWasPressed) {
        g_player.type = (g_player.type == ColliderType::Box) ? ColliderType::Circle : ColliderType::Box;
    }
    spaceWasPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    
    static bool rWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rWasPressed) {
        g_player.position = Vec2(200, 360);
    }
    rWasPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
}

// ============================================================================
// 渲染辅助函数
// ============================================================================

// 将像素坐标转换为NDC (-1到1)
Vec2 PixelToNDC(const Vec2& pixel, int width, int height) {
    return Vec2(
        (pixel.x / width) * 2.0f - 1.0f,
        (pixel.y / height) * 2.0f - 1.0f
    );
}

float PixelToNDCX(float x, int width) {
    return (x / width) * 2.0f - 1.0f;
}

float PixelToNDCY(float y, int height) {
    return (y / height) * 2.0f - 1.0f;
}

void DrawAABBInNDC(Renderer2D* renderer, const AABB& aabb, const Vec4& color, int width, int height) {
    Vec2 min = aabb.GetMin();
    Vec2 max = aabb.GetMax();
    
    Vec2 ndcMin = PixelToNDC(min, width, height);
    Vec2 ndcMax = PixelToNDC(max, width, height);
    
    Vec2 p1(ndcMin.x, ndcMin.y);
    Vec2 p2(ndcMax.x, ndcMin.y);
    Vec2 p3(ndcMax.x, ndcMax.y);
    Vec2 p4(ndcMin.x, ndcMax.y);
    
    renderer->DrawTriangle(p1, p2, p3, color);
    renderer->DrawTriangle(p1, p3, p4, color);
}

void DrawCircleInNDC(Renderer2D* renderer, const Circle& circle, const Vec4& color, int width, int height) {
    const int segments = 32;
    Vec2 centerNDC = PixelToNDC(circle.center, width, height);
    float radiusNDCX = (circle.radius / width) * 2.0f;
    float radiusNDCY = (circle.radius / height) * 2.0f;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / segments * PI * 2.0f;
        float angle2 = (float)(i + 1) / segments * PI * 2.0f;
        
        Vec2 p1(centerNDC.x + std::cos(angle1) * radiusNDCX,
                centerNDC.y + std::sin(angle1) * radiusNDCY);
        Vec2 p2(centerNDC.x + std::cos(angle2) * radiusNDCX,
                centerNDC.y + std::sin(angle2) * radiusNDCY);
        
        renderer->DrawTriangle(centerNDC, p1, p2, color);
    }
}

// ============================================================================
// 渲染
// ============================================================================

void Render() {
    Engine& engine = Engine::Instance();
    int width = engine.GetWindowWidth();
    int height = engine.GetWindowHeight();
    
    // 绘制障碍物
    for (const auto& obs : g_obstacles) {
        Vec4 color = obs.color;
        if (obs.isColliding) {
            color = color * 1.5f;
            color.w = 1.0f;
        }
        
        if (obs.type == ColliderType::Box) {
            DrawAABBInNDC(g_renderer.get(), obs.GetAABB(), color, width, height);
        } else {
            DrawCircleInNDC(g_renderer.get(), obs.GetCircle(), color, width, height);
        }
    }
    
    // 绘制玩家
    Vec4 playerColor = g_player.color;
    if (g_player.isColliding) {
        playerColor = Vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }
    
    if (g_player.type == ColliderType::Box) {
        DrawAABBInNDC(g_renderer.get(), g_player.GetAABB(), playerColor, width, height);
    } else {
        DrawCircleInNDC(g_renderer.get(), g_player.GetCircle(), playerColor, width, height);
    }
}

// ============================================================================
// ImGui界面
// ============================================================================

void RenderImGui() {
    Engine& engine = Engine::Instance();
    
    ImGui::Begin("Collision Demo");
    {
        ImGui::Text("FPS: %.1f", engine.GetFPS());
        
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::BulletText("WASD  - Move player");
        ImGui::BulletText("Space - Toggle shape");
        ImGui::BulletText("R     - Reset");
        
        ImGui::Separator();
        ImGui::Text("Player:");
        ImGui::Text("  Position: (%.1f, %.1f)", g_player.position.x, g_player.position.y);
        ImGui::Text("  Shape: %s", g_player.type == ColliderType::Box ? "Box (AABB)" : "Circle");
        ImGui::Text("  Colliding: %s", g_player.isColliding ? "YES" : "No");
        
        if (!g_collisionInfo.empty()) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", g_collisionInfo.c_str());
        }
    }
    ImGui::End();
    
    ImGui::Begin("Collision Info");
    {
        ImGui::Text("AABB vs AABB:");
        ImGui::BulletText("Check overlap on X and Y axes");
        ImGui::BulletText("Fast and simple");
        
        ImGui::Separator();
        ImGui::Text("Circle vs Circle:");
        ImGui::BulletText("Compare distance to sum of radii");
        ImGui::BulletText("Use squared distance to avoid sqrt");
        
        ImGui::Separator();
        ImGui::Text("Collision Response:");
        ImGui::BulletText("Calculate penetration depth");
        ImGui::BulletText("Push out by: normal * penetration");
    }
    ImGui::End();
}

// ============================================================================
// 清理
// ============================================================================

void Cleanup() {
    g_obstacles.clear();
    g_renderer->Shutdown();
    g_renderer.reset();
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Mini Game Engine - Example 05" << std::endl;
    std::cout << "  2D Collision Detection" << std::endl;
    std::cout << "========================================" << std::endl;
    
    Engine& engine = Engine::Instance();
    
    EngineConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "Mini Engine - Collision Detection";
    
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
