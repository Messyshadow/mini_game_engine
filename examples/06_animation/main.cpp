/**
 * 示例06：精灵动画系统
 * 
 * ============================================================================
 * 学习目标
 * ============================================================================
 * 
 * 1. 理解精灵图集和帧动画
 * 2. 使用Animator控制动画播放
 * 3. 实现动画状态切换
 * 4. UV坐标计算原理
 * 
 * ============================================================================
 * 操作说明
 * ============================================================================
 * 
 * A/D      - 左右移动（播放跑步动画）
 * 空格      - 跳跃（播放跳跃动画）
 * 松开按键  - 待机动画
 * 1/2/3/4  - 直接切换动画
 * +/-      - 调整动画速度
 * ESC      - 退出
 * 
 * ============================================================================
 * 说明
 * ============================================================================
 * 
 * 由于没有实际的精灵图集图片，本示例使用程序生成的纹理来模拟。
 * 每个"帧"显示不同颜色，帮助你理解动画播放原理。
 * 
 * 在实际项目中，你需要：
 * 1. 准备精灵图集PNG文件
 * 2. 使用Texture::Load()加载
 * 3. 设置正确的行列数
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "engine/Texture.h"
#include "animation/Animation.h"
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
std::shared_ptr<Texture> g_spriteSheet;

// 带动画的精灵
AnimatedSprite g_player;

// 玩家状态
enum class PlayerState {
    Idle,
    Run,
    Jump,
    Fall
};
PlayerState g_playerState = PlayerState::Idle;
Vec2 g_velocity(0, 0);
bool g_onGround = true;
float g_groundY = 150.0f;

// 动画速度
float g_animSpeed = 1.0f;

// ============================================================================
// 创建模拟精灵图集
// ============================================================================

/**
 * 创建一个模拟的精灵图集纹理
 * 
 * 布局：4列 × 4行 = 16帧
 * - 帧 0-3:   待机动画 (Idle)  - 蓝色渐变
 * - 帧 4-7:   跑步动画 (Run)   - 绿色渐变
 * - 帧 8-11:  跳跃动画 (Jump)  - 红色渐变
 * - 帧 12-15: 下落动画 (Fall)  - 紫色渐变
 */
std::shared_ptr<Texture> CreateMockSpriteSheet() {
    const int frameSize = 64;   // 每帧64x64像素
    const int cols = 4;
    const int rows = 4;
    const int width = frameSize * cols;   // 256
    const int height = frameSize * rows;  // 256
    
    std::vector<unsigned char> pixels(width * height * 4);
    
    // 定义每行的基础颜色
    Vec4 rowColors[4] = {
        Vec4(0.2f, 0.4f, 0.8f, 1.0f),  // 行0: 蓝色 (Idle)
        Vec4(0.2f, 0.8f, 0.3f, 1.0f),  // 行1: 绿色 (Run)
        Vec4(0.8f, 0.3f, 0.2f, 1.0f),  // 行2: 红色 (Jump)
        Vec4(0.6f, 0.2f, 0.8f, 1.0f),  // 行3: 紫色 (Fall)
    };
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int col = x / frameSize;
            int row = y / frameSize;
            
            // 获取基础颜色
            Vec4 color = rowColors[row];
            
            // 根据列号调整亮度，模拟动画帧变化
            float brightness = 0.7f + 0.3f * (float)col / (cols - 1);
            color.x *= brightness;
            color.y *= brightness;
            color.z *= brightness;
            
            // 在每帧内画一个简单的角色形状
            int localX = x % frameSize;
            int localY = y % frameSize;
            
            // 角色身体（椭圆形）
            int centerX = frameSize / 2;
            int centerY = frameSize / 2;
            
            // 根据动画帧偏移中心，模拟移动
            int offsetX = (col - 2) * 3;  // 左右摆动
            int offsetY = (row == 2) ? (col * 2) : 0;  // 跳跃时上升
            
            centerX += offsetX;
            centerY += offsetY;
            
            float dx = (float)(localX - centerX) / 15.0f;
            float dy = (float)(localY - centerY) / 20.0f;
            float dist = dx * dx + dy * dy;
            
            int index = (y * width + x) * 4;
            
            if (dist < 1.0f) {
                // 角色身体
                pixels[index + 0] = (unsigned char)(color.x * 255);
                pixels[index + 1] = (unsigned char)(color.y * 255);
                pixels[index + 2] = (unsigned char)(color.z * 255);
                pixels[index + 3] = 255;
            } else if (dist < 1.3f) {
                // 边缘
                pixels[index + 0] = (unsigned char)(color.x * 180);
                pixels[index + 1] = (unsigned char)(color.y * 180);
                pixels[index + 2] = (unsigned char)(color.z * 180);
                pixels[index + 3] = 255;
            } else {
                // 透明背景
                pixels[index + 0] = 0;
                pixels[index + 1] = 0;
                pixels[index + 2] = 0;
                pixels[index + 3] = 0;
            }
            
            // 在帧角落画帧号（用于调试）
            if (localX < 12 && localY >= frameSize - 12) {
                pixels[index + 0] = 255;
                pixels[index + 1] = 255;
                pixels[index + 2] = 255;
                pixels[index + 3] = 200;
            }
        }
    }
    
    auto texture = std::make_shared<Texture>();
    texture->LoadFromMemory(pixels.data(), width, height, 4);
    texture->SetFilter(TextureFilter::Nearest, TextureFilter::Nearest);
    
    return texture;
}

