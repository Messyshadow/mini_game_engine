/**
 * 第21章：光照系统 (Lighting System)
 *
 * Phong着色模型：环境光 + 漫反射 + 镜面反射高光
 * 三种光源：方向光、点光源、聚光灯
 *
 * 操作：
 * - 鼠标左键拖动：旋转视角（Yaw/Pitch）
 * - 滚轮：缩放
 * - E：编辑面板
 * - L：切换全部光照开关
 * - W：线框模式
 * - F：切换聚光灯（手电筒效果）
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
#include <string>

using namespace MiniEngine;
using namespace MiniEngine::Math;

// ============================================================================
// 光源数据结构（C++端，对应Shader中的struct）
// ============================================================================
struct DirLightData {
    Vec3 direction = Vec3(0.5f, 0.8f, 0.3f);
    Vec3 color = Vec3(1, 0.95f, 0.9f);
    float intensity = 1.0f;
    bool enabled = true;
    std::string name = "Directional Light";
};

struct PointLightData {
    Vec3 position = Vec3(0, 3, 0);
    Vec3 color = Vec3(1, 1, 1);
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    bool enabled = true;
    std::string name = "Point Light";
};

struct SpotLightData {
    Vec3 position = Vec3(0, 5, 0);
    Vec3 direction = Vec3(0, -1, 0);
    Vec3 color = Vec3(1, 1, 1);
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float cutOff = 12.5f;      // 内锥角（度）
    float outerCutOff = 17.5f; // 外锥角（度）
    bool enabled = false;
    std::string name = "Spot Light";
};

// ============================================================================
// 场景物体
// ============================================================================
struct SceneObj {
    Model3D model;
    Mesh3D primitive;
    Vec3 pos = Vec3(0,0,0), rot = Vec3(0,0,0), scl = Vec3(1,1,1);
    Vec4 color = Vec4(1,1,1,1);
    bool useVtxColor = true, visible = true, isModel = false;
    float autoRotSpd = 0; int autoRotAxis = 1;
    float shininess = 32.0f;
    std::string name;
    
    void Draw() const { if(isModel && model.IsValid()) model.Draw(); else if(primitive.IsValid()) primitive.Draw(); }
    int Verts() const { return isModel ? model.totalVertices : primitive.GetVertexCount(); }
    int Tris() const { return isModel ? model.totalTriangles : primitive.GetIndexCount()/3; }
};

// ============================================================================
// 全局状态
// ============================================================================
std::unique_ptr<Shader> g_shader;
std::vector<SceneObj> g_objects;

// 光源
std::vector<DirLightData> g_dirLights;
std::vector<PointLightData> g_pointLights;
std::vector<SpotLightData> g_spotLights;

// 全局材质参数
float g_globalShininess = 32.0f;
float g_ambientStrength = 0.1f;

// 轨道摄像机
struct {
    float yaw=-0.5f, pitch=0.45f, distance=12.0f;
    Vec3 target = Vec3(0, 0.5f, 0);
    float fov=45.0f, near=0.1f, far=100.0f;
    Vec3 GetEye() const {
        return target + Vec3(distance*std::cos(pitch)*std::sin(yaw),
                            distance*std::sin(pitch),
                            distance*std::cos(pitch)*std::cos(yaw));
    }
    Mat4 GetView() const { return Mat4::LookAt(GetEye(), target, Vec3(0,1,0)); }
    Mat4 GetProj(float a) const { return Mat4::Perspective(fov*0.0174533f, a, near, far); }
} g_cam;

bool g_showEditor=true, g_wireframe=false;
Vec4 g_bgColor(0.08f, 0.08f, 0.12f, 1);
bool g_mouseLDown=false; double g_lastMX=0, g_lastMY=0;

// ============================================================================
// Shader辅助：设置光源Uniform
// ============================================================================
void SetLightUniforms() {
    // 方向光
    int numDir = 0;
    for (auto& l : g_dirLights) {
        if (!l.enabled) continue;
        std::string base = "uDirLights[" + std::to_string(numDir) + "]";
        g_shader->SetVec3(base+".direction", l.direction);
        g_shader->SetVec3(base+".color", l.color);
        g_shader->SetFloat(base+".intensity", l.intensity);
        numDir++;
    }
    g_shader->SetInt("uNumDirLights", numDir);
    
    // 点光源
    int numPt = 0;
    for (auto& l : g_pointLights) {
        if (!l.enabled) continue;
        std::string base = "uPointLights[" + std::to_string(numPt) + "]";
        g_shader->SetVec3(base+".position", l.position);
        g_shader->SetVec3(base+".color", l.color);
        g_shader->SetFloat(base+".intensity", l.intensity);
        g_shader->SetFloat(base+".constant", l.constant);
        g_shader->SetFloat(base+".linear", l.linear);
        g_shader->SetFloat(base+".quadratic", l.quadratic);
        numPt++;
    }
    g_shader->SetInt("uNumPointLights", numPt);
    
    // 聚光灯
    int numSp = 0;
    for (auto& l : g_spotLights) {
        if (!l.enabled) continue;
        std::string base = "uSpotLights[" + std::to_string(numSp) + "]";
        g_shader->SetVec3(base+".position", l.position);
        g_shader->SetVec3(base+".direction", l.direction);
        g_shader->SetVec3(base+".color", l.color);
        g_shader->SetFloat(base+".intensity", l.intensity);
        g_shader->SetFloat(base+".constant", l.constant);
        g_shader->SetFloat(base+".linear", l.linear);
        g_shader->SetFloat(base+".quadratic", l.quadratic);
        g_shader->SetFloat(base+".cutOff", std::cos(l.cutOff * 0.0174533f));
        g_shader->SetFloat(base+".outerCutOff", std::cos(l.outerCutOff * 0.0174533f));
        numSp++;
    }
    g_shader->SetInt("uNumSpotLights", numSp);
}

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    g_shader = std::make_unique<Shader>();
    const char* vs[]={"data/shader/phong.vs","../data/shader/phong.vs","../../data/shader/phong.vs"};
    const char* fs[]={"data/shader/phong.fs","../data/shader/phong.fs","../../data/shader/phong.fs"};
    bool ok=false;
    for(int i=0;i<3;i++){if(g_shader->LoadFromFile(vs[i],fs[i])){ok=true;break;}}
    if(!ok){std::cerr<<"Failed to load phong shader!"<<std::endl;return false;}
    
    // ---- 创建光源 ----
    // 方向光（太阳光）
    {DirLightData d; d.name="Sun"; d.direction=Vec3(0.4f,0.7f,0.3f); d.color=Vec3(1,0.95f,0.9f); d.intensity=0.8f; g_dirLights.push_back(d);}
    
    // 点光源（红色灯和蓝色灯）
    {PointLightData p; p.name="Red Lamp"; p.position=Vec3(-3,2,2); p.color=Vec3(1,0.2f,0.1f); p.intensity=1.5f; p.linear=0.14f; p.quadratic=0.07f; g_pointLights.push_back(p);}
    {PointLightData p; p.name="Blue Lamp"; p.position=Vec3(3,2,-2); p.color=Vec3(0.1f,0.3f,1); p.intensity=1.5f; p.linear=0.14f; p.quadratic=0.07f; g_pointLights.push_back(p);}
    
    // 聚光灯（手电筒）
    {SpotLightData s; s.name="Flashlight"; s.position=Vec3(0,6,0); s.direction=Vec3(0,-1,0); s.color=Vec3(1,1,0.9f); s.intensity=2.0f; s.cutOff=15; s.outerCutOff=20; s.enabled=false; g_spotLights.push_back(s);}
    
    // ---- 创建场景物体 ----
    // 地面
    {SceneObj o; o.name="Ground"; o.primitive=Mesh3D::CreatePlane(12,12,12,12); o.useVtxColor=false; o.color=Vec4(0.3f,0.35f,0.3f,1); o.shininess=8; g_objects.push_back(std::move(o));}
    
    // 中间球体（高光测试）
    {SceneObj o; o.name="Shiny Sphere"; o.primitive=Mesh3D::CreateSphere(1.0f,32,24); o.pos=Vec3(0,1,0); o.useVtxColor=false; o.color=Vec4(0.9f,0.9f,0.95f,1); o.shininess=128; g_objects.push_back(std::move(o));}
    
    // 哑光立方体
    {SceneObj o; o.name="Matte Cube"; o.primitive=Mesh3D::CreateCube(1.2f); o.pos=Vec3(-3,0.6f,0); o.useVtxColor=false; o.color=Vec4(0.8f,0.3f,0.2f,1); o.shininess=8; o.autoRotSpd=0.3f; g_objects.push_back(std::move(o));}
    
    // 金属球
    {SceneObj o; o.name="Metal Sphere"; o.primitive=Mesh3D::CreateSphere(0.7f,24,16); o.pos=Vec3(3,0.7f,0); o.useVtxColor=false; o.color=Vec4(0.8f,0.7f,0.3f,1); o.shininess=256; g_objects.push_back(std::move(o));}
    
    // 彩色立方体
    {SceneObj o; o.name="Color Cube"; o.primitive=Mesh3D::CreateColorCube(0.8f); o.pos=Vec3(0,0.4f,3); o.autoRotSpd=0.5f; o.shininess=32; g_objects.push_back(std::move(o));}
    
    // 加载Mixamo角色（如果有）
    {SceneObj o; o.name="X Bot"; o.model=ModelLoader::LoadWithFallback("fbx/X Bot.fbx"); o.isModel=o.model.IsValid();
     if(!o.isModel){o.primitive=Mesh3D::CreateCube(1);} o.pos=Vec3(0,0,-3); o.scl=Vec3(0.02f,0.02f,0.02f); o.shininess=32; g_objects.push_back(std::move(o));}
    
    // 点光源位置可视化小球
    {SceneObj o; o.name="[Light] Red Lamp"; o.primitive=Mesh3D::CreateSphere(0.15f,12,8); o.pos=Vec3(-3,2,2); o.useVtxColor=false; o.color=Vec4(1,0.2f,0.1f,1); o.shininess=1; g_objects.push_back(std::move(o));}
    {SceneObj o; o.name="[Light] Blue Lamp"; o.primitive=Mesh3D::CreateSphere(0.15f,12,8); o.pos=Vec3(3,2,-2); o.useVtxColor=false; o.color=Vec4(0.1f,0.3f,1,1); o.shininess=1; g_objects.push_back(std::move(o));}
    
    std::cout<<"\n=== Chapter 21: Lighting System ==="<<std::endl;
    std::cout<<"Left-drag: Rotate | Scroll: Zoom | E: Editor"<<std::endl;
    std::cout<<"L: Toggle Lights | F: Flashlight | W: Wireframe"<<std::endl;
    return true;
}

// ============================================================================
// 更新
// ============================================================================
void Update(float dt) {
    Engine& eng=Engine::Instance(); GLFWwindow*w=eng.GetWindow();
    
    static bool eL=0; bool e=glfwGetKey(w,GLFW_KEY_E); if(e&&!eL)g_showEditor=!g_showEditor; eL=e;
    static bool wL=0; bool wk=glfwGetKey(w,GLFW_KEY_W); if(wk&&!wL)g_wireframe=!g_wireframe; wL=wk;
    
    // L键切换所有光源
    static bool lL=0; bool l=glfwGetKey(w,GLFW_KEY_L);
    if(l&&!lL){for(auto&d:g_dirLights)d.enabled=!d.enabled;for(auto&p:g_pointLights)p.enabled=!p.enabled;} lL=l;
    
    // F键切换聚光灯
    static bool fL=0; bool f=glfwGetKey(w,GLFW_KEY_F);
    if(f&&!fL){for(auto&s:g_spotLights)s.enabled=!s.enabled;} fL=f;
    
    // 鼠标左键旋转
    double mx,my; glfwGetCursorPos(w,&mx,&my);
    if(glfwGetMouseButton(w,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse){
        if(!g_mouseLDown){g_mouseLDown=true;g_lastMX=mx;g_lastMY=my;}
        g_cam.yaw+=(float)(mx-g_lastMX)*0.005f;
        g_cam.pitch-=(float)(my-g_lastMY)*0.005f;
        if(g_cam.pitch>1.5f)g_cam.pitch=1.5f;
        if(g_cam.pitch<-0.2f)g_cam.pitch=-0.2f;
        g_lastMX=mx;g_lastMY=my;
    }else g_mouseLDown=false;
    
    // 自动旋转
    for(auto&o:g_objects){if(o.autoRotSpd!=0){float*a=&o.rot.x+o.autoRotAxis;*a+=o.autoRotSpd*dt;}}
    
    // 聚光灯跟随摄像机
    if(!g_spotLights.empty()&&g_spotLights[0].enabled){
        Vec3 eye=g_cam.GetEye();
        g_spotLights[0].position=eye;
        Vec3 dir=g_cam.target-eye;
        float len=std::sqrt(dir.x*dir.x+dir.y*dir.y+dir.z*dir.z);
        if(len>0.001f)g_spotLights[0].direction=Vec3(dir.x/len,dir.y/len,dir.z/len);
    }
}

// ============================================================================
// 渲染
// ============================================================================
void Render() {
    Engine&eng=Engine::Instance();
    int sw=eng.GetWindowWidth(),sh=eng.GetWindowHeight();
    float aspect=(float)sw/(float)sh;
    
    glClearColor(g_bgColor.x,g_bgColor.y,g_bgColor.z,g_bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(g_wireframe)glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); else glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    
    g_shader->Bind();
    Mat4 view=g_cam.GetView(), proj=g_cam.GetProj(aspect);
    g_shader->SetMat4("uView",view.m);
    g_shader->SetMat4("uProjection",proj.m);
    g_shader->SetVec3("uViewPos",g_cam.GetEye());
    g_shader->SetFloat("uAmbientStrength",g_ambientStrength);
    
    SetLightUniforms();
    
    for(auto&o:g_objects){
        if(!o.visible)continue;
        Mat4 model=Mat4::Translate(o.pos.x,o.pos.y,o.pos.z)
                  *Mat4::RotateY(o.rot.y)*Mat4::RotateX(o.rot.x)*Mat4::RotateZ(o.rot.z)
                  *Mat4::Scale(o.scl.x,o.scl.y,o.scl.z);
        g_shader->SetMat4("uModel",model.m);
        
        // 法线矩阵 = Model矩阵左上3x3的逆转置
        Mat3 normalMat=model.GetRotationMatrix().Inverse().Transposed();
        g_shader->SetMat3("uNormalMatrix",normalMat.m);
        
        g_shader->SetVec4("uBaseColor",o.color);
        g_shader->SetBool("uUseVertexColor",o.useVtxColor);
        g_shader->SetFloat("uShininess",o.shininess);
        o.Draw();
    }
    
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui() {
    if(!g_showEditor)return;
    
    ImGui::Begin("Lighting Editor [E]",&g_showEditor,ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %.0f",Engine::Instance().GetFPS());
    
    if(ImGui::BeginTabBar("LightTabs")){
        // ---- Camera ----
        if(ImGui::BeginTabItem("Camera")){
            ImGui::SliderFloat("Yaw",&g_cam.yaw,-3.14f,3.14f);
            ImGui::SliderFloat("Pitch",&g_cam.pitch,-0.2f,1.5f);
            ImGui::SliderFloat("Distance",&g_cam.distance,2,30);
            ImGui::DragFloat3("Target",&g_cam.target.x,0.1f);
            ImGui::SliderFloat("FOV",&g_cam.fov,10,120);
            Vec3 eye=g_cam.GetEye();
            ImGui::Text("Eye: (%.1f, %.1f, %.1f)",eye.x,eye.y,eye.z);
            ImGui::EndTabItem();
        }
        
        // ---- Objects ----
        if(ImGui::BeginTabItem("Objects")){
            for(int i=0;i<(int)g_objects.size();i++){
                auto&o=g_objects[i]; ImGui::PushID(i);
                if(ImGui::TreeNode("##o","[%d] %s",i,o.name.c_str())){
                    ImGui::Checkbox("Visible",&o.visible);
                    ImGui::DragFloat3("Pos",&o.pos.x,0.05f);
                    Vec3 rd(o.rot.x*57.3f,o.rot.y*57.3f,o.rot.z*57.3f);
                    if(ImGui::DragFloat3("Rot",&rd.x,1))o.rot=Vec3(rd.x*0.01745f,rd.y*0.01745f,rd.z*0.01745f);
                    ImGui::DragFloat3("Scale",&o.scl.x,0.005f,0.001f,100);
                    ImGui::Checkbox("Vertex Color",&o.useVtxColor);
                    if(!o.useVtxColor)ImGui::ColorEdit4("Color",&o.color.x);
                    ImGui::SliderFloat("Shininess",&o.shininess,1,512);
                    ImGui::SliderFloat("Auto Rotate",&o.autoRotSpd,-3,3);
                    ImGui::Text("Verts: %d  Tris: %d",o.Verts(),o.Tris());
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        
        // ---- Lighting ----
        if(ImGui::BeginTabItem("Lighting")){
            ImGui::SliderFloat("Global Ambient",&g_ambientStrength,0,0.5f);
            ImGui::SliderFloat("Default Shininess",&g_globalShininess,1,512);
            
            ImGui::Separator(); ImGui::Text("=== Directional Lights ===");
            for(int i=0;i<(int)g_dirLights.size();i++){
                auto&l=g_dirLights[i]; ImGui::PushID(100+i);
                if(ImGui::TreeNode("##dl","%s",l.name.c_str())){
                    ImGui::Checkbox("Enabled",&l.enabled);
                    ImGui::DragFloat3("Direction",&l.direction.x,0.01f,-1,1);
                    ImGui::ColorEdit3("Color",&l.color.x);
                    ImGui::SliderFloat("Intensity",&l.intensity,0,3);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            
            ImGui::Separator(); ImGui::Text("=== Point Lights ===");
            for(int i=0;i<(int)g_pointLights.size();i++){
                auto&l=g_pointLights[i]; ImGui::PushID(200+i);
                if(ImGui::TreeNode("##pl","%s",l.name.c_str())){
                    ImGui::Checkbox("Enabled",&l.enabled);
                    ImGui::DragFloat3("Position",&l.position.x,0.1f);
                    ImGui::ColorEdit3("Color",&l.color.x);
                    ImGui::SliderFloat("Intensity",&l.intensity,0,5);
                    ImGui::SliderFloat("Linear",&l.linear,0.001f,1);
                    ImGui::SliderFloat("Quadratic",&l.quadratic,0.001f,2);
                    // 同步位置到可视化小球
                    for(auto&o:g_objects){
                        if(o.name=="[Light] "+l.name) o.pos=l.position;
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            
            ImGui::Separator(); ImGui::Text("=== Spot Lights ===");
            for(int i=0;i<(int)g_spotLights.size();i++){
                auto&l=g_spotLights[i]; ImGui::PushID(300+i);
                if(ImGui::TreeNode("##sl","%s [F]",l.name.c_str())){
                    ImGui::Checkbox("Enabled (F)",&l.enabled);
                    ImGui::DragFloat3("Position",&l.position.x,0.1f);
                    ImGui::DragFloat3("Direction",&l.direction.x,0.01f,-1,1);
                    ImGui::ColorEdit3("Color",&l.color.x);
                    ImGui::SliderFloat("Intensity",&l.intensity,0,5);
                    ImGui::SliderFloat("Inner Cone",&l.cutOff,5,45);
                    ImGui::SliderFloat("Outer Cone",&l.outerCutOff,l.cutOff,60);
                    ImGui::SliderFloat("Linear",&l.linear,0.001f,1);
                    ImGui::SliderFloat("Quadratic",&l.quadratic,0.001f,2);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            
            ImGui::Separator();
            if(ImGui::Button("Daylight")){g_dirLights[0].direction=Vec3(0.4f,0.7f,0.3f);g_dirLights[0].color=Vec3(1,0.95f,0.9f);g_dirLights[0].intensity=0.8f;g_ambientStrength=0.15f;g_bgColor=Vec4(0.4f,0.5f,0.7f,1);}
            ImGui::SameLine();
            if(ImGui::Button("Sunset")){g_dirLights[0].direction=Vec3(0.9f,0.15f,0.1f);g_dirLights[0].color=Vec3(1,0.5f,0.2f);g_dirLights[0].intensity=0.6f;g_ambientStrength=0.08f;g_bgColor=Vec4(0.3f,0.15f,0.1f,1);}
            ImGui::SameLine();
            if(ImGui::Button("Night")){g_dirLights[0].direction=Vec3(-0.2f,0.5f,-0.3f);g_dirLights[0].color=Vec3(0.3f,0.35f,0.5f);g_dirLights[0].intensity=0.3f;g_ambientStrength=0.03f;g_bgColor=Vec4(0.02f,0.02f,0.05f,1);}
            
            ImGui::EndTabItem();
        }
        
        // ---- Render ----
        if(ImGui::BeginTabItem("Render")){
            ImGui::ColorEdit4("Background",&g_bgColor.x);
            ImGui::Checkbox("Wireframe (W)",&g_wireframe);
            ImGui::Separator();
            ImGui::Text("Renderer: %s",glGetString(GL_RENDERER));
            ImGui::Text("OpenGL: %s",glGetString(GL_VERSION));
            int tv=0,tt=0;for(auto&o:g_objects){tv+=o.Verts();tt+=o.Tris();}
            ImGui::Text("Scene: %d verts, %d tris",tv,tt);
            ImGui::Text("Lights: %dD + %dP + %dS",(int)g_dirLights.size(),(int)g_pointLights.size(),(int)g_spotLights.size());
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    ImGui::End();
}

// ============================================================================
// 滚轮 & 清理 & main
// ============================================================================
void ScrollCB(GLFWwindow*,double,double y){
    g_cam.distance-=(float)y*0.5f;
    if(g_cam.distance<1)g_cam.distance=1;
    if(g_cam.distance>30)g_cam.distance=30;
}

void Cleanup(){
    for(auto&o:g_objects){o.model.Destroy();o.primitive.Destroy();}
    g_objects.clear(); g_shader.reset();
}

int main(){
    std::cout<<"=== Example 21: Lighting System ==="<<std::endl;
    Engine&engine=Engine::Instance();
    EngineConfig cfg;cfg.windowWidth=1280;cfg.windowHeight=720;
    cfg.windowTitle="Mini Engine - Lighting System";
    if(!engine.Initialize(cfg))return -1;
    glfwSetScrollCallback(engine.GetWindow(),ScrollCB);
    if(!Initialize()){engine.Shutdown();return -1;}
    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();
    Cleanup();engine.Shutdown();
    return 0;
}
