/**
 * 示例02：三角形渲染和向量可视化
 */

#include "engine/Engine.h"
#include "engine/Renderer2D.h"
#include "math/Math.h"
#include <imgui.h>
#include <iostream>
#include <memory>
#include <cmath>

using namespace MiniEngine;
using namespace MiniEngine::Math;

std::unique_ptr<Renderer2D> g_renderer;

enum class DemoMode { BasicShapes, VectorOperations, DotProduct, CrossProduct };
DemoMode g_currentMode = DemoMode::BasicShapes;

Vec2 g_vectorA(0.5f, 0.3f);
Vec2 g_vectorB(0.2f, 0.6f);
float g_animTime = 0.0f;
bool g_animateVectors = false;
float g_rotationSpeed = 1.0f;

void RenderBasicShapes() {
    g_renderer->DrawGrid(0.2f, Vec4(0.2f, 0.2f, 0.2f, 1.0f));
    g_renderer->DrawAxes(0.9f);
    
    // 彩色三角形
    g_renderer->DrawTriangle(
        Vertex2D(-0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f),
        Vertex2D( 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f),
        Vertex2D( 0.0f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f)
    );
    
    g_renderer->DrawRect(Vec2(-0.9f, 0.6f), Vec2(0.3f, 0.3f), Vec4::Yellow());
    g_renderer->DrawCircle(Vec2(0.7f, 0.7f), 0.15f, Vec4::Cyan(), 32);
}

void RenderVectorOperations() {
    g_renderer->DrawGrid(0.2f, Vec4(0.2f, 0.2f, 0.2f, 1.0f));
    g_renderer->DrawAxes(0.9f);
    
    g_renderer->DrawVector(Vec2::Zero(), g_vectorA, Vec4::Red(), 1.0f);
    g_renderer->DrawVector(Vec2::Zero(), g_vectorB, Vec4::Green(), 1.0f);
    
    Vec2 sum = g_vectorA + g_vectorB;
    g_renderer->DrawVector(Vec2::Zero(), sum, Vec4::Yellow(), 1.0f);
    g_renderer->DrawVector(g_vectorA, g_vectorB, Vec4(0.0f, 0.5f, 0.0f, 0.8f), 1.0f);
    
    Vec2 diff = g_vectorA - g_vectorB;
    g_renderer->DrawVector(Vec2::Zero(), diff, Vec4::Cyan(), 1.0f);
}

void RenderDotProduct() {
    g_renderer->DrawGrid(0.2f, Vec4(0.2f, 0.2f, 0.2f, 1.0f));
    g_renderer->DrawAxes(0.9f);
    
    Vec2 a = g_vectorA.Normalized() * 0.7f;
    Vec2 b = g_vectorB.Normalized() * 0.7f;
    
    g_renderer->DrawVector(Vec2::Zero(), a, Vec4::Red(), 1.0f);
    g_renderer->DrawVector(Vec2::Zero(), b, Vec4::Green(), 1.0f);
    
    Vec2 aNorm = a.Normalized();
    Vec2 projection = aNorm * (b.Dot(aNorm));
    g_renderer->DrawLine(b, projection, Vec4(0.5f, 0.5f, 0.5f, 0.8f), 1.0f);
    g_renderer->DrawVector(Vec2::Zero(), projection, Vec4::Blue(), 1.0f);
}

void RenderCrossProduct() {
    g_renderer->DrawGrid(0.2f, Vec4(0.2f, 0.2f, 0.2f, 1.0f));
    g_renderer->DrawAxes(0.9f);
    
    g_renderer->DrawVector(Vec2::Zero(), g_vectorA, Vec4::Red(), 1.0f);
    g_renderer->DrawVector(Vec2::Zero(), g_vectorB, Vec4::Green(), 1.0f);
    
    float cross = g_vectorA.Cross(g_vectorB);
    Vec4 parallelogramColor = (cross > 0) ? Vec4(0.0f, 0.5f, 1.0f, 0.3f) : Vec4(1.0f, 0.5f, 0.0f, 0.3f);
    
    g_renderer->DrawTriangle(Vec2::Zero(), g_vectorA, g_vectorA + g_vectorB, parallelogramColor);
    g_renderer->DrawTriangle(Vec2::Zero(), g_vectorA + g_vectorB, g_vectorB, parallelogramColor);
}

void Update(float deltaTime) {
    g_animTime += deltaTime;
    if (g_animateVectors) {
        float angle = g_animTime * g_rotationSpeed;
        g_vectorB = Vec2(0.5f, 0.0f).Rotated(angle);
    }
}

void Render() {
    switch (g_currentMode) {
        case DemoMode::BasicShapes: RenderBasicShapes(); break;
        case DemoMode::VectorOperations: RenderVectorOperations(); break;
        case DemoMode::DotProduct: RenderDotProduct(); break;
        case DemoMode::CrossProduct: RenderCrossProduct(); break;
    }
}

void RenderImGui() {
    Engine& engine = Engine::Instance();
    
    ImGui::Begin("Vector Demo Controls");
    ImGui::Text("FPS: %.1f", engine.GetFPS());
    ImGui::Separator();
    
    if (ImGui::RadioButton("Basic Shapes", g_currentMode == DemoMode::BasicShapes)) g_currentMode = DemoMode::BasicShapes;
    if (ImGui::RadioButton("Vector Operations", g_currentMode == DemoMode::VectorOperations)) g_currentMode = DemoMode::VectorOperations;
    if (ImGui::RadioButton("Dot Product", g_currentMode == DemoMode::DotProduct)) g_currentMode = DemoMode::DotProduct;
    if (ImGui::RadioButton("Cross Product", g_currentMode == DemoMode::CrossProduct)) g_currentMode = DemoMode::CrossProduct;
    
    if (g_currentMode != DemoMode::BasicShapes) {
        ImGui::Separator();
        ImGui::DragFloat2("Vector A", &g_vectorA.x, 0.01f, -1.0f, 1.0f);
        ImGui::DragFloat2("Vector B", &g_vectorB.x, 0.01f, -1.0f, 1.0f);
        ImGui::Checkbox("Animate Vector B", &g_animateVectors);
        if (g_animateVectors) ImGui::SliderFloat("Speed", &g_rotationSpeed, 0.1f, 5.0f);
        
        ImGui::Separator();
        ImGui::Text("A . B = %.3f", g_vectorA.Dot(g_vectorB));
        ImGui::Text("A x B = %.3f", g_vectorA.Cross(g_vectorB));
        ImGui::Text("Angle = %.1f deg", ToDegrees(Vec2::AngleBetween(g_vectorA, g_vectorB)));
    }
    ImGui::End();
}

int main() {
    std::cout << "Mini Game Engine - Example 02" << std::endl;
    
    Engine& engine = Engine::Instance();
    EngineConfig config;
    config.windowTitle = "Mini Engine - Vector Visualization";
    
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
