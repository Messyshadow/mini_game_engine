/**
 * 示例03：矩阵变换
 * 
 * 学习目标：
 * 1. 理解Model矩阵（平移、旋转、缩放）
 * 2. 理解投影矩阵（正交投影）
 * 3. 使用像素坐标而不是NDC坐标
 * 4. 实现可移动、可旋转、可缩放的精灵
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "math/Math.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <cmath>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 全局状态
// ============================================================================

std::unique_ptr<Renderer2D> g_renderer;

// 精灵变换参数
Vec2 g_position(640.0f, 360.0f);  // 像素坐标
float g_rotation = 0.0f;          // 弧度
Vec2 g_scale(100.0f, 100.0f);     // 像素大小

// 动画
bool g_autoRotate = true;
float g_rotationSpeed = 1.0f;

// 投影矩阵
Mat4 g_projection;

// ============================================================================
// 更新函数
// ============================================================================

void Update(float deltaTime) {
    // 自动旋转
    if (g_autoRotate) {
        g_rotation += g_rotationSpeed * deltaTime;
        if (g_rotation > TWO_PI) g_rotation -= TWO_PI;
    }
    
    // 键盘移动
    Engine& engine = Engine::Instance();
    GLFWwindow* window = engine.GetWindow();
    
    float moveSpeed = 200.0f * deltaTime;  // 每秒200像素
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) g_position.y += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) g_position.y -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) g_position.x -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) g_position.x += moveSpeed;
    
    // Q/E 手动旋转
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) g_rotation += 2.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) g_rotation -= 2.0f * deltaTime;
}

// ============================================================================
// 渲染函数
// ============================================================================

/**
 * 将变换后的顶点从像素坐标转换到NDC
 */
Vec2 TransformVertex(const Vec2& localPos, const Mat4& model, const Mat4& projection) {
    // 1. 应用Model变换（本地坐标 → 世界坐标/像素坐标）
    Vec4 worldPos = model * Vec4(localPos, 0.0f, 1.0f);
    
    // 2. 应用Projection变换（像素坐标 → NDC）
    Vec4 clipPos = projection * worldPos;
    
    return Vec2(clipPos.x, clipPos.y);
}

void Render() {
    Engine& engine = Engine::Instance();
    int width = engine.GetWindowWidth();
    int height = engine.GetWindowHeight();
    
    // 创建正交投影矩阵
    // 将像素坐标 [0, width] x [0, height] 映射到 NDC [-1, 1] x [-1, 1]
    g_projection = Mat4::Ortho2D(0.0f, (float)width, 0.0f, (float)height);
    
    // ========================================================================
    // 绘制网格（像素坐标）
    // ========================================================================
    float gridSpacing = 100.0f;  // 100像素间隔
    Vec4 gridColor(0.2f, 0.2f, 0.2f, 1.0f);
    
    for (float x = 0; x <= width; x += gridSpacing) {
        Vec2 start = TransformVertex(Vec2(x, 0), Mat4::Identity(), g_projection);
        Vec2 end = TransformVertex(Vec2(x, (float)height), Mat4::Identity(), g_projection);
        g_renderer->DrawLine(start, end, gridColor, 1.0f);
    }
    for (float y = 0; y <= height; y += gridSpacing) {
        Vec2 start = TransformVertex(Vec2(0, y), Mat4::Identity(), g_projection);
        Vec2 end = TransformVertex(Vec2((float)width, y), Mat4::Identity(), g_projection);
        g_renderer->DrawLine(start, end, gridColor, 1.0f);
    }
    
    // ========================================================================
    // 绘制坐标轴原点指示
    // ========================================================================
    {
        Vec2 origin = TransformVertex(Vec2(0, 0), Mat4::Identity(), g_projection);
        Vec2 xEnd = TransformVertex(Vec2(100, 0), Mat4::Identity(), g_projection);
        Vec2 yEnd = TransformVertex(Vec2(0, 100), Mat4::Identity(), g_projection);
        
        g_renderer->DrawLine(origin, xEnd, Vec4::Red(), 2.0f);
        g_renderer->DrawLine(origin, yEnd, Vec4::Green(), 2.0f);
    }
    
    // ========================================================================
    // 绘制变换后的精灵（正方形）
    // ========================================================================
    
    /**
     * 构建Model矩阵
     * 
     * Model = Translation * Rotation * Scale
     * 
     * 注意：矩阵乘法从右到左应用
     * 所以实际变换顺序是：先缩放，再旋转，最后平移
     */
    Mat4 model = Mat4::TRS2D(g_position, g_rotation, Vec2(1.0f, 1.0f));
    
    // 正方形的本地坐标（以中心为原点）
    // 缩放已经在model矩阵中处理，这里用g_scale作为大小
    float halfW = g_scale.x / 2.0f;
    float halfH = g_scale.y / 2.0f;
    
    Vec2 localVerts[4] = {
        Vec2(-halfW, -halfH),  // 左下
        Vec2( halfW, -halfH),  // 右下
        Vec2( halfW,  halfH),  // 右上
        Vec2(-halfW,  halfH),  // 左上
    };
    
    // 变换顶点
    Vec2 transformedVerts[4];
    for (int i = 0; i < 4; i++) {
        transformedVerts[i] = TransformVertex(localVerts[i], model, g_projection);
    }
    
    // 绘制填充的正方形
    Vec4 fillColor(0.3f, 0.5f, 0.8f, 0.8f);
    g_renderer->DrawTriangle(transformedVerts[0], transformedVerts[1], transformedVerts[2], fillColor);
    g_renderer->DrawTriangle(transformedVerts[0], transformedVerts[2], transformedVerts[3], fillColor);
    
    // 绘制边框
    Vec4 borderColor = Vec4::White();
    g_renderer->DrawLine(transformedVerts[0], transformedVerts[1], borderColor, 2.0f);
    g_renderer->DrawLine(transformedVerts[1], transformedVerts[2], borderColor, 2.0f);
    g_renderer->DrawLine(transformedVerts[2], transformedVerts[3], borderColor, 2.0f);
    g_renderer->DrawLine(transformedVerts[3], transformedVerts[0], borderColor, 2.0f);
    
    // 绘制中心点
    Vec2 center = TransformVertex(Vec2(0, 0), model, g_projection);
    g_renderer->DrawPoint(center, Vec4::Yellow(), 8.0f);
    
    // 绘制前向方向（显示旋转）
    Vec2 forward = TransformVertex(Vec2(halfW * 1.5f, 0), model, g_projection);
    g_renderer->DrawLine(center, forward, Vec4::Red(), 3.0f);
}

