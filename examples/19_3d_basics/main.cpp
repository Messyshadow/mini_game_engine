/**
 * 第19章：3D渲染基础 (3D Rendering Basics)
 *
 * 从2D迈入3D的第一步。学习透视投影、MVP管线、深度测试。
 * 渲染立方体、平面和球体，支持ImGui实时编辑所有参数。
 *
 * 操作：
 * - 鼠标右键+拖动：旋转视角
 * - 滚轮：缩放
 * - E：显示/隐藏编辑面板
 * - 1/2/3：切换物体（立方体/球体/全部）
 * - L：切换光照开关
 * - W：切换线框模式
 */

#include "engine/Engine.h"
#include "engine/Shader.h"
#include "engine3d/Mesh3D.h"
#include "math/Math.h"
#include <glad/gl.h>    // 必须在GLFW之前！glad加载OpenGL函数
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

std::unique_ptr<Shader> g_shader;

// 场景中的物体
struct SceneObject {
    Mesh3D mesh;
    Vec3 position = Vec3(0, 0, 0);
    Vec3 rotation = Vec3(0, 0, 0);   // 欧拉角（弧度）
    Vec3 scale = Vec3(1, 1, 1);
    Vec4 color = Vec4(1, 1, 1, 1);
    bool useVertexColor = true;
    bool visible = true;
    float autoRotateSpeed = 0;       // 自动旋转速度（弧度/秒）
    int autoRotateAxis = 1;          // 0=X, 1=Y, 2=Z
    std::string name;
};

SceneObject g_cube, g_plane, g_sphere;

// 摄像机（轨道摄像机）
struct OrbitCamera {
    float yaw = -0.5f;      // 水平角度（弧度）
    float pitch = 0.4f;     // 垂直角度
    float distance = 5.0f;  // 距离目标的距离
    Vec3 target = Vec3(0, 0, 0); // 看向的目标点
    float fov = 45.0f;      // 视野角度（度）
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    
    Vec3 GetEyePosition() const {
        float x = distance * std::cos(pitch) * std::sin(yaw);
        float y = distance * std::sin(pitch);
        float z = distance * std::cos(pitch) * std::cos(yaw);
        return target + Vec3(x, y, z);
    }
    
    Mat4 GetViewMatrix() const {
        return Mat4::LookAt(GetEyePosition(), target, Vec3(0, 1, 0));
    }
    
    Mat4 GetProjectionMatrix(float aspect) const {
        float fovRad = fov * 3.14159f / 180.0f;
        return Mat4::Perspective(fovRad, aspect, nearPlane, farPlane);
    }
} g_camera;

// 光照
struct LightParams {
    Vec3 direction = Vec3(0.5f, 0.8f, 0.3f); // 光照方向
    Vec3 color = Vec3(1.0f, 0.95f, 0.9f);    // 光照颜色
    float ambientStrength = 0.2f;
    bool enabled = true;
} g_light;

// 显示
bool g_showEditor = true;
bool g_wireframe = false;
Vec4 g_bgColor = Vec4(0.15f, 0.15f, 0.2f, 1.0f);
int g_displayMode = 2; // 0=立方体, 1=球体, 2=全部

// 鼠标控制
bool g_mouseRightDown = false;
double g_lastMouseX = 0, g_lastMouseY = 0;