// ============================================================================
// 初始化
// ============================================================================

bool Initialize() {
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) {
        return false;
    }
    
    // 创建模拟精灵图集
    std::cout << "[Example06] Creating mock sprite sheet..." << std::endl;
    g_spriteSheet = CreateMockSpriteSheet();
    
    // 设置玩家精灵
    g_player.SetTexture(g_spriteSheet);
    g_player.SetSpriteSheet(4, 4);  // 4列4行
    g_player.SetPosition(640, g_groundY);
    g_player.SetSize(128, 128);     // 显示尺寸
    g_player.SetPivotCenter();
    
    // 添加动画
    // 注意：帧索引是从左到右、从上到下计数
    // 行0: 帧0-3,  行1: 帧4-7,  行2: 帧8-11,  行3: 帧12-15
    g_player.AddAnimation("idle", 0, 3, 8.0f, true);    // 待机：8帧/秒，循环
    g_player.AddAnimation("run", 4, 7, 12.0f, true);    // 跑步：12帧/秒，循环
    g_player.AddAnimation("jump", 8, 11, 10.0f, false); // 跳跃：10帧/秒，不循环
    g_player.AddAnimation("fall", 12, 15, 8.0f, true);  // 下落：8帧/秒，循环
    
    // 默认播放待机动画
    g_player.Play("idle");
    
    // 设置动画完成回调
    g_player.GetAnimator().SetOnComplete([](const std::string& name) {
        std::cout << "[Animation] '" << name << "' completed!" << std::endl;
        
        // 跳跃动画完成后切换到下落
        if (name == "jump") {
            g_player.Play("fall");
            g_playerState = PlayerState::Fall;
        }
    });
    
    std::cout << "[Example06] Initialization complete." << std::endl;
    return true;
}

// ============================================================================
// 更新
// ============================================================================