// ============================================================================
// ImGui界面
// ============================================================================

void RenderImGui() {
    Engine& engine = Engine::Instance();
    
    ImGui::Begin("Transform Demo");
    {
        ImGui::Text("FPS: %.1f", engine.GetFPS());
        ImGui::Text("Window: %d x %d", engine.GetWindowWidth(), engine.GetWindowHeight());
        
        ImGui::Separator();
        ImGui::Text("Controls: WASD to move, Q/E to rotate");
        
        ImGui::Separator();
        ImGui::Text("Transform:");
        
        // 位置（像素）
        ImGui::DragFloat2("Position (px)", &g_position.x, 1.0f, 0.0f, 1280.0f);
        
        // 旋转（度）
        float rotDegrees = ToDegrees(g_rotation);
        if (ImGui::DragFloat("Rotation (deg)", &rotDegrees, 1.0f, -360.0f, 360.0f)) {
            g_rotation = ToRadians(rotDegrees);
        }
        
        // 缩放（像素大小）
        ImGui::DragFloat2("Size (px)", &g_scale.x, 1.0f, 10.0f, 500.0f);
        
        ImGui::Separator();
        ImGui::Checkbox("Auto Rotate", &g_autoRotate);
        if (g_autoRotate) {
            ImGui::SliderFloat("Rotation Speed", &g_rotationSpeed, 0.1f, 5.0f);
        }
        
        ImGui::Separator();
        if (ImGui::Button("Reset")) {
            g_position = Vec2(640.0f, 360.0f);
            g_rotation = 0.0f;
            g_scale = Vec2(100.0f, 100.0f);
        }
    }
    ImGui::End();
    
    // 矩阵显示面板
    ImGui::Begin("Matrix Info");
    {
        ImGui::Text("Model Matrix (TRS):");
        Mat4 model = Mat4::TRS2D(g_position, g_rotation, Vec2(1.0f, 1.0f));
        
        ImGui::Text("| %6.2f %6.2f %6.2f %6.2f |", model.m[0], model.m[4], model.m[8], model.m[12]);
        ImGui::Text("| %6.2f %6.2f %6.2f %6.2f |", model.m[1], model.m[5], model.m[9], model.m[13]);
        ImGui::Text("| %6.2f %6.2f %6.2f %6.2f |", model.m[2], model.m[6], model.m[10], model.m[14]);
        ImGui::Text("| %6.2f %6.2f %6.2f %6.2f |", model.m[3], model.m[7], model.m[11], model.m[15]);
        
        ImGui::Separator();
        ImGui::Text("Projection Matrix (Ortho2D):");
        ImGui::Text("| %6.3f %6.3f %6.3f %6.3f |", g_projection.m[0], g_projection.m[4], g_projection.m[8], g_projection.m[12]);
        ImGui::Text("| %6.3f %6.3f %6.3f %6.3f |", g_projection.m[1], g_projection.m[5], g_projection.m[9], g_projection.m[13]);
        ImGui::Text("| %6.3f %6.3f %6.3f %6.3f |", g_projection.m[2], g_projection.m[6], g_projection.m[10], g_projection.m[14]);
        ImGui::Text("| %6.3f %6.3f %6.3f %6.3f |", g_projection.m[3], g_projection.m[7], g_projection.m[11], g_projection.m[15]);
        
        ImGui::Separator();
        ImGui::TextWrapped(
            "Transformation Pipeline:\n"
            "LocalPos -> Model -> WorldPos -> Projection -> NDC\n\n"
            "Model = T * R * S\n"
            "(Scale first, then Rotate, then Translate)"
        );
    }
    ImGui::End();
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Mini Game Engine - Example 03" << std::endl;
    std::cout << "  Matrix Transforms" << std::endl;
    std::cout << "========================================" << std::endl;
    
    Engine& engine = Engine::Instance();
    
    EngineConfig config;
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.windowTitle = "Mini Engine - Matrix Transforms";
    
    if (!engine.Initialize(config)) return -1;
    
    g_renderer = std::make_unique<Renderer2D>();
    if (!g_renderer->Initialize()) return -1;
    
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    
    g_renderer->Shutdown();
    engine.Shutdown();
    
    return 0;
}
