/**
 * ============================================================================
 * 示例11：精灵动画系统 (Sprite Animation System)
 * ============================================================================
 * 
 * 本章学习目标：
 * ----------------------------------------------------------------------------
 * 1. 从Spritesheet加载角色动画帧
 * 2. 实现帧动画播放（按时间切换帧）
 * 3. 构建动画状态机（待机、跑步、跳跃）
 * 4. 根据角色状态自动切换动画
 * 
 * 核心概念：
 * ----------------------------------------------------------------------------
 * 
 * 【什么是Spritesheet（精灵图集）？】
 * 
 *   player.png 包含角色所有动画帧：
 *   
 *   ┌────┬────┬────┬────┐
 *   │ 0  │ 1  │ 2  │ 3  │  行0: 待机动画 (Idle)
 *   ├────┼────┼────┼────┤
 *   │ 4  │ 5  │ 6  │ 7  │  行1: 跑步动画 (Run)
 *   ├────┼────┼────┼────┤
 *   │ 8  │ 9  │ 10 │ 11 │  行2: 跳跃动画 (Jump)
 *   └────┴────┴────┴────┘
 *        每帧 64×64 像素
 * 
 * 【帧动画原理】
 * 
 *   帧动画就是快速切换图片，产生运动的错觉：
 *   
 *   时间线：  0ms    100ms   200ms   300ms   400ms ...
 *   显示帧：  [0]  →  [1]  →  [2]  →  [3]  →  [0]  → 循环
 *   
 *   关键参数：
 *   - 帧率(FPS)：每秒播放多少帧，如10FPS = 每帧100ms
 *   - 帧时长：每帧显示的时间 = 1000ms / FPS
 * 
 * 【动画状态机】
 * 
 *   根据角色状态自动切换动画：
 *   
 *           ┌─────────┐
 *      ┌────│  Idle   │←───┐
 *      │    └────┬────┘    │
 *      │         │ 按A/D   │ 停止移动
 *      │         ↓         │
 *      │    ┌─────────┐    │
 *      │    │   Run   │────┘
 *      │    └────┬────┘
 *      │         │ 按空格
 *      │         ↓
 *      │    ┌─────────┐
 *      └────│  Jump   │
 *   落地    └─────────┘
 * 
 * 文件依赖：
 * ----------------------------------------------------------------------------
 * - data/texture/player.png : 256×192像素，4列×3行，每帧64×64
 * - data/texture/tileset.png : 瓦片纹理（地面渲染）
 * 
 * 操作说明：
 * ----------------------------------------------------------------------------
 * - A/D     : 左右移动（播放跑步动画）
 * - 空格    : 跳跃（播放跳跃动画）
 * - 1/2/3   : 手动切换动画（待机/跑步/跳跃）
 * - +/-     : 调整动画速度
 * - P       : 暂停/继续动画
 * - R       : 重置位置
 * 
 * 作者：Mini Game Engine Tutorial
 * 章节：第11章 - 精灵动画系统
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "engine/SpriteRenderer.h"
#include "engine/Texture.h"
#include "engine/Sprite.h"
#include "tilemap/TilemapSystem.h"
#include "math/Math.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <map>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 动画数据结构
// ============================================================================

/**
 * 单个动画定义
 * 
 * 存储动画的帧范围和播放参数
 */
struct Animation {
    std::string name;       // 动画名称
    int startFrame;         // 起始帧索引
    int frameCount;         // 帧数量
    float frameTime;        // 每帧时长（秒）
    bool loop;              // 是否循环播放
    
    Animation(const std::string& n = "", int start = 0, int count = 1, 
              float time = 0.1f, bool l = true)
        : name(n), startFrame(start), frameCount(count), frameTime(time), loop(l) {}
};

/**
 * 动画状态枚举
 */
enum class AnimState {
    Idle,   // 待机
    Run,    // 跑步
    Jump    // 跳跃
};

/**
 * 动画控制器
 * 
 * 管理动画播放状态，自动计算当前帧
 */