void Update(float deltaTime) {
    Engine& engine = Engine::Instance();
    GLFWwindow* window = engine.GetWindow();
    
    // 设置动画速度
    g_player.GetAnimator().SetSpeed(g_animSpeed);
    
    // 物理参数
    const float moveSpeed = 300.0f;
    const float jumpForce = 500.0f;
    const float gravity = 1200.0f;
    
    // 输入处理
    bool moveLeft = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool moveRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    bool jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    
    static bool jumpWasPressed = false;
    bool jumpJustPressed = jumpPressed && !jumpWasPressed;
    jumpWasPressed = jumpPressed;
    
    // 水平移动
    g_velocity.x = 0;
    if (moveLeft) {
        g_velocity.x = -moveSpeed;
        g_player.SetFlipX(true);
    }
    if (moveRight) {
        g_velocity.x = moveSpeed;
        g_player.SetFlipX(false);
    }
    
    // 跳跃
    if (jumpJustPressed && g_onGround) {
        g_velocity.y = jumpForce;
        g_onGround = false;
    }
    
    // 应用重力
    if (!g_onGround) {
        g_velocity.y -= gravity * deltaTime;
    }
    
    // 更新位置
    Vec2 pos = g_player.GetPosition();
    pos.x += g_velocity.x * deltaTime;
    pos.y += g_velocity.y * deltaTime;
    
    // 地面检测
    if (pos.y <= g_groundY) {
        pos.y = g_groundY;
        g_velocity.y = 0;
        g_onGround = true;
    }
    
    // 边界限制
    if (pos.x < 64) pos.x = 64;
    if (pos.x > 1216) pos.x = 1216;
    
    g_player.SetPosition(pos);
    
    // 动画状态机
    PlayerState newState = g_playerState;
    
    if (!g_onGround) {
        if (g_velocity.y > 0) {
            newState = PlayerState::Jump;
        } else {
            newState = PlayerState::Fall;
        }
    } else {
        if (g_velocity.x != 0) {
            newState = PlayerState::Run;
        } else {
            newState = PlayerState::Idle;
        }
    }
    
    // 切换动画
    if (newState != g_playerState) {
        g_playerState = newState;
        
        switch (g_playerState) {
            case PlayerState::Idle:
                g_player.Play("idle");
                break;
            case PlayerState::Run:
                g_player.Play("run");
                break;
            case PlayerState::Jump:
                g_player.Play("jump");
                break;
            case PlayerState::Fall:
                g_player.Play("fall");
                break;
        }
    }
    
    // 数字键直接切换动画
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) g_player.Play("idle", true);
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) g_player.Play("run", true);
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) g_player.Play("jump", true);
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) g_player.Play("fall", true);
    
    // 调整动画速度
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
        g_animSpeed += deltaTime;
        if (g_animSpeed > 3.0f) g_animSpeed = 3.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
        g_animSpeed -= deltaTime;
        if (g_animSpeed < 0.1f) g_animSpeed = 0.1f;
    }
    
    // 更新动画
    g_player.Update(deltaTime);
}

// ============================================================================
// 渲染
// ============================================================================

void Render() {
    Engine& engine = Engine::Instance();
    int width = engine.GetWindowWidth();
    int height = engine.GetWindowHeight();
    
    // 绘制地面
    Vec2 groundMin = Vec2(0.0f / width * 2 - 1, 0.0f / height * 2 - 1);
    Vec2 groundMax = Vec2(1280.0f / width * 2 - 1, (g_groundY - 64) / height * 2 - 1);
    g_renderer->DrawTriangle(
        Vec2(groundMin.x, groundMin.y),
        Vec2(groundMax.x, groundMin.y),
        Vec2(groundMax.x, groundMax.y),
        Vec4(0.3f, 0.25f, 0.2f, 1.0f)
    );
    g_renderer->DrawTriangle(
        Vec2(groundMin.x, groundMin.y),
        Vec2(groundMax.x, groundMax.y),
        Vec2(groundMin.x, groundMax.y),
        Vec4(0.3f, 0.25f, 0.2f, 1.0f)
    );
    
    // 绘制玩家精灵
    if (g_player.IsVisible() && g_spriteSheet) {
        // 获取当前UV
        Vec2 uvMin, uvMax;
        g_player.GetCurrentUV(uvMin, uvMax);
        
        // 获取位置和尺寸
        Vec2 pos = g_player.GetPosition();
        Vec2 size = g_player.GetRenderSize();
        Vec2 pivot = g_player.GetPivot();
        
        // 计算四个角的位置（考虑锚点）
        float left = pos.x - size.x * pivot.x;
        float right = pos.x + size.x * (1 - pivot.x);
        float bottom = pos.y - size.y * pivot.y;
        float top = pos.y + size.y * (1 - pivot.y);
        
        // 转换为NDC
        float ndcLeft = left / width * 2 - 1;
        float ndcRight = right / width * 2 - 1;
        float ndcBottom = bottom / height * 2 - 1;
        float ndcTop = top / height * 2 - 1;
        
        // 绘制带纹理的四边形
        // 由于Renderer2D目前不支持纹理，我们用颜色来模拟
        // 根据UV坐标计算颜色
        int frameIndex = g_player.GetAnimator().GetCurrentFrame();
        int col = frameIndex % 4;
        int row = frameIndex / 4;
        
        Vec4 frameColors[4] = {
            Vec4(0.2f, 0.4f, 0.8f, 1.0f),  // Idle - 蓝
            Vec4(0.2f, 0.8f, 0.3f, 1.0f),  // Run - 绿
            Vec4(0.8f, 0.3f, 0.2f, 1.0f),  // Jump - 红
            Vec4(0.6f, 0.2f, 0.8f, 1.0f),  // Fall - 紫
        };
        
        Vec4 color = frameColors[row];
        float brightness = 0.7f + 0.3f * (float)col / 3.0f;
        color.x *= brightness;
        color.y *= brightness;
        color.z *= brightness;
        
        // 画玩家（简化为矩形）
        g_renderer->DrawTriangle(
            Vec2(ndcLeft, ndcBottom),
            Vec2(ndcRight, ndcBottom),
            Vec2(ndcRight, ndcTop),
            color
        );
        g_renderer->DrawTriangle(
            Vec2(ndcLeft, ndcBottom),
            Vec2(ndcRight, ndcTop),
            Vec2(ndcLeft, ndcTop),
            color
        );
    }
}

