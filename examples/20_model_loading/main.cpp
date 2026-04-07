/**
 * 第20章：3D模型加载 (3D Model Loading)
 *
 * 集成Assimp库，加载OBJ/FBX格式的3D模型。
 * 展示多种模型（角色、静态物体），支持ImGui实时编辑。
 *
 * 操作：
 * - 鼠标右键+拖动：旋转视角
 * - 滚轮：缩放
 * - E：显示/隐藏编辑面板
 * - 1~4：切换显示的模型
 * - L：切换光照
 * - W：切换线框模式
 */

#include "engine/Engine.h"
#include "engine/Shader.h"
#include "engine3d/Mesh3D.h"
#include "engine3d/ModelLoader.h"
#include "math/Math.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <cmath>
#include <vector>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 场景物体
// ============================================================================
struct SceneObject {
    Model3D model;
    Mesh3D primitiveMesh; // 备用：加载失败时用预设形状
    Vec3 position = Vec3(0, 0, 0);
    Vec3 rotation = Vec3(0, 0, 0);
    Vec3 scale = Vec3(1, 1, 1);
    Vec4 color = Vec4(1, 1, 1, 1);
    bool useVertexColor = true;
    bool visible = true;
    float autoRotateSpeed = 0;
    int autoRotateAxis = 1;
    std::string name;
    bool isModel = false; // true=用Model3D, false=用primitiveMesh
    
    void Draw() const {
        if (isModel && model.IsValid()) model.Draw();
        else if (primitiveMesh.IsValid()) primitiveMesh.Draw();
    }
    
    int GetVertexCount() const {
        if (isModel) return model.totalVertices;
        return primitiveMesh.GetVertexCount();
    }
    int GetTriangleCount() const {
        if (isModel) return model.totalTriangles;
        return primitiveMesh.GetIndexCount() / 3;
    }
};

// ============================================================================
// 全局状态
// ============================================================================
std::unique_ptr<Shader> g_shader;
std::vector<SceneObject> g_objects;
int g_selectedObject = 0;

// 轨道摄像机
struct OrbitCamera {
    float yaw = -0.5f, pitch = 0.4f, distance = 5.0f;
    Vec3 target = Vec3(0, 0.5f, 0);
    float fov = 45.0f, nearPlane = 0.1f, farPlane = 100.0f;
    
    Vec3 GetEye() const {
        return target + Vec3(
            distance * std::cos(pitch) * std::sin(yaw),
            distance * std::sin(pitch),
            distance * std::cos(pitch) * std::cos(yaw));
    }
    Mat4 GetView() const { return Mat4::LookAt(GetEye(), target, Vec3(0,1,0)); }
    Mat4 GetProj(float a) const { return Mat4::Perspective(fov*0.0174533f, a, nearPlane, farPlane); }
} g_camera;

// 光照
struct { Vec3 dir=Vec3(0.5f,0.8f,0.3f); Vec3 color=Vec3(1,0.95f,0.9f); float ambient=0.25f; bool on=true; } g_light;

