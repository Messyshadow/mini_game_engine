/**
 * 示例12：摄像机系统 (Camera System)
 * 
 * 学习目标：平滑跟随、边界限制、震动效果
 * 
 * 操作：A/D移动，空格跳跃，1/2/3切换速度，B切换边界，Z震动，R重置
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "engine/SpriteRenderer.h"
#include "engine/Texture.h"
#include "camera/Camera2D.h"
#include "tilemap/TilemapSystem.h"
#include "math/Math.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <map>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// 动画系统
struct Animation {
    std::string name; int startFrame, frameCount; float frameTime; bool loop;
    Animation(const std::string& n="", int s=0, int c=1, float t=0.1f, bool l=true)
        : name(n), startFrame(s), frameCount(c), frameTime(t), loop(l) {}
};

enum class AnimState { Idle, Run, Jump };

class AnimationController {
public:
    void AddAnimation(AnimState s, const Animation& a) { m_anims[s] = a; }
    void Play(AnimState s) { if (m_state != s) { m_state = s; m_frame = 0; m_timer = 0; } }
    void Update(float dt) {
        auto it = m_anims.find(m_state);
        if (it == m_anims.end()) return;
        m_timer += dt;
        if (m_timer >= it->second.frameTime) {
            m_timer -= it->second.frameTime;
            m_frame++;
            if (m_frame >= it->second.frameCount)
                m_frame = it->second.loop ? 0 : it->second.frameCount - 1;
        }
    }
    int GetFrameIndex() const {
        auto it = m_anims.find(m_state);
        return it != m_anims.end() ? it->second.startFrame + m_frame : 0;
    }
    AnimState GetState() const { return m_state; }
private:
    std::map<AnimState, Animation> m_anims;
    AnimState m_state = AnimState::Idle;
    int m_frame = 0;
    float m_timer = 0;
};

// 全局变量
std::unique_ptr<Renderer2D> g_renderer;
std::unique_ptr<SpriteRenderer> g_spriteRenderer;
std::shared_ptr<Texture> g_playerTex, g_tilesetTex;
std::unique_ptr<Camera2D> g_camera;
std::unique_ptr<Tilemap> g_tilemap;
AnimationController g_animator;

Vec2 g_playerPos(400, 86), g_playerVel(0, 0);  // 地面顶部y=64，碰撞体半高22，中心在64+22=86
bool g_facingRight = true, g_isGrounded = true, g_useBounds = true;
int g_smoothMode = 1;
int g_bgStyle = 0;  // 0=深蓝, 1=天蓝, 2=黄昏, 3=夜晚

const float GRAVITY = -1500.f, MOVE_SPEED = 350.f, JUMP_FORCE = 600.f;

void CalculateFrameUV(int idx, int cols, int rows, Vec2& uvMin, Vec2& uvMax) {
    int col = idx % cols, row = idx / cols;
    uvMin = Vec2(col/(float)cols, 1.f - (row+1)/(float)rows);
    uvMax = Vec2((col+1)/(float)cols, 1.f - row/(float)rows);
}

void CreateLargeMap() {
    g_tilemap = std::make_unique<Tilemap>();
    int w = 80, h = 30, ts = 32;  // 增加高度到30格（960像素），大于屏幕720
    g_tilemap->Create(w, h, ts);
    
    // 只创建地面层和装饰层
    int ground = g_tilemap->AddLayer("ground", true);
    int decor = g_tilemap->AddLayer("decor", false);
    
    // 地面（底部2层）
    for (int x = 0; x < w; x++) {
        g_tilemap->SetTile(ground, x, 0, 11);  // 底层泥土
        g_tilemap->SetTile(ground, x, 1, 9);   // 草地
    }
    
    // 悬空平台
    for (int x = 10; x < 18; x++) g_tilemap->SetTile(ground, x, 5, 9);
    for (int x = 25; x < 35; x++) g_tilemap->SetTile(ground, x, 8, 9);
    for (int x = 45; x < 55; x++) g_tilemap->SetTile(ground, x, 6, 9);
    for (int x = 60; x < 75; x++) g_tilemap->SetTile(ground, x, 10, 9);
    
    // 左右墙壁
    for (int y = 2; y < 20; y++) {
        g_tilemap->SetTile(ground, 0, y, 17);
        g_tilemap->SetTile(ground, w-1, y, 17);
    }
    
    // 装饰物
    g_tilemap->SetTile(decor, 14, 6, 29);
    g_tilemap->SetTile(decor, 30, 9, 30);
    g_tilemap->SetTile(decor, 50, 7, 29);
}

struct ColResult { bool ground=false, ceiling=false, wallL=false, wallR=false; };

ColResult MoveWithCollision(Vec2& pos, Vec2& vel, const Vec2& size, float dt) {
    ColResult r;
    vel.y += GRAVITY * dt;
    if (vel.y < -800.f) vel.y = -800.f;
    
    // 修复：使用AABB(center, halfSize)构造，之前误用了4-float构造函数
    // AABB(float,float,float,float) = (centerX, centerY, width, height)
    // 而不是 (minX, minY, maxX, maxY)，所以要用FromMinMax或center+halfSize
    Vec2 halfSz = size * 0.5f;
    
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

bool Initialize() {
    Engine& engine = Engine::Instance();
    int sw = engine.GetWindowWidth(), sh = engine.GetWindowHeight();
    
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) return false;
    g_spriteRenderer = std::make_unique<SpriteRenderer>();
    if (!g_spriteRenderer->Initialize()) return false;
    
    // 加载robot3角色的idle动画作为玩家纹理
    g_playerTex = std::make_shared<Texture>();
    const char* pp[] = {
        "data/texture/robot3/robot3_idle.png",
        "../data/texture/robot3/robot3_idle.png"
    };
    for (auto p : pp) {
        if (g_playerTex->Load(p)) {
            std::cout << "[Example12] Loaded robot3 texture: " << p << std::endl;
            break;
        }
    }
    if (!g_playerTex->IsValid()) {
        std::cout << "[Example12] WARNING: Robot texture not found!" << std::endl;
        g_playerTex->CreateSolidColor(Vec4(0.3f,0.6f,1,1), 64, 64);
    }
    
    g_tilesetTex = std::make_shared<Texture>();
    const char* tp[] = {"data/texture/tileset.png", "../data/texture/tileset.png"};
    for (auto p : tp) if (g_tilesetTex->Load(p)) break;
    
    CreateLargeMap();
    
    g_camera = std::make_unique<Camera2D>();
    g_camera->SetViewportSize((float)sw, (float)sh);
    g_camera->SetSmoothSpeed(5.0f);
    g_camera->SetWorldBounds(0, 0, (float)g_tilemap->GetPixelWidth(), (float)g_tilemap->GetPixelHeight());
    g_camera->SetTarget(g_playerPos);
    g_camera->SnapToTarget();
    
    // robot3_idle.png: 7列x7行, 每帧200x200, 共49帧
    // 使用前几行作为idle动画
    g_animator.AddAnimation(AnimState::Idle, Animation("Idle", 0, 7, 0.12f, true));   // 第1行
    g_animator.AddAnimation(AnimState::Run, Animation("Run", 7, 7, 0.08f, true));     // 第2行
    g_animator.AddAnimation(AnimState::Jump, Animation("Jump", 14, 7, 0.1f, false));  // 第3行
    
    std::cout << "[Example12] Camera System Ready! Map: " << g_tilemap->GetPixelWidth() << "x" << g_tilemap->GetPixelHeight() << std::endl;
    return true;
}

void Update(float dt) {
    Engine& engine = Engine::Instance();
    GLFWwindow* w = engine.GetWindow();
    if (dt > 0.1f) dt = 0.1f;
    
    static bool k1L=0,k2L=0,k3L=0,bL=0,zL=0,jL=0,cL=0;
    bool k1=glfwGetKey(w,GLFW_KEY_1), k2=glfwGetKey(w,GLFW_KEY_2), k3=glfwGetKey(w,GLFW_KEY_3);
    bool bK=glfwGetKey(w,GLFW_KEY_B), zK=glfwGetKey(w,GLFW_KEY_Z), cK=glfwGetKey(w,GLFW_KEY_C);
    
    if (k1&&!k1L) { g_smoothMode=0; g_camera->SetSmoothSpeed(10.f); }
    if (k2&&!k2L) { g_smoothMode=1; g_camera->SetSmoothSpeed(5.f); }
    if (k3&&!k3L) { g_smoothMode=2; g_camera->SetSmoothSpeed(2.f); }
    if (cK&&!cL) { g_bgStyle = (g_bgStyle + 1) % 4; }  // 切换背景
    cL = cK;
    if (bK&&!bL) {
        g_useBounds = !g_useBounds;
        if (g_useBounds) g_camera->SetWorldBounds(0,0,(float)g_tilemap->GetPixelWidth(),(float)g_tilemap->GetPixelHeight());
        else g_camera->ClearWorldBounds();
    }
    if (zK&&!zL) g_camera->Shake(15.f, 0.3f);
    k1L=k1; k2L=k2; k3L=k3; bL=bK; zL=zK;
    
    if (glfwGetKey(w,GLFW_KEY_R)) { g_playerPos=Vec2(400,86); g_playerVel=Vec2::Zero(); g_camera->SnapToTarget(); }
    
    bool moving = false;
    if (glfwGetKey(w,GLFW_KEY_A)) { g_playerVel.x=-MOVE_SPEED; g_facingRight=false; moving=true; }
    else if (glfwGetKey(w,GLFW_KEY_D)) { g_playerVel.x=MOVE_SPEED; g_facingRight=true; moving=true; }
    else { g_playerVel.x*=0.85f; if(std::abs(g_playerVel.x)<10)g_playerVel.x=0; }
    
    bool jK = glfwGetKey(w,GLFW_KEY_SPACE);
    if (jK&&!jL&&g_isGrounded) { g_playerVel.y=JUMP_FORCE; g_isGrounded=false; }
    jL = jK;
    
    ColResult col = MoveWithCollision(g_playerPos, g_playerVel, Vec2(28,44), dt);
    g_isGrounded = col.ground;
    
    if (!g_isGrounded) g_animator.Play(AnimState::Jump);
    else if (moving) g_animator.Play(AnimState::Run);
    else g_animator.Play(AnimState::Idle);
    g_animator.Update(dt);
    
    g_camera->SetTarget(g_playerPos);
    g_camera->Update(dt);
}

void Render() {
    Engine& engine = Engine::Instance();
    int sw = engine.GetWindowWidth(), sh = engine.GetWindowHeight();
    Vec2 camPos = g_camera->GetPosition();
    
    // 根据背景样式设置清屏颜色
    Vec4 bgColors[] = {
        Vec4(0.1f, 0.1f, 0.2f, 1.0f),   // 深蓝夜空
        Vec4(0.4f, 0.7f, 0.9f, 1.0f),   // 天蓝白天
        Vec4(0.9f, 0.5f, 0.3f, 1.0f),   // 黄昏
        Vec4(0.05f, 0.05f, 0.1f, 1.0f)  // 深夜
    };
    Vec4 bg = bgColors[g_bgStyle];
    glClearColor(bg.x, bg.y, bg.z, bg.w);
    glClear(GL_COLOR_BUFFER_BIT);
    
    Mat4 proj = Mat4::Ortho(0,(float)sw,0,(float)sh,-1,1);
    Mat4 view = g_camera->GetViewMatrix();
    g_spriteRenderer->SetProjection(proj * view);
    g_spriteRenderer->Begin();
    
    if (g_tilesetTex->IsValid()) {
        int tw = g_tilemap->GetTileWidth(), th = g_tilemap->GetTileHeight();
        int sx = std::max(0,(int)(camPos.x/tw)-1), sy = std::max(0,(int)(camPos.y/th)-1);
        int ex = std::min(g_tilemap->GetWidth()-1,(int)((camPos.x+sw)/tw)+1);
        int ey = std::min(g_tilemap->GetHeight()-1,(int)((camPos.y+sh)/th)+1);
        
        for (int l = 0; l < g_tilemap->GetLayerCount(); l++) {
            const TilemapLayer* layer = g_tilemap->GetLayer(l);
            if (!layer || !layer->visible) continue;
            for (int y = sy; y <= ey; y++) {
                for (int x = sx; x <= ex; x++) {
                    int id = layer->GetTile(x, y);
                    if (id == 0) continue;
                    int idx = id - 1, col = idx % 8, row = idx / 8;
                    Vec2 uvMin(col/8.f, 1.f-(row+1)/5.f), uvMax((col+1)/8.f, 1.f-row/5.f);
                    Sprite tile;
                    tile.SetTexture(g_tilesetTex);
                    tile.SetPosition(Vec2(x*tw+tw*0.5f, y*th+th*0.5f));
                    tile.SetSize(Vec2((float)tw,(float)th));
                    tile.SetUV(uvMin, uvMax);
                    tile.SetColor(Vec4(1,1,1,layer->opacity));
                    g_spriteRenderer->DrawSprite(tile);
                }
            }
        }
    }
    
    // 渲染玩家 - 总是渲染，即使纹理无效也用颜色方块
    {
        Vec2 uvMin, uvMax;
        CalculateFrameUV(g_animator.GetFrameIndex(), 7, 7, uvMin, uvMax);
        if (!g_facingRight) std::swap(uvMin.x, uvMax.x);
        
        Sprite player;
        if (g_playerTex && g_playerTex->IsValid()) {
            player.SetTexture(g_playerTex);
            player.SetUV(uvMin, uvMax);
        }
        player.SetPosition(g_playerPos);
        player.SetSize(Vec2(200, 200));  // robot3每帧200x200
        player.SetColor(Vec4(1, 1, 1, 1));
        g_spriteRenderer->DrawSprite(player);
    }
    
    g_spriteRenderer->End();
}

void RenderImGui() {
    Engine& e = Engine::Instance();
    Vec2 camPos = g_camera->GetPosition();
    
    ImGui::Begin("Camera System");
    ImGui::Text("FPS: %.1f", e.GetFPS());
    ImGui::Separator();
    ImGui::Text("Camera: (%.0f, %.0f)", camPos.x, camPos.y);
    ImGui::Text("Target: (%.0f, %.0f)", g_camera->GetTarget().x, g_camera->GetTarget().y);
    ImGui::Text("Smooth: %.1f", g_camera->GetSmoothSpeed());
    ImGui::Text("Bounds: %s", g_useBounds ? "ON" : "OFF");
    ImGui::Text("Shaking: %s", g_camera->IsShaking() ? "Yes" : "No");
    ImGui::Separator();
    ImGui::Text("Player: (%.0f, %.0f)", g_playerPos.x, g_playerPos.y);
    ImGui::Text("Grounded: %s", g_isGrounded ? "Yes" : "No");
    ImGui::Text("Player Texture: %s", (g_playerTex && g_playerTex->IsValid()) ? "OK" : "MISSING");
    ImGui::End();
    
    ImGui::Begin("Controls");
    ImGui::BulletText("A/D - Move");
    ImGui::BulletText("SPACE - Jump");
    ImGui::BulletText("1/2/3 - Speed (Fast/Med/Slow)");
    ImGui::BulletText("B - Toggle bounds");
    ImGui::BulletText("Z - Camera shake");
    ImGui::BulletText("R - Reset");
    const char* modes[] = {"Fast(10)","Medium(5)","Slow(2)"};
    ImGui::Text("Mode: %s", modes[g_smoothMode]);
    ImGui::End();
}

void Cleanup() {
    g_camera.reset(); g_tilemap.reset();
    g_tilesetTex.reset(); g_playerTex.reset();
    g_spriteRenderer.reset(); g_renderer.reset();
}

int main() {
    std::cout << "=== Example 12: Camera System ===" << std::endl;
    Engine& engine = Engine::Instance();
    EngineConfig cfg; cfg.windowWidth=1280; cfg.windowHeight=720;
    cfg.windowTitle = "Mini Engine - Camera System";
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