// ============================================================================
// ImGui界面
// ============================================================================

void RenderImGui() {
    Engine& engine = Engine::Instance();
    
    ImGui::Begin("Animation Demo");
    {
        ImGui::Text("FPS: %.1f", engine.GetFPS());
        
        ImGui::Separator();
        ImGui::Text("Controls:");
        ImGui::BulletText("A/D   - Move left/right");
        ImGui::BulletText("Space - Jump");
        ImGui::BulletText("1-4   - Switch animation");
        ImGui::BulletText("+/-   - Adjust speed");
        
        ImGui::Separator();
        ImGui::Text("Animation Info:");
        ImGui::Text("  Current: %s", g_player.GetAnimator().GetCurrentClipName().c_str());
        ImGui::Text("  Frame: %d", g_player.GetAnimator().GetCurrentFrame());
        ImGui::Text("  Progress: %.1f%%", g_player.GetAnimator().GetProgress() * 100);
        ImGui::Text("  Speed: %.2fx", g_animSpeed);
        
        ImGui::Separator();
        ImGui::Text("Player State:");
        const char* stateNames[] = {"Idle", "Run", "Jump", "Fall"};
        ImGui::Text("  State: %s", stateNames[(int)g_playerState]);
        ImGui::Text("  Position: (%.0f, %.0f)", g_player.GetPosition().x, g_player.GetPosition().y);
        ImGui::Text("  Velocity: (%.0f, %.0f)", g_velocity.x, g_velocity.y);
        ImGui::Text("  On Ground: %s", g_onGround ? "Yes" : "No");
        ImGui::Text("  Flip X: %s", g_player.GetFlipX() ? "Yes" : "No");
        
        ImGui::Separator();
        ImGui::Text("UV Coordinates:");
        Vec2 uvMin, uvMax;
        g_player.GetCurrentUV(uvMin, uvMax);
        ImGui::Text("  Min: (%.3f, %.3f)", uvMin.x, uvMin.y);
        ImGui::Text("  Max: (%.3f, %.3f)", uvMax.x, uvMax.y);
    }
    ImGui::End();
    
    // 动画帧预览
    ImGui::Begin("Animation Clips");
    {
        ImGui::Text("Sprite Sheet: 4x4 = 16 frames");
        ImGui::Separator();
        
        ImGui::Text("Idle (frames 0-3):");
        ImGui::TextColored(ImVec4(0.2f, 0.4f, 0.8f, 1.0f), "  [0] [1] [2] [3]");
        
        ImGui::Text("Run (frames 4-7):");
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.3f, 1.0f), "  [4] [5] [6] [7]");
        
        ImGui::Text("Jump (frames 8-11):");
        ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.2f, 1.0f), "  [8] [9] [10] [11]");
        
        ImGui::Text("Fall (frames 12-15):");
        ImGui::TextColored(ImVec4(0.6f, 0.2f, 0.8f, 1.0f), "  [12] [13] [14] [15]");
        
        ImGui::Separator();
        ImGui::TextWrapped(
            "Note: This demo uses programmatically generated textures. "
            "In a real game, you would load a sprite sheet image file."
        );
    }
    ImGui::End();
}

// ============================================================================
// 清理
// ============================================================================

void Cleanup() {
    g_spriteSheet.reset();
    g_renderer->Shutdown();
    g_renderer.reset();
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Mini Game Engine - Example 06" << std::endl;
    std::cout << "  Sprite Animation System" << std::endl;
    std::cout << "========================================" << std::endl;
    
    Engine& engine = Engine::Instance();
    
    EngineConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "Mini Engine - Animation System";
    
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
