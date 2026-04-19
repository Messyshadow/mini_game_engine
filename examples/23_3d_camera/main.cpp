/**
 * 第23章：3D摄像机系统 (3D Camera System)
 *
 * 三种摄像机模式：轨道、第一人称、第三人称
 * 使用第22章的材质纹理 + 第21章的Phong光照
 *
 * 操作：
 * - Tab：切换摄像机模式（Orbit→FPS→TPS→Orbit）
 * - Orbit模式：左键拖动旋转，滚轮缩放
 * - FPS模式：WASD移动，鼠标自由看，Shift加速，Space上升，Ctrl下降
 * - TPS模式：左键拖动旋转视角，WASD移动角色，滚轮调距离
 * - E：编辑面板 | L：光照开关 | W：线框 | G：地面网格
 */

#include "engine/Engine.h"
#include "engine/Shader.h"
#include "engine3d/Mesh3D.h"
#include "engine3d/ModelLoader.h"
#include "engine3d/Texture3D.h"
#include "engine3d/Camera3D.h"
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
// 全局
// ============================================================================
std::unique_ptr<Shader> g_matShader;  // 材质shader
std::unique_ptr<Shader> g_basicShader; // 无纹理shader（光照指示球等）
Camera3D g_cam;

// 材质
std::vector<Material3D> g_materials;

// 场景物体
struct SceneObj {
    Mesh3D mesh; Model3D model; bool isModel=false;
    Vec3 pos, rot=Vec3(0,0,0), scl=Vec3(1,1,1);
    int matIdx=-1; float tiling=1; float shininess=32;
    bool visible=true; std::string name;
    void Draw()const{if(isModel&&model.IsValid())model.Draw();else mesh.Draw();}
};
std::vector<SceneObj> g_objects;

// TPS角色（简单立方体代表）
Vec3 g_playerPos(0, 0.5f, 0);
float g_playerYaw = 0;
float g_playerSpeed = 5.0f;

// 光照
struct{Vec3 dir=Vec3(0.4f,0.7f,0.3f);Vec3 color=Vec3(1,0.95f,0.9f);float intensity=0.8f;bool on=true;}g_dirLight;
struct{Vec3 pos=Vec3(3,3,3);Vec3 color=Vec3(1,0.8f,0.6f);float intensity=1.5f;float lin=0.09f,quad=0.032f;bool on=true;}g_ptLight;