class AnimationController {
public:
    // 添加动画
    void AddAnimation(AnimState state, const Animation& anim) {
        m_animations[state] = anim;
    }
    
    // 切换动画
    void Play(AnimState state) {
        if (m_currentState != state) {
            m_currentState = state;
            m_currentFrame = 0;
            m_timer = 0;
        }
    }
    
    // 更新动画（每帧调用）
    void Update(float deltaTime) {
        if (m_paused) return;
        
        auto it = m_animations.find(m_currentState);
        if (it == m_animations.end()) return;
        
        const Animation& anim = it->second;
        
        m_timer += deltaTime * m_speed;
        
        if (m_timer >= anim.frameTime) {
            m_timer -= anim.frameTime;
            m_currentFrame++;
            
            if (m_currentFrame >= anim.frameCount) {
                if (anim.loop) {
                    m_currentFrame = 0;
                } else {
                    m_currentFrame = anim.frameCount - 1;
                }
            }
        }
    }
    
    // 获取当前全局帧索引（在spritesheet中的索引）
    int GetCurrentFrameIndex() const {
        auto it = m_animations.find(m_currentState);
        if (it == m_animations.end()) return 0;
        return it->second.startFrame + m_currentFrame;
    }
    
    // 获取当前动画
    const Animation* GetCurrentAnimation() const {
        auto it = m_animations.find(m_currentState);
        if (it != m_animations.end()) return &it->second;
        return nullptr;
    }
    
    // 状态访问
    AnimState GetState() const { return m_currentState; }
    int GetCurrentFrame() const { return m_currentFrame; }
    float GetTimer() const { return m_timer; }
    
    // 播放控制
    void Pause() { m_paused = true; }
    void Resume() { m_paused = false; }
    void TogglePause() { m_paused = !m_paused; }
    bool IsPaused() const { return m_paused; }
    
    // 速度控制
    void SetSpeed(float speed) { m_speed = speed; }
    float GetSpeed() const { return m_speed; }
    
private:
    std::map<AnimState, Animation> m_animations;
    AnimState m_currentState = AnimState::Idle;
    int m_currentFrame = 0;
    float m_timer = 0;
    float m_speed = 1.0f;
    bool m_paused = false;
};

// ============================================================================
// 全局变量
// ============================================================================

// 渲染器
std::unique_ptr<Renderer2D> g_renderer;
std::unique_ptr<SpriteRenderer> g_spriteRenderer;

// 纹理
std::shared_ptr<Texture> g_playerTexture;
std::shared_ptr<Texture> g_tilesetTexture;

// 动画
AnimationController g_animator;

// Spritesheet参数
const int SPRITE_COLS = 4;      // 列数
const int SPRITE_ROWS = 3;      // 行数
const int FRAME_WIDTH = 64;     // 每帧宽度
const int FRAME_HEIGHT = 64;    // 每帧高度

// 玩家状态
Vec2 g_playerPos(400, 200);     // 位置
Vec2 g_playerVel(0, 0);         // 速度
bool g_facingRight = true;      // 朝向
bool g_isGrounded = true;       // 是否在地面
float g_playerScale = 2.0f;     // 缩放（让角色更大更清晰）

// 物理参数
const float GRAVITY = -1500.0f;
const float MOVE_SPEED = 300.0f;
const float JUMP_FORCE = 600.0f;
const float GROUND_Y = 150.0f;  // 地面高度

// ============================================================================
// UV坐标计算
// ============================================================================

/**
 * 计算指定帧的UV坐标
 * 
 * @param frameIndex 帧索引（0-11）
 * @param cols spritesheet列数
 * @param rows spritesheet行数
 * @param uvMin 输出UV左下角
 * @param uvMax 输出UV右上角
 */
void CalculateFrameUV(int frameIndex, int cols, int rows, Vec2& uvMin, Vec2& uvMax) {
    int col = frameIndex % cols;
    int row = frameIndex / cols;
    
    float tileU = 1.0f / cols;
    float tileV = 1.0f / rows;
    
    // Y轴翻转（OpenGL纹理坐标原点在左下）
    uvMin.x = col * tileU;
    uvMin.y = 1.0f - (row + 1) * tileV;
    uvMax.x = (col + 1) * tileU;
    uvMax.y = 1.0f - row * tileV;
}

