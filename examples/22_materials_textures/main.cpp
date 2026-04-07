/**
 * 第22章：材质与纹理 (Materials & Textures)
 *
 * 将纹理贴图应用到3D物体上：漫反射贴图、法线贴图、粗糙度贴图。
 * 不同材质在光照下呈现不同的视觉效果。
 *
 * 操作：
 * - 鼠标左键拖动：旋转视角
 * - 滚轮：缩放
 * - E：编辑面板
 * - 1~5：切换材质（砖墙/木板/金属/金属板/木地板）
 * - N：切换法线贴图开关
 * - L：切换光照
 * - W：线框模式
 */

#include "engine/Engine.h"
#include "engine/Shader.h"
#include "engine3d/Mesh3D.h"
#include "engine3d/Texture3D.h"
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
// 全局状态
// ============================================================================
std::unique_ptr<Shader> g_shader;

// 材质列表
std::vector<Material3D> g_materials;
int g_currentMaterial = 0;

// 场景物体
Mesh3D g_cube, g_plane, g_sphere, g_wall;

struct ObjState {
    Vec3 pos, rot, scl; std::string name; bool visible;
    float tiling;
    int materialIdx; // -1=无材质（用纯色）
};
std::vector<ObjState> g_objStates;

// 轨道摄像机
struct {
    float yaw=-0.6f, pitch=0.4f, distance=10.0f;
    Vec3 target=Vec3(0,1,0);
    float fov=45, near=0.1f, far=100;
    Vec3 GetEye() const { return target+Vec3(distance*std::cos(pitch)*std::sin(yaw),distance*std::sin(pitch),distance*std::cos(pitch)*std::cos(yaw)); }
    Mat4 GetView() const { return Mat4::LookAt(GetEye(),target,Vec3(0,1,0)); }
    Mat4 GetProj(float a) const { return Mat4::Perspective(fov*0.0174533f,a,near,far); }
} g_cam;

// 光源
struct { Vec3 dir=Vec3(0.4f,0.7f,0.3f); Vec3 color=Vec3(1,0.95f,0.9f); float intensity=0.8f; bool on=true; } g_dirLight;
struct { Vec3 pos=Vec3(2,3,2); Vec3 color=Vec3(1,0.8f,0.6f); float intensity=1.5f; float linear=0.09f,quadratic=0.032f; bool on=true; } g_pointLight;

float g_ambientStrength = 0.15f;
bool g_showEditor=true, g_wireframe=false, g_useNormalMap=true;
Vec4 g_bgColor(0.1f,0.1f,0.15f,1);
bool g_mouseLDown=false; double g_lastMX=0,g_lastMY=0;

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // 加载Shader
    g_shader = std::make_unique<Shader>();
    const char* vs[]={"data/shader/material.vs","../data/shader/material.vs","../../data/shader/material.vs"};
    const char* fs[]={"data/shader/material.fs","../data/shader/material.fs","../../data/shader/material.fs"};
    bool ok=false;
    for(int i=0;i<3;i++){if(g_shader->LoadFromFile(vs[i],fs[i])){ok=true;break;}}
    if(!ok){std::cerr<<"Failed to load material shader!"<<std::endl;return false;}
    
    // 设置纹理单元编号（只需设一次）
    g_shader->Bind();
    g_shader->SetInt("uDiffuseMap", 0);
    g_shader->SetInt("uNormalMap", 1);
    g_shader->SetInt("uRoughnessMap", 2);
    
    // 加载材质
    {Material3D m; m.LoadFromDirectory("bricks","Bricks"); m.tiling=1; g_materials.push_back(std::move(m));}
    {Material3D m; m.LoadFromDirectory("wood","Wood Planks"); m.tiling=2; g_materials.push_back(std::move(m));}
    {Material3D m; m.LoadFromDirectory("metal","Metal"); m.tiling=1; g_materials.push_back(std::move(m));}
    {Material3D m; m.LoadFromDirectory("metalplates","Metal Plates"); m.tiling=1; g_materials.push_back(std::move(m));}
    {Material3D m; m.LoadFromDirectory("woodfloor","Wood Floor"); m.tiling=3; g_materials.push_back(std::move(m));}
    
    // 创建Mesh
    g_cube = Mesh3D::CreateCube(2.0f);
    g_sphere = Mesh3D::CreateSphere(1.2f, 32, 24);
    g_plane = Mesh3D::CreatePlane(12, 12, 1, 1);
    g_wall = Mesh3D::CreatePlane(6, 4, 1, 1);
    
    // 场景物体配置
    g_objStates.push_back({Vec3(0,0,0),Vec3(0,0,0),Vec3(1,1,1),"Ground",true,3.0f,4}); // 木地板
    g_objStates.push_back({Vec3(-2,1,0),Vec3(0,0,0),Vec3(1,1,1),"Brick Cube",true,1.0f,0}); // 砖墙立方体
    g_objStates.push_back({Vec3(2,1.2f,0),Vec3(0,0,0),Vec3(1,1,1),"Metal Sphere",true,1.0f,2}); // 金属球
    g_objStates.push_back({Vec3(0,2,-4),Vec3(-1.5708f,0,0),Vec3(1,1,1),"Back Wall",true,2.0f,3}); // 背景墙
    g_objStates.push_back({Vec3(0,1.2f,3),Vec3(0,0,0),Vec3(1,1,1),"Wood Sphere",true,1.0f,1}); // 木球
    
    std::cout<<"\n=== Chapter 22: Materials & Textures ==="<<std::endl;
    std::cout<<"Left-drag: Rotate | Scroll: Zoom | E: Editor"<<std::endl;
    std::cout<<"1-5: Material | N: Normal Map | L: Lighting | W: Wireframe"<<std::endl;
    return true;
}