// ============================================================================
// Shader加载
// ============================================================================
bool LoadShader() {
    g_shader = std::make_unique<Shader>();
    const char* vsPaths[] = {"data/shader/basic3d.vs", "../data/shader/basic3d.vs", "../../data/shader/basic3d.vs"};
    const char* fsPaths[] = {"data/shader/basic3d.fs", "../data/shader/basic3d.fs", "../../data/shader/basic3d.fs"};
    
    for (int i = 0; i < 3; i++) {
        if (g_shader->LoadFromFile(vsPaths[i], fsPaths[i])) {
            std::cout << "[Shader] Loaded basic3d shader" << std::endl;
            return true;
        }
    }
    std::cerr << "[Shader] Failed to load basic3d shader!" << std::endl;
    return false;
}

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    // 启用深度测试（3D必须！）
    glEnable(GL_DEPTH_TEST);
    
    // 启用背面剔除（优化：不渲染背向摄像机的三角形）
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    if (!LoadShader()) return false;
    
    // 创建场景物体
    g_cube.mesh = Mesh3D::CreateColorCube(1.5f);
    g_cube.name = "Cube";
    g_cube.position = Vec3(-1.5f, 0.75f, 0);
    g_cube.autoRotateSpeed = 0.8f;
    
    g_plane.mesh = Mesh3D::CreatePlane(8.0f, 8.0f, 8, 8);
    g_plane.name = "Ground";
    g_plane.position = Vec3(0, 0, 0);
    g_plane.useVertexColor = false;
    g_plane.color = Vec4(0.35f, 0.45f, 0.35f, 1.0f);
    
    g_sphere.mesh = Mesh3D::CreateSphere(0.8f, 24, 16);
    g_sphere.name = "Sphere";
    g_sphere.position = Vec3(1.5f, 0.8f, 0);
    g_sphere.autoRotateSpeed = 0.5f;
    g_sphere.autoRotateAxis = 0;
    g_sphere.useVertexColor = false;
    g_sphere.color = Vec4(0.3f, 0.6f, 1.0f, 1.0f);
    
    std::cout << "\n=== Chapter 19: 3D Rendering Basics ===" << std::endl;
    std::cout << "Right-click+drag: Rotate | Scroll: Zoom | E: Editor" << std::endl;
    std::cout << "1/2/3: Display mode | L: Lighting | W: Wireframe" << std::endl;
    return true;
}

// ============================================================================
// 渲染单个物体
// ============================================================================
void RenderObject(SceneObject& obj) {
    if (!obj.visible) return;
    
    // 构建Model矩阵：缩放 → 旋转 → 平移
    Mat4 model = Mat4::Translate(obj.position.x, obj.position.y, obj.position.z)
               * Mat4::RotateY(obj.rotation.y)
               * Mat4::RotateX(obj.rotation.x)
               * Mat4::RotateZ(obj.rotation.z)
               * Mat4::Scale(obj.scale.x, obj.scale.y, obj.scale.z);
    
    g_shader->SetMat4("uModel", model.m);
    g_shader->SetVec4("uBaseColor", obj.color);
    g_shader->SetBool("uUseVertexColor", obj.useVertexColor);
    
    obj.mesh.Draw();
}

// ============================================================================
// 更新
// ============================================================================
void Update(float dt) {
    Engine& eng = Engine::Instance();
    GLFWwindow* w = eng.GetWindow();
    
    // E键切换编辑面板
    static bool eL = false;
    bool e = glfwGetKey(w, GLFW_KEY_E);
    if (e && !eL) g_showEditor = !g_showEditor;
    eL = e;
    
    // L键切换光照
    static bool lL = false;
    bool l = glfwGetKey(w, GLFW_KEY_L);
    if (l && !lL) g_light.enabled = !g_light.enabled;
    lL = l;
    
    // W键切换线框
    static bool wL = false;
    bool wk = glfwGetKey(w, GLFW_KEY_W);
    if (wk && !wL) g_wireframe = !g_wireframe;
    wL = wk;
    
    // 1/2/3切换显示模式
    if (glfwGetKey(w, GLFW_KEY_1)) g_displayMode = 0;
    if (glfwGetKey(w, GLFW_KEY_2)) g_displayMode = 1;
    if (glfwGetKey(w, GLFW_KEY_3)) g_displayMode = 2;
    
    // 鼠标右键旋转摄像机
    double mx, my;
    glfwGetCursorPos(w, &mx, &my);
    
    // 鼠标右键旋转摄像机（左右=Yaw，上下=Pitch）
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (!g_mouseRightDown) {
            g_mouseRightDown = true;
            g_lastMouseX = mx; g_lastMouseY = my;
        }
        float dx = (float)(mx - g_lastMouseX) * 0.005f;
        float dy = (float)(my - g_lastMouseY) * 0.005f;
        g_camera.yaw += dx;
        g_camera.pitch -= dy;
        // 限制垂直角度
        if (g_camera.pitch > 1.5f) g_camera.pitch = 1.5f;
        if (g_camera.pitch < -1.5f) g_camera.pitch = -1.5f;
        g_lastMouseX = mx; g_lastMouseY = my;
    } else {
        g_mouseRightDown = false;
    }
    
    // 自动旋转物体
    if (g_cube.autoRotateSpeed != 0) {
        float* axis = &g_cube.rotation.x + g_cube.autoRotateAxis;
        *axis += g_cube.autoRotateSpeed * dt;
    }
    if (g_sphere.autoRotateSpeed != 0) {
        float* axis = &g_sphere.rotation.x + g_sphere.autoRotateAxis;
        *axis += g_sphere.autoRotateSpeed * dt;
    }
}

