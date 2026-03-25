/**
 * 示例04：纹理和精灵
 * 
 * ============================================================================
 * 学习目标
 * ============================================================================
 * 
 * 1. 理解纹理的加载和使用
 * 2. 理解精灵（Sprite）的概念
 * 3. 使用SpriteRenderer进行高效渲染
 * 4. 精灵动画基础（图集）
 * 
 * ============================================================================
 * 操作说明
 * ============================================================================
 * 
 * WASD   - 移动精灵
 * Q/E    - 旋转精灵
 * Z/X    - 缩放精灵
 * Space  - 切换动画帧
 * R      - 重置变换
 * ESC    - 退出
 */

#include "engine/Engine.h"
#include "engine/SpriteRenderer.h"
#include "engine/Texture.h"
#include "engine/Sprite.h"
#include "math/Math.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <vector>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 全局变量
// ============================================================================

// 渲染器
std::unique_ptr<SpriteRenderer> g_renderer;

// 纹理
std::shared_ptr<Texture> g_checkerTexture;  // 棋盘格纹理（程序生成）
std::shared_ptr<Texture> g_colorTexture;    // 彩色测试纹理

// 精灵
Sprite g_mainSprite;        // 主精灵
std::vector<Sprite> g_backgroundSprites;  // 背景精灵

// 投影矩阵
Mat4 g_projection;

// 动画
int g_currentFrame = 0;
float g_animTimer = 0.0f;
float g_animSpeed = 0.2f;  // 每帧时间（秒）
bool g_autoAnimate = true;

// ============================================================================
// 程序生成纹理
// ============================================================================

/**
 * 创建棋盘格纹理
 * 
 * 这是一个常用的测试纹理，可以清晰地看到：
 * - 纹理是否正确加载
 * - UV坐标是否正确
 * - 缩放和旋转是否正常
 */
std::shared_ptr<Texture> CreateCheckerTexture(int width, int height, int gridSize) {
    std::vector<unsigned char> pixels(width * height * 4);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // 计算棋盘格
            bool isWhite = ((x / gridSize) + (y / gridSize)) % 2 == 0;
            
            int index = (y * width + x) * 4;
            if (isWhite) {
                pixels[index + 0] = 255;  // R
                pixels[index + 1] = 255;  // G
                pixels[index + 2] = 255;  // B
            } else {
                pixels[index + 0] = 128;  // R
                pixels[index + 1] = 128;  // G
                pixels[index + 2] = 128;  // B
            }
            pixels[index + 3] = 255;  // A (不透明)
        }
    }
    
    auto texture = std::make_shared<Texture>();
    texture->LoadFromMemory(pixels.data(), width, height, 4);
    return texture;
}

/**
 * 创建彩色渐变纹理
 * 
 * 4x4的精灵图集模拟，每格不同颜色
 * 用于测试精灵图集功能
 */
std::shared_ptr<Texture> CreateColorGridTexture(int size) {
    int gridSize = size / 4;  // 4x4网格
    std::vector<unsigned char> pixels(size * size * 4);
    
    // 定义16种颜色
    Vec4 colors[16] = {
        Vec4(1, 0, 0, 1),     // 红
        Vec4(0, 1, 0, 1),     // 绿
        Vec4(0, 0, 1, 1),     // 蓝
        Vec4(1, 1, 0, 1),     // 黄
        Vec4(1, 0, 1, 1),     // 紫
        Vec4(0, 1, 1, 1),     // 青
        Vec4(1, 0.5f, 0, 1),  // 橙
        Vec4(0.5f, 0, 1, 1),  // 紫蓝
        Vec4(1, 1, 1, 1),     // 白
        Vec4(0.5f, 0.5f, 0.5f, 1),  // 灰
        Vec4(0.5f, 0, 0, 1),  // 暗红
        Vec4(0, 0.5f, 0, 1),  // 暗绿
        Vec4(0, 0, 0.5f, 1),  // 暗蓝
        Vec4(1, 0.8f, 0.6f, 1),  // 肤色
        Vec4(0.6f, 0.3f, 0, 1),  // 棕色
        Vec4(0, 0, 0, 1)      // 黑
    };
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // 计算属于哪个格子
            int gridX = x / gridSize;
            int gridY = y / gridSize;
            int colorIndex = gridY * 4 + gridX;
            
            Vec4 color = colors[colorIndex % 16];
            
            int index = (y * size + x) * 4;
            pixels[index + 0] = static_cast<unsigned char>(color.x * 255);
            pixels[index + 1] = static_cast<unsigned char>(color.y * 255);
            pixels[index + 2] = static_cast<unsigned char>(color.z * 255);
            pixels[index + 3] = static_cast<unsigned char>(color.w * 255);
        }
    }
    
    auto texture = std::make_shared<Texture>();
    texture->LoadFromMemory(pixels.data(), size, size, 4);
    // 使用最近邻过滤以保持像素清晰
    texture->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
    return texture;
}