// ============================================================================
// 初始化
// ============================================================================

bool Initialize() {
    std::cout << "[Example11] Initializing Sprite Animation Demo..." << std::endl;
    
    // 初始化渲染器
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) {
        std::cerr << "[Example11] Failed to initialize Renderer2D!" << std::endl;
        return false;
    }
    
    g_spriteRenderer = std::make_unique<SpriteRenderer>();
    if (!g_spriteRenderer->Initialize()) {
        std::cerr << "[Example11] Failed to initialize SpriteRenderer!" << std::endl;
        return false;
    }
    
    // 加载玩家纹理
    g_playerTexture = std::make_shared<Texture>();
    const char* playerPaths[] = {
        "data/texture/player.png",
        "../data/texture/player.png",
        "../../data/texture/player.png"
    };
    
    bool playerLoaded = false;
    for (const char* path : playerPaths) {
        if (g_playerTexture->Load(path)) {
            std::cout << "[Example11] Loaded player: " << path 
                      << " (" << g_playerTexture->GetWidth() << "x" 
                      << g_playerTexture->GetHeight() << ")" << std::endl;
            playerLoaded = true;
            break;
        }
    }
    
    if (!playerLoaded) {
        std::cerr << "[Example11] WARNING: player.png not found!" << std::endl;
        std::cerr << "[Example11] Please place player.png (256x192, 4x3 frames) in data/texture/" << std::endl;
        g_playerTexture->CreateSolidColor(Vec4(0.3f, 0.6f, 1.0f, 1.0f), 64, 64);
    }
    
    // 加载tileset纹理（用于地面）
    g_tilesetTexture = std::make_shared<Texture>();
    const char* tilesetPaths[] = {
        "data/texture/tileset.png",
        "../data/texture/tileset.png",
        "../../data/texture/tileset.png"
    };
    
    for (const char* path : tilesetPaths) {
        if (g_tilesetTexture->Load(path)) {
            std::cout << "[Example11] Loaded tileset: " << path << std::endl;
            break;
        }
    }
    
    // ========================================================================
    // 设置动画
    // ========================================================================
    // player.png布局：4列×3行
    // 行0 (帧0-3): 待机
    // 行1 (帧4-7): 跑步
    // 行2 (帧8-11): 跳跃
    
    g_animator.AddAnimation(AnimState::Idle, Animation("Idle", 0, 4, 0.15f, true));
    g_animator.AddAnimation(AnimState::Run,  Animation("Run",  4, 4, 0.1f,  true));
    g_animator.AddAnimation(AnimState::Jump, Animation("Jump", 8, 4, 0.15f, false));
    
    std::cout << "[Example11] Animations configured:" << std::endl;
    std::cout << "  - Idle: frames 0-3, 0.15s/frame, loop" << std::endl;
    std::cout << "  - Run:  frames 4-7, 0.1s/frame, loop" << std::endl;
    std::cout << "  - Jump: frames 8-11, 0.15s/frame, no loop" << std::endl;
    
    // 设置投影矩阵
    Engine& engine = Engine::Instance();
    g_spriteRenderer->SetProjection(Mat4::Ortho(
        0, (float)engine.GetWindowWidth(),
        0, (float)engine.GetWindowHeight(),
        -1, 1
    ));
    
    std::cout << "[Example11] Sprite Animation Demo Ready!" << std::endl;
    std::cout << "  - A/D: Move left/right" << std::endl;
    std::cout << "  - SPACE: Jump" << std::endl;
    std::cout << "  - 1/2/3: Switch animation manually" << std::endl;
    std::cout << "  - P: Pause/Resume" << std::endl;
    
    return true;
}

// ============================================================================
// 更新
// ============================================================================