float g_ambient=0.15f;
bool g_showEditor=true, g_wireframe=false, g_showGrid=true;
Vec4 g_bgColor(0.1f,0.12f,0.18f,1);
bool g_mouseLDown=false; double g_lastMX=0,g_lastMY=0;
bool g_fpsCursorLocked=false;

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    // 加载材质Shader
    g_matShader=std::make_unique<Shader>();
    const char*vs[]={"data/shader/material.vs","../data/shader/material.vs","../../data/shader/material.vs"};
    const char*fs[]={"data/shader/material.fs","../data/shader/material.fs","../../data/shader/material.fs"};
    bool ok=false;for(int i=0;i<3;i++){if(g_matShader->LoadFromFile(vs[i],fs[i])){ok=true;break;}}
    if(!ok){std::cerr<<"Failed to load material shader!"<<std::endl;return false;}
    g_matShader->Bind();
    g_matShader->SetInt("uDiffuseMap",0);
    g_matShader->SetInt("uNormalMap",1);
    g_matShader->SetInt("uRoughnessMap",2);
    
    // 加载phong shader（无纹理物体用）
    g_basicShader=std::make_unique<Shader>();
    const char*bvs[]={"data/shader/phong.vs","../data/shader/phong.vs","../../data/shader/phong.vs"};
    const char*bfs[]={"data/shader/phong.fs","../data/shader/phong.fs","../../data/shader/phong.fs"};
    ok=false;for(int i=0;i<3;i++){if(g_basicShader->LoadFromFile(bvs[i],bfs[i])){ok=true;break;}}
    
    // 加载材质
    {Material3D m;m.LoadFromDirectory("bricks","Bricks");g_materials.push_back(std::move(m));}
    {Material3D m;m.LoadFromDirectory("woodfloor","Wood Floor");m.tiling=3;g_materials.push_back(std::move(m));}
    {Material3D m;m.LoadFromDirectory("metal","Metal");g_materials.push_back(std::move(m));}
    {Material3D m;m.LoadFromDirectory("metalplates","Metal Plates");g_materials.push_back(std::move(m));}
    {Material3D m;m.LoadFromDirectory("wood","Wood Planks");g_materials.push_back(std::move(m));}
    
    // 场景物体
    // 地面
    {SceneObj o;o.name="Ground";o.mesh=Mesh3D::CreatePlane(20,20,1,1);o.matIdx=1;o.tiling=5;o.shininess=8;g_objects.push_back(std::move(o));}
    // 砖墙
    {SceneObj o;o.name="Brick Wall";o.mesh=Mesh3D::CreateCube(2);o.pos=Vec3(-4,1,0);o.matIdx=0;o.shininess=16;g_objects.push_back(std::move(o));}
    // 金属球
    {SceneObj o;o.name="Metal Sphere";o.mesh=Mesh3D::CreateSphere(1,32,24);o.pos=Vec3(4,1,0);o.matIdx=2;o.shininess=128;g_objects.push_back(std::move(o));}
    // 金属板柱子
    {SceneObj o;o.name="Metal Pillar";o.mesh=Mesh3D::CreateCube(1);o.pos=Vec3(0,1.5f,-5);o.scl=Vec3(1,3,1);o.matIdx=3;o.shininess=64;g_objects.push_back(std::move(o));}
    // 木板立方体
    {SceneObj o;o.name="Wood Crate";o.mesh=Mesh3D::CreateCube(1.2f);o.pos=Vec3(2,0.6f,3);o.matIdx=4;o.shininess=16;g_objects.push_back(std::move(o));}
    // X Bot角色（TPS跟随目标的视觉参考）
    {SceneObj o;o.name="X Bot";o.model=ModelLoader::LoadWithFallback("fbx/X Bot.fbx");o.isModel=o.model.IsValid();
     if(!o.isModel){o.mesh=Mesh3D::CreateCube(0.5f);}o.pos=Vec3(0,0,0);o.scl=Vec3(0.02f,0.02f,0.02f);o.shininess=32;g_objects.push_back(std::move(o));}
    
    // 摄像机初始化
    g_cam.mode=CameraMode::Orbit;
    g_cam.orbitDistance=12;
    g_cam.pitch=25;
    g_cam.yaw=-30;
    
    std::cout<<"\n=== Chapter 23: 3D Camera System ==="<<std::endl;
    std::cout<<"Tab: Switch Mode | E: Editor | G: Grid"<<std::endl;
    std::cout<<"Orbit: LMB+drag rotate, Scroll zoom"<<std::endl;
    std::cout<<"FPS: WASD move, Mouse look, Shift sprint"<<std::endl;
    std::cout<<"TPS: WASD move player, LMB+drag orbit around player"<<std::endl;
    return true;
}