// 显示
bool g_showEditor = true, g_wireframe = false;
Vec4 g_bgColor(0.12f, 0.12f, 0.18f, 1.0f);
bool g_mouseRDown = false;
double g_lastMX=0, g_lastMY=0;

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // 加载Shader
    g_shader = std::make_unique<Shader>();
    const char* vs[] = {"data/shader/basic3d.vs","../data/shader/basic3d.vs","../../data/shader/basic3d.vs"};
    const char* fs[] = {"data/shader/basic3d.fs","../data/shader/basic3d.fs","../../data/shader/basic3d.fs"};
    bool shaderOK = false;
    for (int i = 0; i < 3; i++) { if (g_shader->LoadFromFile(vs[i], fs[i])) { shaderOK = true; break; } }
    if (!shaderOK) { std::cerr << "Failed to load shader!" << std::endl; return false; }
    
    // ---- 加载模型 ----
    
    // 0: 地面（预设平面）
    {
        SceneObject obj;
        obj.name = "Ground Plane";
        obj.primitiveMesh = Mesh3D::CreatePlane(10.0f, 10.0f, 10, 10);
        obj.position = Vec3(0, 0, 0);
        obj.useVertexColor = false;
        obj.color = Vec4(0.3f, 0.4f, 0.3f, 1);
        g_objects.push_back(std::move(obj));
    }
    
    // 1: Mixamo角色 (X Bot)
    {
        SceneObject obj;
        obj.name = "X Bot (FBX)";
        obj.model = ModelLoader::LoadWithFallback("fbx/X Bot.fbx");
        obj.isModel = obj.model.IsValid();
        if (!obj.isModel) {
            obj.primitiveMesh = Mesh3D::CreateColorCube(1.0f);
            std::cout << "[Warning] X Bot not found, using cube" << std::endl;
        }
        obj.position = Vec3(0, 0, 0);
        obj.scale = Vec3(0.02f, 0.02f, 0.02f); // FBX通常很大需要缩小
        obj.autoRotateSpeed = 0.3f;
        g_objects.push_back(std::move(obj));
    }
    
    // 2: StopSign (OBJ)
    {
        SceneObject obj;
        obj.name = "Stop Sign (OBJ)";
        obj.model = ModelLoader::LoadWithFallback("obj/StopSign/StopSign.obj");
        obj.isModel = obj.model.IsValid();
        if (!obj.isModel) {
            obj.primitiveMesh = Mesh3D::CreateSphere(0.5f);
        }
        obj.position = Vec3(3, 0, 0);
        obj.scale = Vec3(0.5f, 0.5f, 0.5f);
        g_objects.push_back(std::move(obj));
    }
    
    // 3: 立方体参照物
    {
        SceneObject obj;
        obj.name = "Reference Cube";
        obj.primitiveMesh = Mesh3D::CreateColorCube(1.0f);
        obj.position = Vec3(-3, 0.5f, 0);
        obj.autoRotateSpeed = 0.5f;
        g_objects.push_back(std::move(obj));
    }
    
    // 4: 球体参照物
    {
        SceneObject obj;
        obj.name = "Reference Sphere";
        obj.primitiveMesh = Mesh3D::CreateSphere(0.6f, 24, 16);
        obj.position = Vec3(-3, 0.6f, 2.5f);
        obj.useVertexColor = false;
        obj.color = Vec4(0.9f, 0.5f, 0.2f, 1);
        g_objects.push_back(std::move(obj));
    }
    
    std::cout << "\n=== Chapter 20: 3D Model Loading ===" << std::endl;
    std::cout << "Right-drag: Rotate | Scroll: Zoom | E: Editor" << std::endl;
    std::cout << "L: Lighting | W: Wireframe" << std::endl;
    return true;
}

// ============================================================================
// 更新
// ============================================================================
void Update(float dt) {
    Engine& eng = Engine::Instance();
    GLFWwindow* w = eng.GetWindow();
    
    static bool eL=0; bool e=glfwGetKey(w,GLFW_KEY_E); if(e&&!eL)g_showEditor=!g_showEditor; eL=e;
    static bool lL=0; bool l=glfwGetKey(w,GLFW_KEY_L); if(l&&!lL)g_light.on=!g_light.on; lL=l;
    static bool wL=0; bool wk=glfwGetKey(w,GLFW_KEY_W); if(wk&&!wL)g_wireframe=!g_wireframe; wL=wk;
    
    // 鼠标右键旋转
    double mx, my; glfwGetCursorPos(w, &mx, &my);
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (!g_mouseRDown) { g_mouseRDown=true; g_lastMX=mx; g_lastMY=my; }
        g_camera.yaw += (float)(mx-g_lastMX)*0.005f;
        g_camera.pitch -= (float)(my-g_lastMY)*0.005f;
        if (g_camera.pitch > 1.5f) g_camera.pitch = 1.5f;
        if (g_camera.pitch < -1.5f) g_camera.pitch = -1.5f;
        g_lastMX=mx; g_lastMY=my;
    } else g_mouseRDown=false;
    
    // 自动旋转
    for (auto& obj : g_objects) {
        if (obj.autoRotateSpeed != 0) {
            float* axis = &obj.rotation.x + obj.autoRotateAxis;
            *axis += obj.autoRotateSpeed * dt;
        }
    }
}