// ============================================================================
// 初始化
// ============================================================================

bool Initialize() {
    // 创建渲染器
    g_renderer = std::make_unique<SpriteRenderer>();
    if (!g_renderer->Initialize()) {
        return false;
    }
    
    // 创建程序生成的纹理
    std::cout << "[Example] Creating procedural textures..." << std::endl;
    g_checkerTexture = CreateCheckerTexture(128, 128, 16);
    g_colorTexture = CreateColorGridTexture(64);
    
    // 设置主精灵
    g_mainSprite.SetTexture(g_colorTexture);
    g_mainSprite.SetPosition(640, 360);  // 屏幕中心
    g_mainSprite.SetSize(200, 200);
    g_mainSprite.SetPivotCenter();
    
    // 创建背景精灵（棋盘格）
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 7; x++) {
            Sprite bgSprite;
            bgSprite.SetTexture(g_checkerTexture);
            bgSprite.SetPosition(100.0f + x * 180.0f, 100.0f + y * 180.0f);
            bgSprite.SetSize(160, 160);
            bgSprite.SetPivotCenter();
            bgSprite.SetColor(Vec4(0.5f, 0.5f, 0.5f, 0.3f));  // 半透明
            g_backgroundSprites.push_back(bgSprite);
        }
    }
    
    std::cout << "[Example] Initialization complete." << std::endl;
    return true;
}

// ============================================================================
// 更新
// ============================================================================

void Update(float deltaTime) {
    Engine& engine = Engine::Instance();
    GLFWwindow* window = engine.GetWindow();
    
    // 获取当前精灵状态
    Vec2 pos = g_mainSprite.GetPosition();
    float rot = g_mainSprite.GetRotation();
    Vec2 scale = g_mainSprite.GetScale();
    
    // 移动速度
    float moveSpeed = 300.0f * deltaTime;
    float rotSpeed = 2.0f * deltaTime;
    float scaleSpeed = 1.0f * deltaTime;
    
    // WASD移动
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) pos.y += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) pos.y -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) pos.x -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) pos.x += moveSpeed;
    
    // Q/E旋转
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) rot += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) rot -= rotSpeed;
    
    // Z/X缩放
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        scale.x += scaleSpeed;
        scale.y += scaleSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        scale.x -= scaleSpeed;
        scale.y -= scaleSpeed;
        // 防止缩放为负
        if (scale.x < 0.1f) scale.x = 0.1f;
        if (scale.y < 0.1f) scale.y = 0.1f;
    }
    
    // R重置
    static bool rWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rWasPressed) {
        pos = Vec2(640, 360);
        rot = 0;
        scale = Vec2(1, 1);
        g_mainSprite.SetSize(200, 200);
    }
    rWasPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    
    // 应用变换
    g_mainSprite.SetPosition(pos);
    g_mainSprite.SetRotation(rot);
    g_mainSprite.SetScale(scale);
    
    // 动画更新
    if (g_autoAnimate) {
        g_animTimer += deltaTime;
        if (g_animTimer >= g_animSpeed) {
            g_animTimer = 0;
            g_currentFrame = (g_currentFrame + 1) % 16;  // 16帧循环
            g_mainSprite.SetFrameFromSheet(g_currentFrame, 4, 4);
        }
    }
    
    // 空格手动切换帧
    static bool spaceWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spaceWasPressed) {
        g_currentFrame = (g_currentFrame + 1) % 16;
        g_mainSprite.SetFrameFromSheet(g_currentFrame, 4, 4);
    }
    spaceWasPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
}

// ============================================================================
// 渲染
// ============================================================================