void Update(float deltaTime) {
    Engine& engine = Engine::Instance();
    GLFWwindow* window = engine.GetWindow();
    
    // 限制deltaTime
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    
    // ========================================================================
    // 输入处理
    // ========================================================================
    
    // 手动切换动画
    static bool key1Last = false, key2Last = false, key3Last = false;
    bool key1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    bool key2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
    bool key3 = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
    
    if (key1 && !key1Last) g_animator.Play(AnimState::Idle);
    if (key2 && !key2Last) g_animator.Play(AnimState::Run);
    if (key3 && !key3Last) g_animator.Play(AnimState::Jump);
    key1Last = key1; key2Last = key2; key3Last = key3;
    
    // 暂停
    static bool pKeyLast = false;
    bool pKey = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (pKey && !pKeyLast) g_animator.TogglePause();
    pKeyLast = pKey;
    
    // 速度调整
    static bool plusLast = false, minusLast = false;
    bool plusKey = glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS;
    bool minusKey = glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS;
    if (plusKey && !plusLast) g_animator.SetSpeed(g_animator.GetSpeed() + 0.25f);
    if (minusKey && !minusLast) g_animator.SetSpeed(std::max(0.25f, g_animator.GetSpeed() - 0.25f));
    plusLast = plusKey; minusLast = minusKey;
    
    // 重置
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        g_playerPos = Vec2(400, 200);
        g_playerVel = Vec2::Zero();
        g_isGrounded = true;
    }
    
    // ========================================================================
    // 玩家移动
    // ========================================================================
    
    bool moving = false;
    
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        g_playerVel.x = -MOVE_SPEED;
        g_facingRight = false;
        moving = true;
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        g_playerVel.x = MOVE_SPEED;
        g_facingRight = true;
        moving = true;
    } else {
        // 摩擦力
        g_playerVel.x *= 0.85f;
        if (std::abs(g_playerVel.x) < 10.0f) g_playerVel.x = 0;
    }
    
    // 跳跃
    static bool jumpLast = false;
    bool jumpKey = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (jumpKey && !jumpLast && g_isGrounded) {
        g_playerVel.y = JUMP_FORCE;
        g_isGrounded = false;
    }
    jumpLast = jumpKey;
    
    // ========================================================================
    // 物理更新
    // ========================================================================
    
    // 重力
    if (!g_isGrounded) {
        g_playerVel.y += GRAVITY * deltaTime;
    }
    
    // 更新位置
    g_playerPos.x += g_playerVel.x * deltaTime;
    g_playerPos.y += g_playerVel.y * deltaTime;
    
    // 地面碰撞
    if (g_playerPos.y <= GROUND_Y) {
        g_playerPos.y = GROUND_Y;
        g_playerVel.y = 0;
        g_isGrounded = true;
    }
    
    // 边界限制
    float screenW = (float)engine.GetWindowWidth();
    float halfW = FRAME_WIDTH * g_playerScale * 0.5f;
    if (g_playerPos.x < halfW) g_playerPos.x = halfW;
    if (g_playerPos.x > screenW - halfW) g_playerPos.x = screenW - halfW;
    
    // ========================================================================
    // 动画状态机
    // ========================================================================
    
    if (!g_isGrounded) {
        // 空中 → 跳跃动画
        g_animator.Play(AnimState::Jump);
    } else if (moving) {
        // 地面移动 → 跑步动画
        g_animator.Play(AnimState::Run);
    } else {
        // 站立 → 待机动画
        g_animator.Play(AnimState::Idle);
    }
    
    // 更新动画
    g_animator.Update(deltaTime);
}

// ============================================================================
// 渲染
// ============================================================================