// ============================================================================
// 渲染
// ============================================================================
void Render() {
    Engine& eng = Engine::Instance();
    int sw=eng.GetWindowWidth(), sh=eng.GetWindowHeight();
    float aspect=(float)sw/(float)sh;
    
    glClearColor(g_bgColor.x,g_bgColor.y,g_bgColor.z,g_bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (g_wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    g_shader->Bind();
    g_shader->SetMat4("uView", g_camera.GetView().m);
    g_shader->SetMat4("uProjection", g_camera.GetProj(aspect).m);
    g_shader->SetBool("uUseLighting", g_light.on);
    g_shader->SetVec3("uLightDir", g_light.dir);
    g_shader->SetVec3("uLightColor", g_light.color);
    g_shader->SetFloat("uAmbientStrength", g_light.ambient);
    
    for (auto& obj : g_objects) {
        if (!obj.visible) continue;
        
        Mat4 model = Mat4::Translate(obj.position.x, obj.position.y, obj.position.z)
                   * Mat4::RotateY(obj.rotation.y)
                   * Mat4::RotateX(obj.rotation.x)
                   * Mat4::RotateZ(obj.rotation.z)
                   * Mat4::Scale(obj.scale.x, obj.scale.y, obj.scale.z);
        
        g_shader->SetMat4("uModel", model.m);
        g_shader->SetVec4("uBaseColor", obj.color);
        g_shader->SetBool("uUseVertexColor", obj.useVertexColor);
        
        obj.Draw();
    }
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui() {
    if (!g_showEditor) return;
    
    ImGui::Begin("3D Model Viewer [E]", &g_showEditor, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %.0f", Engine::Instance().GetFPS());
    
    if (ImGui::BeginTabBar("Tabs")) {
        // ---- Camera ----
        if (ImGui::BeginTabItem("Camera")) {
            ImGui::SliderFloat("Yaw", &g_camera.yaw, -3.14f, 3.14f);
            ImGui::SliderFloat("Pitch", &g_camera.pitch, -1.5f, 1.5f);
            ImGui::SliderFloat("Distance", &g_camera.distance, 1.0f, 30.0f);
            ImGui::DragFloat3("Target", &g_camera.target.x, 0.1f);
            ImGui::SliderFloat("FOV", &g_camera.fov, 10, 120);
            Vec3 eye = g_camera.GetEye();
            ImGui::Text("Eye: (%.1f, %.1f, %.1f)", eye.x, eye.y, eye.z);
            ImGui::EndTabItem();
        }
        
        // ---- Objects ----
        if (ImGui::BeginTabItem("Objects")) {
            for (int i = 0; i < (int)g_objects.size(); i++) {
                auto& obj = g_objects[i];
                ImGui::PushID(i);
                
                bool open = ImGui::TreeNode("##obj", "[%d] %s", i, obj.name.c_str());
                if (open) {
                    ImGui::Checkbox("Visible", &obj.visible);
                    ImGui::DragFloat3("Position", &obj.position.x, 0.05f);
                    
                    Vec3 rotDeg(obj.rotation.x*57.2958f, obj.rotation.y*57.2958f, obj.rotation.z*57.2958f);
                    if (ImGui::DragFloat3("Rotation", &rotDeg.x, 1.0f))
                        obj.rotation = Vec3(rotDeg.x*0.0174533f, rotDeg.y*0.0174533f, rotDeg.z*0.0174533f);
                    
                    ImGui::DragFloat3("Scale", &obj.scale.x, 0.005f, 0.001f, 100.0f);
                    
                    ImGui::Checkbox("Vertex Color", &obj.useVertexColor);
                    if (!obj.useVertexColor) ImGui::ColorEdit4("Color", &obj.color.x);
                    
                    ImGui::SliderFloat("Auto Rotate", &obj.autoRotateSpeed, -3, 3);
                    const char* axes[]={"X","Y","Z"};
                    ImGui::Combo("Axis", &obj.autoRotateAxis, axes, 3);
                    
                    if (ImGui::Button("Reset")) { obj.rotation=Vec3(0,0,0); obj.scale=Vec3(1,1,1); obj.autoRotateSpeed=0; }
                    
                    // 模型信息
                    ImGui::Text("Source: %s", obj.isModel ? "Assimp Model" : "Primitive");
                    if (obj.isModel && obj.model.IsValid()) {
                        ImGui::Text("Meshes: %d", (int)obj.model.meshes.size());
                    }
                    ImGui::Text("Vertices: %d  Triangles: %d", obj.GetVertexCount(), obj.GetTriangleCount());
                    
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        
        // ---- Lighting ----
        if (ImGui::BeginTabItem("Lighting")) {
            ImGui::Checkbox("Enable (L)", &g_light.on);
            ImGui::DragFloat3("Direction", &g_light.dir.x, 0.01f, -1, 1);
            ImGui::ColorEdit3("Color", &g_light.color.x);
            ImGui::SliderFloat("Ambient", &g_light.ambient, 0, 1);
            ImGui::Separator();
            if (ImGui::Button("Daylight")) { g_light.dir=Vec3(0.5f,0.8f,0.3f); g_light.color=Vec3(1,0.95f,0.9f); g_light.ambient=0.25f; }
            ImGui::SameLine();
            if (ImGui::Button("Sunset")) { g_light.dir=Vec3(0.8f,0.2f,0.1f); g_light.color=Vec3(1,0.6f,0.3f); g_light.ambient=0.15f; }
            ImGui::SameLine();
            if (ImGui::Button("Moonlight")) { g_light.dir=Vec3(-0.3f,0.9f,-0.2f); g_light.color=Vec3(0.6f,0.7f,1); g_light.ambient=0.08f; }
            ImGui::EndTabItem();
        }
        
        // ---- Render ----
        if (ImGui::BeginTabItem("Render")) {
            ImGui::ColorEdit4("Background", &g_bgColor.x);
            ImGui::Checkbox("Wireframe (W)", &g_wireframe);
            ImGui::Separator();
            ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));
            ImGui::Text("OpenGL: %s", glGetString(GL_VERSION));
            int totalVerts=0, totalTris=0;
            for (auto& o : g_objects) { totalVerts+=o.GetVertexCount(); totalTris+=o.GetTriangleCount(); }
            ImGui::Text("Scene Total: %d vertices, %d triangles", totalVerts, totalTris);
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    ImGui::End();
}

// ============================================================================
// 滚轮 & 清理 & main
// ============================================================================
void ScrollCB(GLFWwindow*, double, double y) {
    g_camera.distance -= (float)y * 0.5f;
    if (g_camera.distance < 0.5f) g_camera.distance = 0.5f;
    if (g_camera.distance > 30.0f) g_camera.distance = 30.0f;
}

void Cleanup() {
    for (auto& o : g_objects) { o.model.Destroy(); o.primitiveMesh.Destroy(); }
    g_objects.clear();
    g_shader.reset();
}

int main() {
    std::cout << "=== Example 20: 3D Model Loading ===" << std::endl;
    Engine& engine = Engine::Instance();
    EngineConfig cfg; cfg.windowWidth=1280; cfg.windowHeight=720;
    cfg.windowTitle = "Mini Engine - 3D Model Loading";
    if (!engine.Initialize(cfg)) return -1;
    glfwSetScrollCallback(engine.GetWindow(), ScrollCB);
    if (!Initialize()) { engine.Shutdown(); return -1; }
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    Cleanup(); engine.Shutdown();
    return 0;
}