void Render() {
    Engine& engine = Engine::Instance();
    int width = engine.GetWindowWidth();
    int height = engine.GetWindowHeight();
    
    // 设置投影矩阵
    g_projection = Mat4::Ortho2D(0.0f, (float)width, 0.0f, (float)height);
    
    // 开始渲染
    g_renderer->Begin();
    g_renderer->SetProjection(g_projection);
    
    // 绘制背景精灵
    for (auto& sprite : g_backgroundSprites) {
        g_renderer->DrawSprite(sprite);
    }
    
    // 绘制主精灵
    g_renderer->DrawSprite(g_mainSprite);
    
    // 绘制一些纯色矩形（演示DrawRect）
    g_renderer->DrawRect(Vec2(50, 50), Vec2(80, 80), Vec4(1, 0, 0, 0.5f));
    g_renderer->DrawRect(Vec2(150, 50), Vec2(80, 80), Vec4(0, 1, 0, 0.5f), ToRadians(45.0f));
    g_renderer->DrawRect(Vec2(250, 50), Vec2(80, 80), Vec4(0, 0, 1, 0.5f));
    
    g_renderer->End();
}

// ============================================================================
// ImGui界面
// ============================================================================

void RenderImGui() {
    Engine& engine = Engine::Instance();
    
    ImGui::Begin("Sprite Demo");
    {
        ImGui::Text("FPS: %.1f", engine.GetFPS());
        
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::BulletText("WASD - Move");
        ImGui::BulletText("Q/E  - Rotate");
        ImGui::BulletText("Z/X  - Scale");
        ImGui::BulletText("Space - Next frame");
        ImGui::BulletText("R    - Reset");
        
        ImGui::Separator();
        ImGui::Text("Sprite Properties:");
        
        Vec2 pos = g_mainSprite.GetPosition();
        if (ImGui::DragFloat2("Position", &pos.x, 1.0f)) {
            g_mainSprite.SetPosition(pos);
        }
        
        float rotDeg = g_mainSprite.GetRotationDegrees();
        if (ImGui::DragFloat("Rotation", &rotDeg, 1.0f)) {
            g_mainSprite.SetRotationDegrees(rotDeg);
        }
        
        Vec2 size = g_mainSprite.GetSize();
        if (ImGui::DragFloat2("Size", &size.x, 1.0f, 10.0f, 500.0f)) {
            g_mainSprite.SetSize(size);
        }
        
        Vec4 color = g_mainSprite.GetColor();
        if (ImGui::ColorEdit4("Color", &color.x)) {
            g_mainSprite.SetColor(color);
        }
        
        ImGui::Separator();
        ImGui::Text("Animation:");
        ImGui::SliderInt("Frame", &g_currentFrame, 0, 15);
        if (ImGui::IsItemEdited()) {
            g_mainSprite.SetFrameFromSheet(g_currentFrame, 4, 4);
        }
        ImGui::Checkbox("Auto Animate", &g_autoAnimate);
        if (g_autoAnimate) {
            ImGui::SliderFloat("Speed", &g_animSpeed, 0.05f, 1.0f);
        }
        
        ImGui::Separator();
        bool flipX = g_mainSprite.GetFlipX();
        bool flipY = g_mainSprite.GetFlipY();
        if (ImGui::Checkbox("Flip X", &flipX)) g_mainSprite.SetFlipX(flipX);
        ImGui::SameLine();
        if (ImGui::Checkbox("Flip Y", &flipY)) g_mainSprite.SetFlipY(flipY);
    }
    ImGui::End();
    
    // 纹理信息面板
    ImGui::Begin("Texture Info");
    {
        ImGui::Text("Color Grid Texture:");
        ImGui::Text("  Size: %dx%d", g_colorTexture->GetWidth(), g_colorTexture->GetHeight());
        ImGui::Text("  Channels: %d", g_colorTexture->GetChannels());
        ImGui::Text("  ID: %u", g_colorTexture->GetID());
        
        ImGui::Separator();
        ImGui::Text("Checker Texture:");
        ImGui::Text("  Size: %dx%d", g_checkerTexture->GetWidth(), g_checkerTexture->GetHeight());
        ImGui::Text("  ID: %u", g_checkerTexture->GetID());
        
        ImGui::Separator();
        ImGui::TextWrapped(
            "Tip: Put your own images in 'template/data/texture/' "
            "and load them with Texture::Load()"
        );
    }
    ImGui::End();
}

// ============================================================================
// 清理
// ============================================================================

void Cleanup() {
    g_backgroundSprites.clear();
    g_checkerTexture.reset();
    g_colorTexture.reset();
    g_renderer->Shutdown();
    g_renderer.reset();
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Mini Game Engine - Example 04" << std::endl;
    std::cout << "  Textures and Sprites" << std::endl;
    std::cout << "========================================" << std::endl;
    
    Engine& engine = Engine::Instance();
    
    EngineConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "Mini Engine - Textures and Sprites";
    
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