// ============================================================================
// 渲染
// ============================================================================
void Render() {
    Engine& eng = Engine::Instance();
    int sw = eng.GetWindowWidth(), sh = eng.GetWindowHeight();
    float aspect = (float)sw / (float)sh;
    
    glClearColor(g_bgColor.x, g_bgColor.y, g_bgColor.z, g_bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 注意：3D要清除深度缓冲！
    
    if (g_wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    g_shader->Bind();
    
    // 设置摄像机矩阵
    Mat4 view = g_camera.GetViewMatrix();
    Mat4 proj = g_camera.GetProjectionMatrix(aspect);
    g_shader->SetMat4("uView", view.m);
    g_shader->SetMat4("uProjection", proj.m);
    
    // 设置光照
    g_shader->SetBool("uUseLighting", g_light.enabled);
    g_shader->SetVec3("uLightDir", g_light.direction);
    g_shader->SetVec3("uLightColor", g_light.color);
    g_shader->SetFloat("uAmbientStrength", g_light.ambientStrength);
    
    // 渲染地面
    g_plane.visible = true;
    RenderObject(g_plane);
    
    // 渲染物体
    g_cube.visible = (g_displayMode == 0 || g_displayMode == 2);
    g_sphere.visible = (g_displayMode == 1 || g_displayMode == 2);
    RenderObject(g_cube);
    RenderObject(g_sphere);
    
    // 恢复填充模式（ImGui需要）
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// ============================================================================
// ImGui编辑面板
// ============================================================================
void RenderImGui() {
    if (!g_showEditor) return;
    
    Engine& eng = Engine::Instance();
    
    ImGui::Begin("3D Editor [E]", &g_showEditor, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %.0f", eng.GetFPS());
    
    if (ImGui::BeginTabBar("EditorTabs3D")) {
        // ---- 摄像机 ----
        if (ImGui::BeginTabItem("Camera")) {
            ImGui::Text("=== Orbit Camera ===");
            ImGui::SliderFloat("Yaw", &g_camera.yaw, -3.14f, 3.14f);
            ImGui::SliderFloat("Pitch", &g_camera.pitch, -1.5f, 1.5f);
            ImGui::SliderFloat("Distance", &g_camera.distance, 1.0f, 20.0f);
            ImGui::DragFloat3("Target", &g_camera.target.x, 0.1f);
            ImGui::Separator();
            ImGui::Text("=== Projection ===");
            ImGui::SliderFloat("FOV", &g_camera.fov, 10.0f, 120.0f);
            ImGui::SliderFloat("Near Plane", &g_camera.nearPlane, 0.01f, 1.0f);
            ImGui::SliderFloat("Far Plane", &g_camera.farPlane, 10.0f, 500.0f);
            ImGui::Separator();
            Vec3 eye = g_camera.GetEyePosition();
            ImGui::Text("Eye: (%.1f, %.1f, %.1f)", eye.x, eye.y, eye.z);
            ImGui::EndTabItem();
        }
        
        // ---- 物体 ----
        if (ImGui::BeginTabItem("Objects")) {
            auto EditObject = [](const char* label, SceneObject& obj) {
                if (ImGui::TreeNode(label)) {
                    ImGui::Checkbox("Visible", &obj.visible);
                    ImGui::DragFloat3("Position", &obj.position.x, 0.05f);
                    
                    // 旋转用度数显示（内部存弧度）
                    Vec3 rotDeg(obj.rotation.x * 57.2958f, obj.rotation.y * 57.2958f, obj.rotation.z * 57.2958f);
                    if (ImGui::DragFloat3("Rotation", &rotDeg.x, 1.0f)) {
                        obj.rotation = Vec3(rotDeg.x * 0.0174533f, rotDeg.y * 0.0174533f, rotDeg.z * 0.0174533f);
                    }
                    
                    ImGui::DragFloat3("Scale", &obj.scale.x, 0.05f, 0.1f, 10.0f);
                    
                    ImGui::Checkbox("Use Vertex Color", &obj.useVertexColor);
                    if (!obj.useVertexColor) {
                        ImGui::ColorEdit4("Color", &obj.color.x);
                    }
                    
                    ImGui::SliderFloat("Auto Rotate Speed", &obj.autoRotateSpeed, -3.0f, 3.0f);
                    const char* axes[] = {"X", "Y", "Z"};
                    ImGui::Combo("Rotate Axis", &obj.autoRotateAxis, axes, 3);
                    
                    if (ImGui::Button("Reset Transform")) {
                        obj.rotation = Vec3(0, 0, 0);
                        obj.scale = Vec3(1, 1, 1);
                        obj.autoRotateSpeed = 0;
                    }
                    
                    ImGui::Text("Vertices: %d  Triangles: %d", obj.mesh.GetVertexCount(), obj.mesh.GetIndexCount() / 3);
                    ImGui::TreePop();
                }
            };
            
            EditObject("Cube", g_cube);
            EditObject("Sphere", g_sphere);
            EditObject("Ground Plane", g_plane);
            ImGui::EndTabItem();
        }
        
        // ---- 光照 ----
        if (ImGui::BeginTabItem("Lighting")) {
            ImGui::Checkbox("Enable Lighting (L)", &g_light.enabled);
            ImGui::Separator();
            ImGui::DragFloat3("Direction", &g_light.direction.x, 0.01f, -1.0f, 1.0f);
            ImGui::ColorEdit3("Light Color", &g_light.color.x);
            ImGui::SliderFloat("Ambient", &g_light.ambientStrength, 0.0f, 1.0f);
            ImGui::Separator();
            // 预设
            if (ImGui::Button("Daylight")) { g_light.direction = Vec3(0.5f, 0.8f, 0.3f); g_light.color = Vec3(1, 0.95f, 0.9f); g_light.ambientStrength = 0.2f; }
            ImGui::SameLine();
            if (ImGui::Button("Sunset")) { g_light.direction = Vec3(0.8f, 0.2f, 0.1f); g_light.color = Vec3(1, 0.6f, 0.3f); g_light.ambientStrength = 0.15f; }
            ImGui::SameLine();
            if (ImGui::Button("Moonlight")) { g_light.direction = Vec3(-0.3f, 0.9f, -0.2f); g_light.color = Vec3(0.6f, 0.7f, 1.0f); g_light.ambientStrength = 0.08f; }
            ImGui::EndTabItem();
        }
        
        // ---- 渲染设置 ----
        if (ImGui::BeginTabItem("Render")) {
            ImGui::ColorEdit4("Background", &g_bgColor.x);
            ImGui::Checkbox("Wireframe (W)", &g_wireframe);
            
            ImGui::Separator();
            ImGui::Text("Display Mode:");
            ImGui::RadioButton("Cube Only", &g_displayMode, 0); ImGui::SameLine();
            ImGui::RadioButton("Sphere Only", &g_displayMode, 1); ImGui::SameLine();
            ImGui::RadioButton("All", &g_displayMode, 2);
            
            ImGui::Separator();
            ImGui::Text("=== OpenGL Info ===");
            ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));
            ImGui::Text("Version: %s", glGetString(GL_VERSION));
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    ImGui::End();
}

// ============================================================================
// 滚轮回调（缩放）
// ============================================================================
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    g_camera.distance -= (float)yoffset * 0.5f;
    if (g_camera.distance < 1.0f) g_camera.distance = 1.0f;
    if (g_camera.distance > 20.0f) g_camera.distance = 20.0f;
}

// ============================================================================
// 清理 & 主函数
// ============================================================================
void Cleanup() {
    g_cube.mesh.Destroy();
    g_sphere.mesh.Destroy();
    g_plane.mesh.Destroy();
    g_shader.reset();
}

int main() {
    std::cout << "=== Example 19: 3D Rendering Basics ===" << std::endl;
    
    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth = 1280;
    cfg.windowHeight = 720;
    cfg.windowTitle = "Mini Engine - 3D Rendering Basics";
    
    if (!engine.Initialize(cfg)) return -1;
    
    // 注册滚轮回调
    glfwSetScrollCallback(engine.GetWindow(), ScrollCallback);
    
    if (!Initialize()) { engine.Shutdown(); return -1; }
    
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    
    Cleanup();
    engine.Shutdown();
    return 0;
}