// ============================================================================
// 更新
// ============================================================================
void Update(float dt) {
    Engine&eng=Engine::Instance();GLFWwindow*w=eng.GetWindow();
    if(dt>0.1f)dt=0.1f;
    
    // E/W/L/G键
    static bool eL=0;bool e=glfwGetKey(w,GLFW_KEY_E);if(e&&!eL)g_showEditor=!g_showEditor;eL=e;
    static bool wL=0;bool wk=glfwGetKey(w,GLFW_KEY_W)&&g_cam.mode!=CameraMode::FPS;if(wk&&!wL)g_wireframe=!g_wireframe;wL=wk;
    static bool lL=0;bool l=glfwGetKey(w,GLFW_KEY_L);if(l&&!lL){g_dirLight.on=!g_dirLight.on;g_ptLight.on=!g_ptLight.on;}lL=l;
    static bool gL=0;bool g=glfwGetKey(w,GLFW_KEY_G)&&g_cam.mode!=CameraMode::FPS;if(g&&!gL)g_showGrid=!g_showGrid;gL=g;
    
    // Tab切换模式
    static bool tabL=0;bool tab=glfwGetKey(w,GLFW_KEY_TAB);
    if(tab&&!tabL){
        if(g_cam.mode==CameraMode::Orbit)g_cam.SwitchMode(CameraMode::FPS);
        else if(g_cam.mode==CameraMode::FPS)g_cam.SwitchMode(CameraMode::TPS);
        else g_cam.SwitchMode(CameraMode::Orbit);
        // FPS模式锁定鼠标
        if(g_cam.mode==CameraMode::FPS){
            glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED);g_fpsCursorLocked=true;
        }else{
            glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL);g_fpsCursorLocked=false;
        }
    }tabL=tab;
    
    double mx,my;glfwGetCursorPos(w,&mx,&my);
    
    if(g_cam.mode==CameraMode::FPS){
        // FPS：鼠标自由控制视角
        static double prevMX=mx,prevMY=my;
        if(g_fpsCursorLocked){
            g_cam.ProcessMouseMove((float)(mx-prevMX),(float)(my-prevMY));
        }
        prevMX=mx;prevMY=my;
        
        // WASD移动
        float fwd=0,rt=0,up=0;
        if(glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS)fwd=1;
        if(glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS)fwd=-1;
        if(glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS)rt=-1;
        if(glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS)rt=1;
        if(glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS)up=1;
        if(glfwGetKey(w,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS)up=-1;
        bool sprint=glfwGetKey(w,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS;
        g_cam.ProcessFPSMovement(fwd,rt,up,dt,sprint);
        
    }else if(g_cam.mode==CameraMode::TPS){
        // TPS：左键拖动旋转视角
        if(glfwGetMouseButton(w,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS&&!ImGui::GetIO().WantCaptureMouse){
            if(!g_mouseLDown){g_mouseLDown=true;g_lastMX=mx;g_lastMY=my;}
            g_cam.ProcessMouseMove((float)(mx-g_lastMX),(float)(my-g_lastMY));
            g_lastMX=mx;g_lastMY=my;
        }else g_mouseLDown=false;
        
        // WASD移动角色
        float fwd=0,rt=0;
        if(glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS)fwd=1;
        if(glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS)fwd=-1;
        if(glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS)rt=-1;
        if(glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS)rt=1;
        if(fwd!=0||rt!=0){
            float camYawRad=g_cam.yaw*0.0174533f;
            Vec3 moveDir(
                std::sin(camYawRad)*fwd + std::cos(camYawRad)*rt,
                0,
                std::cos(camYawRad)*fwd - std::sin(camYawRad)*rt);
            float len=std::sqrt(moveDir.x*moveDir.x+moveDir.z*moveDir.z);
            if(len>0.001f){moveDir=moveDir*(1.0f/len);}
            g_playerPos=g_playerPos+moveDir*(g_playerSpeed*dt);
            g_playerYaw=std::atan2(moveDir.x,moveDir.z);
        }
        g_cam.tpsTarget=g_playerPos;
        g_cam.UpdateTPS(dt);
        // 同步角色模型位置
        if(g_objects.size()>5){g_objects[5].pos=g_playerPos;g_objects[5].rot.y=g_playerYaw;}
        
    }else{
        // Orbit：左键拖动
        if(glfwGetMouseButton(w,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS&&!ImGui::GetIO().WantCaptureMouse){
            if(!g_mouseLDown){g_mouseLDown=true;g_lastMX=mx;g_lastMY=my;}
            g_cam.ProcessMouseMove((float)(mx-g_lastMX),(float)(my-g_lastMY));
            g_lastMX=mx;g_lastMY=my;
        }else g_mouseLDown=false;
    }
}

// ============================================================================
// 渲染辅助
// ============================================================================
void SetLightUniforms(Shader*s){
    s->SetFloat("uAmbientStrength",g_ambient);
    s->SetInt("uNumDirLights",g_dirLight.on?1:0);
    if(g_dirLight.on){
        s->SetVec3("uDirLights[0].direction",g_dirLight.dir);
        s->SetVec3("uDirLights[0].color",g_dirLight.color);
        s->SetFloat("uDirLights[0].intensity",g_dirLight.intensity);
    }
    s->SetInt("uNumPointLights",g_ptLight.on?1:0);
    if(g_ptLight.on){
        s->SetVec3("uPointLights[0].position",g_ptLight.pos);
        s->SetVec3("uPointLights[0].color",g_ptLight.color);
        s->SetFloat("uPointLights[0].intensity",g_ptLight.intensity);
        s->SetFloat("uPointLights[0].constant",1.0f);
        s->SetFloat("uPointLights[0].linear",g_ptLight.lin);
        s->SetFloat("uPointLights[0].quadratic",g_ptLight.quad);
    }
}

void RenderObj(SceneObj&o,Shader*s){
    if(!o.visible)return;
    Mat4 model=Mat4::Translate(o.pos.x,o.pos.y,o.pos.z)
              *Mat4::RotateY(o.rot.y)*Mat4::RotateX(o.rot.x)*Mat4::RotateZ(o.rot.z)
              *Mat4::Scale(o.scl.x,o.scl.y,o.scl.z);
    s->SetMat4("uModel",model.m);
    Mat3 nm=model.GetRotationMatrix().Inverse().Transposed();
    s->SetMat3("uNormalMatrix",nm.m);
    
    bool hasMat=o.matIdx>=0&&o.matIdx<(int)g_materials.size();
    if(hasMat){
        auto&mat=g_materials[o.matIdx];
        mat.Bind();
        s->SetBool("uUseDiffuseMap",mat.hasDiffuse);
        s->SetBool("uUseNormalMap",mat.hasNormal);
        s->SetBool("uUseRoughnessMap",mat.hasRoughness);
        s->SetFloat("uTexTiling",o.tiling);
    }else{
        s->SetBool("uUseDiffuseMap",false);
        s->SetBool("uUseNormalMap",false);
        s->SetBool("uUseRoughnessMap",false);
        s->SetFloat("uTexTiling",1);
    }
    s->SetBool("uUseVertexColor",!hasMat&&o.matIdx<0);
    s->SetVec4("uBaseColor",Vec4(0.7f,0.7f,0.7f,1));
    s->SetFloat("uShininess",o.shininess);
    o.Draw();
}

// ============================================================================
// 渲染
// ============================================================================
void Render(){
    Engine&eng=Engine::Instance();
    int sw=eng.GetWindowWidth(),sh=eng.GetWindowHeight();
    float aspect=(float)sw/(float)sh;
    
    glClearColor(g_bgColor.x,g_bgColor.y,g_bgColor.z,g_bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(g_wireframe)glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);else glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    
    Mat4 view=g_cam.GetViewMatrix();
    Mat4 proj=g_cam.GetProjectionMatrix(aspect);
    Vec3 eye=g_cam.GetPosition();
    
    // 用材质shader渲染场景
    g_matShader->Bind();
    g_matShader->SetMat4("uView",view.m);
    g_matShader->SetMat4("uProjection",proj.m);
    g_matShader->SetVec3("uViewPos",eye);
    SetLightUniforms(g_matShader.get());
    
    for(auto&o:g_objects)RenderObj(o,g_matShader.get());
    
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui(){
    // HUD信息（始终显示）
    ImGui::SetNextWindowPos(ImVec2(10,10));
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("##HUD",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove);
    const char*modeNames[]={"Orbit","FPS","TPS"};
    ImGui::Text("Ch23: 3D Camera System");
    ImGui::Text("Mode: %s",modeNames[(int)g_cam.mode]);
    Vec3 eye=g_cam.GetPosition();
    ImGui::Text("Eye: (%.1f, %.1f, %.1f)",eye.x,eye.y,eye.z);
    ImGui::Text("Yaw:%.1f  Pitch:%.1f",g_cam.yaw,g_cam.pitch);
    ImGui::Text("FPS: %.0f",Engine::Instance().GetFPS());
    ImGui::Separator();
    ImGui::Text("[Tab] Switch Mode  [E] Editor");
    if(g_cam.mode==CameraMode::FPS)ImGui::Text("WASD+Mouse | Shift=Sprint | Space/Ctrl=Up/Down");
    else if(g_cam.mode==CameraMode::TPS)ImGui::Text("WASD=Move Player | LMB+Drag=Orbit | Scroll=Distance");
    else ImGui::Text("LMB+Drag=Rotate | Scroll=Zoom");
    ImGui::End();
    
    if(!g_showEditor)return;
    
    ImGui::Begin("Camera Editor [E]",&g_showEditor,ImGuiWindowFlags_AlwaysAutoResize);
    
    if(ImGui::BeginTabBar("CamTabs")){
        if(ImGui::BeginTabItem("Camera")){
            ImGui::Text("=== Mode ===");
            int m=(int)g_cam.mode;
            if(ImGui::RadioButton("Orbit",m==0)){g_cam.SwitchMode(CameraMode::Orbit);glfwSetInputMode(Engine::Instance().GetWindow(),GLFW_CURSOR,GLFW_CURSOR_NORMAL);g_fpsCursorLocked=false;}ImGui::SameLine();
            if(ImGui::RadioButton("FPS",m==1)){g_cam.SwitchMode(CameraMode::FPS);glfwSetInputMode(Engine::Instance().GetWindow(),GLFW_CURSOR,GLFW_CURSOR_DISABLED);g_fpsCursorLocked=true;}ImGui::SameLine();
            if(ImGui::RadioButton("TPS",m==2)){g_cam.SwitchMode(CameraMode::TPS);glfwSetInputMode(Engine::Instance().GetWindow(),GLFW_CURSOR,GLFW_CURSOR_NORMAL);g_fpsCursorLocked=false;}
            
            ImGui::Separator();
            ImGui::SliderFloat("Yaw",&g_cam.yaw,-180,180);
            ImGui::SliderFloat("Pitch",&g_cam.pitch,-89,89);
            ImGui::SliderFloat("FOV",&g_cam.fov,10,120);
            ImGui::SliderFloat("Near",&g_cam.nearPlane,0.01f,1);
            ImGui::SliderFloat("Far",&g_cam.farPlane,10,500);
            
            if(g_cam.mode==CameraMode::Orbit){
                ImGui::Separator();ImGui::Text("=== Orbit ===");
                ImGui::SliderFloat("Distance",&g_cam.orbitDistance,1,50);
                ImGui::DragFloat3("Target",&g_cam.orbitTarget.x,0.1f);
            }else if(g_cam.mode==CameraMode::FPS){
                ImGui::Separator();ImGui::Text("=== FPS ===");
                ImGui::DragFloat3("Position",&g_cam.fpsPosition.x,0.1f);
                ImGui::SliderFloat("Move Speed",&g_cam.moveSpeed,1,30);
                ImGui::SliderFloat("Sprint x",&g_cam.sprintMultiplier,1,5);
                ImGui::SliderFloat("Mouse Sens",&g_cam.mouseSensitivity,0.01f,0.5f);
            }else{
                ImGui::Separator();ImGui::Text("=== TPS ===");
                ImGui::SliderFloat("TPS Distance",&g_cam.tpsDistance,2,20);
                ImGui::SliderFloat("TPS Height",&g_cam.tpsHeight,0,10);
                ImGui::SliderFloat("Smooth Speed",&g_cam.tpsSmooth,1,20);
                ImGui::DragFloat3("Player Pos",&g_playerPos.x,0.1f);
                ImGui::SliderFloat("Player Speed",&g_playerSpeed,1,20);
            }
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Objects")){
            for(int i=0;i<(int)g_objects.size();i++){
                auto&o=g_objects[i];ImGui::PushID(i);
                if(ImGui::TreeNode("##o","[%d] %s",i,o.name.c_str())){
                    ImGui::Checkbox("Visible",&o.visible);
                    ImGui::DragFloat3("Pos",&o.pos.x,0.05f);
                    ImGui::DragFloat3("Scale",&o.scl.x,0.01f,0.001f,100);
                    ImGui::SliderFloat("Shininess",&o.shininess,1,512);
                    ImGui::SliderFloat("Tiling",&o.tiling,0.1f,10);
                    const char*matN[]={"None","Bricks","WoodFloor","Metal","MetalPlates","Wood"};
                    int sel=o.matIdx+1;if(ImGui::Combo("Material",&sel,matN,6))o.matIdx=sel-1;
                    ImGui::TreePop();
                }ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Lighting")){
            ImGui::SliderFloat("Ambient",&g_ambient,0,0.5f);
            ImGui::Separator();
            ImGui::Checkbox("Dir Light",&g_dirLight.on);
            ImGui::DragFloat3("Dir",&g_dirLight.dir.x,0.01f,-1,1);
            ImGui::ColorEdit3("Dir Color",&g_dirLight.color.x);
            ImGui::SliderFloat("Dir Intensity",&g_dirLight.intensity,0,3);
            ImGui::Separator();
            ImGui::Checkbox("Point Light",&g_ptLight.on);
            ImGui::DragFloat3("Pt Pos",&g_ptLight.pos.x,0.1f);
            ImGui::ColorEdit3("Pt Color",&g_ptLight.color.x);
            ImGui::SliderFloat("Pt Intensity",&g_ptLight.intensity,0,5);
            ImGui::Separator();
            if(ImGui::Button("Daylight")){g_dirLight.dir=Vec3(0.4f,0.7f,0.3f);g_dirLight.color=Vec3(1,0.95f,0.9f);g_ambient=0.15f;g_bgColor=Vec4(0.35f,0.45f,0.6f,1);}
            ImGui::SameLine();
            if(ImGui::Button("Night")){g_dirLight.dir=Vec3(-0.2f,0.5f,-0.3f);g_dirLight.color=Vec3(0.3f,0.35f,0.5f);g_ambient=0.03f;g_bgColor=Vec4(0.02f,0.02f,0.05f,1);}
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Render")){
            ImGui::ColorEdit4("Background",&g_bgColor.x);
            ImGui::Checkbox("Wireframe",&g_wireframe);
            ImGui::Checkbox("Show Grid",&g_showGrid);
            ImGui::Separator();
            ImGui::Text("Renderer: %s",glGetString(GL_RENDERER));
            ImGui::Text("OpenGL: %s",glGetString(GL_VERSION));
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

// ============================================================================
// 滚轮 & 清理 & main
// ============================================================================
void ScrollCB(GLFWwindow*,double,double y){g_cam.ProcessScroll((float)y);}

void Cleanup(){
    for(auto&o:g_objects){o.model.Destroy();o.mesh.Destroy();}
    for(auto&m:g_materials)m.Destroy();
    g_objects.clear();g_materials.clear();
    g_matShader.reset();g_basicShader.reset();
}

int main(){
    std::cout<<"=== Example 23: 3D Camera System ==="<<std::endl;
    Engine&engine=Engine::Instance();
    EngineConfig cfg;cfg.windowWidth=1280;cfg.windowHeight=720;
    cfg.windowTitle="Mini Engine - 3D Camera System";
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