void Render() {
    Engine& engine = Engine::Instance();
    int screenW = engine.GetWindowWidth();
    int screenH = engine.GetWindowHeight();
    
    g_spriteRenderer->Begin();
    
    // ========================================================================
    // 渲染地面（使用tileset）
    // ========================================================================
    
    if (g_tilesetTexture->IsValid()) {
        // 使用tileset中的草地瓦片(ID 1)铺满底部
        int tileSize = 32;
        int tilesX = screenW / tileSize + 1;
        
        Vec2 uvMin, uvMax;
        // 草地瓦片在tileset中的位置（第1个瓦片）
        int tileID = 1;
        int col = (tileID - 1) % 8;
        int row = (tileID - 1) / 8;
        uvMin.x = col / 8.0f;
        uvMin.y = 1.0f - (row + 1) / 5.0f;
        uvMax.x = (col + 1) / 8.0f;
        uvMax.y = 1.0f - row / 5.0f;
        
        for (int i = 0; i < tilesX; i++) {
            for (int j = 0; j < 4; j++) {  // 4层地面
                Sprite groundTile;
                groundTile.SetTexture(g_tilesetTexture);
                groundTile.SetPosition(Vec2(i * tileSize + tileSize/2.0f, 
                                            j * tileSize + tileSize/2.0f));
                groundTile.SetSize(Vec2((float)tileSize, (float)tileSize));
                groundTile.SetUV(uvMin, uvMax);
                g_spriteRenderer->DrawSprite(groundTile);
            }
        }
    } else {
        // 没有tileset时画一个简单的地面
        float groundTop = GROUND_Y + 20;
        float ndcY = (groundTop / screenH) * 2.0f - 1.0f;
        g_renderer->DrawTriangle(
            Vec2(-1, -1), Vec2(1, -1), Vec2(1, ndcY),
            Vec4(0.3f, 0.5f, 0.2f, 1.0f));
        g_renderer->DrawTriangle(
            Vec2(-1, -1), Vec2(1, ndcY), Vec2(-1, ndcY),
            Vec4(0.3f, 0.5f, 0.2f, 1.0f));
    }
    
    // ========================================================================
    // 渲染玩家
    // ========================================================================
    
    // 计算当前帧的UV
    int frameIndex = g_animator.GetCurrentFrameIndex();
    Vec2 uvMin, uvMax;
    CalculateFrameUV(frameIndex, SPRITE_COLS, SPRITE_ROWS, uvMin, uvMax);
    
    // 如果角色朝左，水平翻转UV
    if (!g_facingRight) {
        std::swap(uvMin.x, uvMax.x);
    }
    
    // 创建玩家精灵
    Sprite playerSprite;
    playerSprite.SetTexture(g_playerTexture);
    playerSprite.SetPosition(g_playerPos);
    playerSprite.SetSize(Vec2(FRAME_WIDTH * g_playerScale, FRAME_HEIGHT * g_playerScale));
    playerSprite.SetUV(uvMin, uvMax);
    
    g_spriteRenderer->DrawSprite(playerSprite);
    
    g_spriteRenderer->End();
}

// ============================================================================
// ImGui界面
// ============================================================================