// ============================================================================
// 更新
// ============================================================================
void Update(float dt) {
    Engine&eng=Engine::Instance(); GLFWwindow*w=eng.GetWindow();
    
    static bool eL=0; bool e=glfwGetKey(w,GLFW_KEY_E); if(e&&!eL)g_showEditor=!g_showEditor; eL=e;
    static bool wL=0; bool wk=glfwGetKey(w,GLFW_KEY_W); if(wk&&!wL)g_wireframe=!g_wireframe; wL=wk;
    static bool lL=0; bool l=glfwGetKey(w,GLFW_KEY_L); if(l&&!lL){g_dirLight.on=!g_dirLight.on;g_pointLight.on=!g_pointLight.on;} lL=l;
    static bool nL=0; bool n=glfwGetKey(w,GLFW_KEY_N); if(n&&!nL)g_useNormalMap=!g_useNormalMap; nL=n;
    
    // 数字键切换当前选中材质
    for(int i=0;i<5;i++){
        if(glfwGetKey(w,GLFW_KEY_1+i))g_currentMaterial=i;
    }
    
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
}

// ============================================================================
// 渲染
// ============================================================================
void RenderObj(int idx, Mesh3D& mesh) {
    auto& s = g_objStates[idx];
    if(!s.visible) return;
    
    Mat4 model = Mat4::Translate(s.pos.x,s.pos.y,s.pos.z)
               * Mat4::RotateY(s.rot.y)*Mat4::RotateX(s.rot.x)*Mat4::RotateZ(s.rot.z)
               * Mat4::Scale(s.scl.x,s.scl.y,s.scl.z);
    g_shader->SetMat4("uModel", model.m);
    Mat3 nm = model.GetRotationMatrix().Inverse().Transposed();
    g_shader->SetMat3("uNormalMatrix", nm.m);
    
    // 绑定材质
    bool hasMat = s.materialIdx >= 0 && s.materialIdx < (int)g_materials.size();
    if (hasMat) {
        auto& mat = g_materials[s.materialIdx];
        mat.Bind();
        g_shader->SetBool("uUseDiffuseMap", mat.hasDiffuse);
        g_shader->SetBool("uUseNormalMap", mat.hasNormal && g_useNormalMap);
        g_shader->SetBool("uUseRoughnessMap", mat.hasRoughness);
        g_shader->SetFloat("uShininess", mat.shininess);
        g_shader->SetFloat("uTexTiling", s.tiling);
        g_shader->SetBool("uUseVertexColor", false);
        g_shader->SetVec4("uBaseColor", Vec4(1,1,1,1));
    } else {
        g_shader->SetBool("uUseDiffuseMap", false);
        g_shader->SetBool("uUseNormalMap", false);
        g_shader->SetBool("uUseRoughnessMap", false);
        g_shader->SetBool("uUseVertexColor", false);
        g_shader->SetVec4("uBaseColor", Vec4(0.5f,0.5f,0.5f,1));
        g_shader->SetFloat("uShininess", 32);
        g_shader->SetFloat("uTexTiling", 1);
    }
    
    mesh.Draw();
}

void Render() {
    Engine&eng=Engine::Instance();
    int sw=eng.GetWindowWidth(),sh=eng.GetWindowHeight();
    float aspect=(float)sw/(float)sh;
    
    glClearColor(g_bgColor.x,g_bgColor.y,g_bgColor.z,g_bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(g_wireframe)glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);else glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    
    g_shader->Bind();
    g_shader->SetMat4("uView",g_cam.GetView().m);
    g_shader->SetMat4("uProjection",g_cam.GetProj(aspect).m);
    g_shader->SetVec3("uViewPos",g_cam.GetEye());
    g_shader->SetFloat("uAmbientStrength",g_ambientStrength);
    
    // 方向光
    g_shader->SetInt("uNumDirLights", g_dirLight.on?1:0);
    if(g_dirLight.on){
        g_shader->SetVec3("uDirLights[0].direction",g_dirLight.dir);
        g_shader->SetVec3("uDirLights[0].color",g_dirLight.color);
        g_shader->SetFloat("uDirLights[0].intensity",g_dirLight.intensity);
    }
    // 点光源
    g_shader->SetInt("uNumPointLights",g_pointLight.on?1:0);
    if(g_pointLight.on){
        g_shader->SetVec3("uPointLights[0].position",g_pointLight.pos);
        g_shader->SetVec3("uPointLights[0].color",g_pointLight.color);
        g_shader->SetFloat("uPointLights[0].intensity",g_pointLight.intensity);
        g_shader->SetFloat("uPointLights[0].constant",1.0f);
        g_shader->SetFloat("uPointLights[0].linear",g_pointLight.linear);
        g_shader->SetFloat("uPointLights[0].quadratic",g_pointLight.quadratic);
    }
    
    // 渲染场景
    RenderObj(0, g_plane);   // 地面
    RenderObj(1, g_cube);    // 砖墙立方体
    RenderObj(2, g_sphere);  // 金属球
    RenderObj(3, g_wall);    // 背景墙
    RenderObj(4, g_sphere);  // 木球
    
    // 点光源位置指示球
    {
        Mat4 m=Mat4::Translate(g_pointLight.pos.x,g_pointLight.pos.y,g_pointLight.pos.z)*Mat4::Scale(0.1f,0.1f,0.1f);
        g_shader->SetMat4("uModel",m.m);
        Mat3 nm; nm.m[0]=nm.m[4]=nm.m[8]=1; nm.m[1]=nm.m[2]=nm.m[3]=nm.m[5]=nm.m[6]=nm.m[7]=0;
        g_shader->SetMat3("uNormalMatrix",nm.m);
        g_shader->SetBool("uUseDiffuseMap",false); g_shader->SetBool("uUseNormalMap",false); g_shader->SetBool("uUseRoughnessMap",false);
        g_shader->SetBool("uUseVertexColor",false);
        g_shader->SetVec4("uBaseColor",Vec4(g_pointLight.color.x,g_pointLight.color.y,g_pointLight.color.z,1));
        g_shader->SetFloat("uTexTiling",1);
        Mesh3D lightSphere = Mesh3D::CreateSphere(1,12,8);
        lightSphere.Draw(); lightSphere.Destroy();
    }
    
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui() {
    if(!g_showEditor)return;
    
    ImGui::Begin("Material Editor [E]",&g_showEditor,ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("FPS: %.0f",Engine::Instance().GetFPS());
    
    if(ImGui::BeginTabBar("MatTabs")){
        if(ImGui::BeginTabItem("Camera")){
            ImGui::SliderFloat("Yaw",&g_cam.yaw,-3.14f,3.14f);
            ImGui::SliderFloat("Pitch",&g_cam.pitch,-0.2f,1.5f);
            ImGui::SliderFloat("Distance",&g_cam.distance,2,30);
            ImGui::DragFloat3("Target",&g_cam.target.x,0.1f);
            ImGui::SliderFloat("FOV",&g_cam.fov,10,120);
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Objects")){
            for(int i=0;i<(int)g_objStates.size();i++){
                auto&s=g_objStates[i]; ImGui::PushID(i);
                if(ImGui::TreeNode("##o","[%d] %s",i,s.name.c_str())){
                    ImGui::Checkbox("Visible",&s.visible);
                    ImGui::DragFloat3("Position",&s.pos.x,0.05f);
                    Vec3 rd(s.rot.x*57.3f,s.rot.y*57.3f,s.rot.z*57.3f);
                    if(ImGui::DragFloat3("Rotation",&rd.x,1))s.rot=Vec3(rd.x*0.01745f,rd.y*0.01745f,rd.z*0.01745f);
                    ImGui::DragFloat3("Scale",&s.scl.x,0.05f,0.1f,10);
                    ImGui::SliderFloat("Tex Tiling",&s.tiling,0.1f,10);
                    // 材质选择
                    const char* matNames[6]={"None","Bricks","Wood","Metal","MetalPlates","WoodFloor"};
                    int sel=s.materialIdx+1;
                    if(ImGui::Combo("Material",&sel,matNames,6))s.materialIdx=sel-1;
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Materials")){
            ImGui::Checkbox("Normal Mapping (N)",&g_useNormalMap);
            ImGui::Separator();
            for(int i=0;i<(int)g_materials.size();i++){
                auto&m=g_materials[i]; ImGui::PushID(100+i);
                if(ImGui::TreeNode("##m","[%d] %s",i,m.name.c_str())){
                    ImGui::Text("Diffuse: %s (%dx%d)",m.hasDiffuse?"Yes":"No",m.diffuseMap.GetWidth(),m.diffuseMap.GetHeight());
                    ImGui::Text("Normal: %s",m.hasNormal?"Yes":"No");
                    ImGui::Text("Roughness: %s",m.hasRoughness?"Yes":"No");
                    ImGui::SliderFloat("Shininess",&m.shininess,1,512);
                    ImGui::SliderFloat("Tiling",&m.tiling,0.1f,10);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Lighting")){
            ImGui::SliderFloat("Ambient",&g_ambientStrength,0,0.5f);
            ImGui::Separator();
            ImGui::Text("=== Directional ===");
            ImGui::Checkbox("Dir Light",&g_dirLight.on);
            ImGui::DragFloat3("Dir Direction",&g_dirLight.dir.x,0.01f,-1,1);
            ImGui::ColorEdit3("Dir Color",&g_dirLight.color.x);
            ImGui::SliderFloat("Dir Intensity",&g_dirLight.intensity,0,3);
            ImGui::Separator();
            ImGui::Text("=== Point Light ===");
            ImGui::Checkbox("Point Light",&g_pointLight.on);
            ImGui::DragFloat3("Pt Position",&g_pointLight.pos.x,0.1f);
            ImGui::ColorEdit3("Pt Color",&g_pointLight.color.x);
            ImGui::SliderFloat("Pt Intensity",&g_pointLight.intensity,0,5);
            ImGui::SliderFloat("Pt Linear",&g_pointLight.linear,0.001f,1);
            ImGui::SliderFloat("Pt Quadratic",&g_pointLight.quadratic,0.001f,2);
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Render")){
            ImGui::ColorEdit4("Background",&g_bgColor.x);
            ImGui::Checkbox("Wireframe (W)",&g_wireframe);
            ImGui::Separator();
            ImGui::Text("Renderer: %s",glGetString(GL_RENDERER));
            ImGui::Text("Materials loaded: %d",(int)g_materials.size());
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
    g_cube.Destroy();g_sphere.Destroy();g_plane.Destroy();g_wall.Destroy();
    for(auto&m:g_materials)m.Destroy();
    g_materials.clear(); g_shader.reset();
}

int main(){
    std::cout<<"=== Example 22: Materials & Textures ==="<<std::endl;
    Engine&engine=Engine::Instance();
    EngineConfig cfg;cfg.windowWidth=1280;cfg.windowHeight=720;
    cfg.windowTitle="Mini Engine - Materials & Textures";
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