void RenderImGui() {
    Engine& engine = Engine::Instance();
    
    // 动画信息面板
    ImGui::Begin("Sprite Animation Demo");
    ImGui::Text("FPS: %.1f", engine.GetFPS());
    
    ImGui::Separator();
    ImGui::Text("Animation System");
    
    const Animation* currentAnim = g_animator.GetCurrentAnimation();
    if (currentAnim) {
        ImGui::Text("Current: %s", currentAnim->name.c_str());
        ImGui::Text("Frame: %d / %d", g_animator.GetCurrentFrame() + 1, currentAnim->frameCount);
        ImGui::Text("Global Frame: %d", g_animator.GetCurrentFrameIndex());
        
        // 动画进度条
        float progress = (float)(g_animator.GetCurrentFrame() + 1) / currentAnim->frameCount;
        ImGui::ProgressBar(progress, ImVec2(-1, 0), "");
    }
    
    ImGui::Text("Speed: %.2fx", g_animator.GetSpeed());
    ImGui::Text("Paused: %s", g_animator.IsPaused() ? "Yes" : "No");
    
    ImGui::Separator();
    ImGui::Text("Player State");
    ImGui::Text("Position: (%.0f, %.0f)", g_playerPos.x, g_playerPos.y);
    ImGui::Text("Velocity: (%.0f, %.0f)", g_playerVel.x, g_playerVel.y);
    ImGui::Text("Grounded: %s", g_isGrounded ? "Yes" : "No");
    ImGui::Text("Facing: %s", g_facingRight ? "Right" : "Left");
    ImGui::End();
    
    // 控制面板
    ImGui::Begin("Controls");
    ImGui::Text("Movement:");
    ImGui::BulletText("A/D - Move left/right");
    ImGui::BulletText("SPACE - Jump");
    
    ImGui::Separator();
    ImGui::Text("Animation:");
    ImGui::BulletText("1 - Play Idle");
    ImGui::BulletText("2 - Play Run");
    ImGui::BulletText("3 - Play Jump");
    ImGui::BulletText("P - Pause/Resume");
    ImGui::BulletText("+/- - Speed up/down");
    ImGui::BulletText("R - Reset position");
    ImGui::End();
    
    // Spritesheet预览
    ImGui::Begin("Spritesheet Preview");
    if (g_playerTexture->IsValid()) {
        ImGui::Text("Size: %dx%d", g_playerTexture->GetWidth(), g_playerTexture->GetHeight());
        ImGui::Text("Layout: %d cols x %d rows", SPRITE_COLS, SPRITE_ROWS);
        ImGui::Text("Frame: %dx%d pixels", FRAME_WIDTH, FRAME_HEIGHT);
        
        // 显示整个spritesheet
        ImTextureID texID = (ImTextureID)(intptr_t)g_playerTexture->GetID();
        ImGui::Image(texID, ImVec2(256, 192));
        
        // 高亮当前帧
        ImGui::Text("Current Frame: %d", g_animator.GetCurrentFrameIndex());
        
        // 显示当前帧的单独预览
        int frame = g_animator.GetCurrentFrameIndex();
        int col = frame % SPRITE_COLS;
        int row = frame / SPRITE_COLS;
        float u0 = col / (float)SPRITE_COLS;
        float v0 = row / (float)SPRITE_ROWS;
        float u1 = (col + 1) / (float)SPRITE_COLS;
        float v1 = (row + 1) / (float)SPRITE_ROWS;
        
        ImGui::Text("Current Frame Preview:");
        ImGui::Image(texID, ImVec2(128, 128), ImVec2(u0, v0), ImVec2(u1, v1));
    }
    ImGui::End();
    
    // 动画状态机可视化
    ImGui::Begin("State Machine");
    ImGui::Text("Current State:");
    
    AnimState state = g_animator.GetState();
    ImVec4 idleColor = (state == AnimState::Idle) ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1);
    ImVec4 runColor = (state == AnimState::Run) ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1);
    ImVec4 jumpColor = (state == AnimState::Jump) ? ImVec4(0, 1, 0, 1) : ImVec4(0.5f, 0.5f, 0.5f, 1);
    
    ImGui::TextColored(idleColor, "[IDLE]");
    ImGui::SameLine();
    ImGui::TextColored(runColor, "[RUN]");
    ImGui::SameLine();
    ImGui::TextColored(jumpColor, "[JUMP]");
    
    ImGui::Separator();
    ImGui::Text("Transitions:");
    ImGui::BulletText("Idle -> Run: Start moving");
    ImGui::BulletText("Run -> Idle: Stop moving");
    ImGui::BulletText("Any -> Jump: Press SPACE");
    ImGui::BulletText("Jump -> Idle/Run: Land");
    ImGui::End();
}

// ============================================================================
// 清理
// ============================================================================

void Cleanup() {
    g_tilesetTexture.reset();
    g_playerTexture.reset();
    g_spriteRenderer.reset();
    g_renderer.reset();
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Mini Game Engine - Example 11" << std::endl;
    std::cout << "  Sprite Animation System" << std::endl;
    std::cout << "========================================" << std::endl;
    
    Engine& engine = Engine::Instance();
    
    EngineConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "Mini Engine - Sprite Animation";
    
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
