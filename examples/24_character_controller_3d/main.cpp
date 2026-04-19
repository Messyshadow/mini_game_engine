/**
 * 第24章：3D角色控制器 (3D Character Controller)
 *
 * WASD移动角色，跳跃，冲刺，TPS摄像机跟随。
 * 角色朝向平滑转向移动方向。
 *
 * 操作：
 * - WASD：移动角色（相对摄像机方向）
 * - Space：跳跃（支持二段跳）
 * - Shift：冲刺跑
 * - F：闪避冲刺（有冷却）
 * - 鼠标左键拖动：旋转摄像机视角
 * - 滚轮：调整跟随距离
 * - Tab：切换Orbit/TPS模式
 * - E：编辑面板 | R：重置位置
 */

#include "engine/Engine.h"
#include "engine/Shader.h"
#include "engine3d/Mesh3D.h"
#include "engine3d/ModelLoader.h"
#include "engine3d/Texture3D.h"
#include "engine3d/Camera3D.h"
#include "engine3d/CharacterController3D.h"
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
std::unique_ptr<Shader> g_shader;
Camera3D g_cam;
CharacterController3D g_player;

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

// 角色模型
Model3D g_playerModel;
bool g_hasPlayerModel = false;

// 光照
struct{Vec3 dir=Vec3(0.4f,0.7f,0.3f);Vec3 color=Vec3(1,0.95f,0.9f);float intensity=0.8f;bool on=true;}g_dirLight;
struct{Vec3 pos=Vec3(3,4,3);Vec3 color=Vec3(1,0.8f,0.6f);float intensity=1.5f;float lin=0.09f,quad=0.032f;bool on=true;}g_ptLight;
float g_ambient=0.15f;

bool g_showEditor=true, g_wireframe=false;
Vec4 g_bgColor(0.15f,0.18f,0.25f,1);
bool g_mouseLDown=false; double g_lastMX=0,g_lastMY=0;

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    g_shader=std::make_unique<Shader>();
    const char*vs[]={"data/shader/material.vs","../data/shader/material.vs","../../data/shader/material.vs"};
    const char*fs[]={"data/shader/material.fs","../data/shader/material.fs","../../data/shader/material.fs"};
    bool ok=false;for(int i=0;i<3;i++){if(g_shader->LoadFromFile(vs[i],fs[i])){ok=true;break;}}
    if(!ok){std::cerr<<"Shader load failed!"<<std::endl;return false;}
    g_shader->Bind();
    g_shader->SetInt("uDiffuseMap",0);g_shader->SetInt("uNormalMap",1);g_shader->SetInt("uRoughnessMap",2);
    
    // 材质
    {Material3D m;m.LoadFromDirectory("woodfloor","Wood Floor");m.tiling=3;g_materials.push_back(std::move(m));}
    {Material3D m;m.LoadFromDirectory("bricks","Bricks");g_materials.push_back(std::move(m));}
    {Material3D m;m.LoadFromDirectory("metal","Metal");g_materials.push_back(std::move(m));}
    {Material3D m;m.LoadFromDirectory("metalplates","Metal Plates");g_materials.push_back(std::move(m));}
    {Material3D m;m.LoadFromDirectory("wood","Wood");g_materials.push_back(std::move(m));}
    
    // 角色模型
    g_playerModel=ModelLoader::LoadWithFallback("fbx/X Bot.fbx");
    g_hasPlayerModel=g_playerModel.IsValid();
    
    // 场景 — 地面
    {SceneObj o;o.name="Ground";o.mesh=Mesh3D::CreatePlane(30,30,1,1);o.matIdx=0;o.tiling=8;o.shininess=8;g_objects.push_back(std::move(o));}
    // 砖墙障碍物
    {SceneObj o;o.name="Wall 1";o.mesh=Mesh3D::CreateCube(2);o.pos=Vec3(-5,1,3);o.matIdx=1;o.shininess=16;g_objects.push_back(std::move(o));}
    {SceneObj o;o.name="Wall 2";o.mesh=Mesh3D::CreateCube(2);o.pos=Vec3(5,1,-3);o.matIdx=1;o.shininess=16;g_objects.push_back(std::move(o));}
    // 金属柱
    {SceneObj o;o.name="Pillar";o.mesh=Mesh3D::CreateCube(1);o.pos=Vec3(0,2,-8);o.scl=Vec3(1,4,1);o.matIdx=3;o.shininess=64;g_objects.push_back(std::move(o));}
    // 木箱
    {SceneObj o;o.name="Crate 1";o.mesh=Mesh3D::CreateCube(1);o.pos=Vec3(3,0.5f,6);o.matIdx=4;o.shininess=16;g_objects.push_back(std::move(o));}
    {SceneObj o;o.name="Crate 2";o.mesh=Mesh3D::CreateCube(0.8f);o.pos=Vec3(3.5f,1.4f,6);o.matIdx=4;o.shininess=16;g_objects.push_back(std::move(o));}
    // 金属球
    {SceneObj o;o.name="Metal Ball";o.mesh=Mesh3D::CreateSphere(0.8f,24,16);o.pos=Vec3(-3,0.8f,-5);o.matIdx=2;o.shininess=128;g_objects.push_back(std::move(o));}
    // 平台（跳上去测试）
    {SceneObj o;o.name="Platform";o.mesh=Mesh3D::CreateCube(1);o.pos=Vec3(-8,1,0);o.scl=Vec3(3,0.3f,3);o.matIdx=3;o.shininess=32;g_objects.push_back(std::move(o));}
    
    // 角色控制器
    g_player.position=Vec3(0,0.9f,0);
    g_player.params.groundHeight=0;
    
    // 摄像机
    g_cam.mode=CameraMode::TPS;
    g_cam.tpsDistance=8;g_cam.tpsHeight=3;
    g_cam.pitch=20;g_cam.yaw=-30;
    
    std::cout<<"\n=== Chapter 24: 3D Character Controller ==="<<std::endl;
    std::cout<<"WASD: Move | Space: Jump(x2) | Shift: Sprint | F: Dash"<<std::endl;
    std::cout<<"LMB+Drag: Rotate Camera | Scroll: Distance | Tab: Mode"<<std::endl;
    std::cout<<"E: Editor | R: Reset Position"<<std::endl;
    return true;
}

// ============================================================================
// 更新
// ============================================================================
void Update(float dt) {
    Engine&eng=Engine::Instance();GLFWwindow*w=eng.GetWindow();
    if(dt>0.1f)dt=0.1f;
    
    static bool eL=0;bool e=glfwGetKey(w,GLFW_KEY_E);if(e&&!eL)g_showEditor=!g_showEditor;eL=e;
    static bool wfL=0;bool wf=glfwGetKey(w,GLFW_KEY_P);if(wf&&!wfL)g_wireframe=!g_wireframe;wfL=wf;
    static bool lL=0;bool l=glfwGetKey(w,GLFW_KEY_L);if(l&&!lL){g_dirLight.on=!g_dirLight.on;g_ptLight.on=!g_ptLight.on;}lL=l;
    
    // R重置
    static bool rL=0;bool r=glfwGetKey(w,GLFW_KEY_R);
    if(r&&!rL)g_player.Reset();rL=r;
    
    // Tab切换模式
    static bool tabL=0;bool tab=glfwGetKey(w,GLFW_KEY_TAB);
    if(tab&&!tabL){
        if(g_cam.mode==CameraMode::TPS)g_cam.SwitchMode(CameraMode::Orbit);
        else g_cam.SwitchMode(CameraMode::TPS);
    }tabL=tab;
    
    // 鼠标左键旋转摄像机
    double mx,my;glfwGetCursorPos(w,&mx,&my);
    if(glfwGetMouseButton(w,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS&&!ImGui::GetIO().WantCaptureMouse){
        if(!g_mouseLDown){g_mouseLDown=true;g_lastMX=mx;g_lastMY=my;}
        g_cam.ProcessMouseMove((float)(mx-g_lastMX),(float)(my-g_lastMY));
        g_lastMX=mx;g_lastMY=my;
    }else g_mouseLDown=false;
    
    // ---- 角色控制 ----
    float inputFwd=0,inputRt=0;
    if(glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS)inputFwd=1;
    if(glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS)inputFwd=-1;
    if(glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS)inputRt=-1;
    if(glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS)inputRt=1;
    bool sprint=glfwGetKey(w,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS;
    
    g_player.ProcessMovement(inputFwd,inputRt,g_cam.yaw,sprint,dt);
    
    // 跳跃
    static bool spL=0;bool sp=glfwGetKey(w,GLFW_KEY_SPACE);
    if(sp&&!spL)g_player.ProcessJump(true);spL=sp;
    
    // 冲刺
    static bool fL=0;bool f=glfwGetKey(w,GLFW_KEY_F);
    if(f&&!fL)g_player.ProcessDash(true);fL=f;
    
    g_player.Update(dt);
    
    // 摄像机跟随角色
    g_cam.tpsTarget=g_player.position;
    g_cam.orbitTarget=g_player.position+Vec3(0,1,0);
    g_cam.UpdateTPS(dt);
}

// ============================================================================
// 渲染
// ============================================================================
void SetLights(){
    g_shader->SetFloat("uAmbientStrength",g_ambient);
    g_shader->SetInt("uNumDirLights",g_dirLight.on?1:0);
    if(g_dirLight.on){
        g_shader->SetVec3("uDirLights[0].direction",g_dirLight.dir);
        g_shader->SetVec3("uDirLights[0].color",g_dirLight.color);
        g_shader->SetFloat("uDirLights[0].intensity",g_dirLight.intensity);
    }
    g_shader->SetInt("uNumPointLights",g_ptLight.on?1:0);
    if(g_ptLight.on){
        g_shader->SetVec3("uPointLights[0].position",g_ptLight.pos);
        g_shader->SetVec3("uPointLights[0].color",g_ptLight.color);
        g_shader->SetFloat("uPointLights[0].intensity",g_ptLight.intensity);
        g_shader->SetFloat("uPointLights[0].constant",1);
        g_shader->SetFloat("uPointLights[0].linear",g_ptLight.lin);
        g_shader->SetFloat("uPointLights[0].quadratic",g_ptLight.quad);
    }
}

void RenderObj(SceneObj&o){
    if(!o.visible)return;
    Mat4 model=Mat4::Translate(o.pos.x,o.pos.y,o.pos.z)
              *Mat4::RotateY(o.rot.y)*Mat4::RotateX(o.rot.x)*Mat4::RotateZ(o.rot.z)
              *Mat4::Scale(o.scl.x,o.scl.y,o.scl.z);
    g_shader->SetMat4("uModel",model.m);
    Mat3 nm=model.GetRotationMatrix().Inverse().Transposed();
    g_shader->SetMat3("uNormalMatrix",nm.m);
    
    bool hasMat=o.matIdx>=0&&o.matIdx<(int)g_materials.size();
    if(hasMat){auto&m=g_materials[o.matIdx];m.Bind();g_shader->SetBool("uUseDiffuseMap",m.hasDiffuse);g_shader->SetBool("uUseNormalMap",m.hasNormal);g_shader->SetBool("uUseRoughnessMap",m.hasRoughness);g_shader->SetFloat("uTexTiling",o.tiling);}
    else{g_shader->SetBool("uUseDiffuseMap",false);g_shader->SetBool("uUseNormalMap",false);g_shader->SetBool("uUseRoughnessMap",false);g_shader->SetFloat("uTexTiling",1);}
    g_shader->SetBool("uUseVertexColor",false);
    g_shader->SetVec4("uBaseColor",Vec4(0.7f,0.7f,0.7f,1));
    g_shader->SetFloat("uShininess",o.shininess);
    o.Draw();
}

void Render(){
    Engine&eng=Engine::Instance();int sw=eng.GetWindowWidth(),sh=eng.GetWindowHeight();
    float aspect=(float)sw/(float)sh;
    glClearColor(g_bgColor.x,g_bgColor.y,g_bgColor.z,g_bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(g_wireframe)glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);else glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    
    Mat4 view=g_cam.GetViewMatrix(),proj=g_cam.GetProjectionMatrix(aspect);
    g_shader->Bind();
    g_shader->SetMat4("uView",view.m);
    g_shader->SetMat4("uProjection",proj.m);
    g_shader->SetVec3("uViewPos",g_cam.GetPosition());
    SetLights();
    
    // 场景
    for(auto&o:g_objects)RenderObj(o);
    
    // 角色
    {
        Mat4 model=Mat4::Translate(g_player.position.x,g_player.position.y-g_player.params.height*0.5f,g_player.position.z)
                  *Mat4::RotateY(g_player.state.currentYaw)
                  *Mat4::Scale(0.02f,0.02f,0.02f);
        g_shader->SetMat4("uModel",model.m);
        Mat3 nm=model.GetRotationMatrix().Inverse().Transposed();
        g_shader->SetMat3("uNormalMatrix",nm.m);
        g_shader->SetBool("uUseDiffuseMap",false);g_shader->SetBool("uUseNormalMap",false);g_shader->SetBool("uUseRoughnessMap",false);
        g_shader->SetBool("uUseVertexColor",true);g_shader->SetFloat("uShininess",32);g_shader->SetFloat("uTexTiling",1);
        
        if(g_hasPlayerModel)g_playerModel.Draw();
        else{Mesh3D tmp=Mesh3D::CreateColorCube(50);tmp.Draw();tmp.Destroy();}
    }
    
    // 冲刺方向指示（冲刺中显示）
    if(g_player.state.dashing){
        Vec3 dp=g_player.position+g_player.state.dashDirection*2;
        Mat4 m=Mat4::Translate(dp.x,dp.y+0.5f,dp.z)*Mat4::Scale(0.15f,0.15f,0.15f);
        g_shader->SetMat4("uModel",m.m);
        g_shader->SetBool("uUseVertexColor",false);
        g_shader->SetVec4("uBaseColor",Vec4(0.2f,0.8f,1,1));
        Mesh3D s=Mesh3D::CreateSphere(1,8,6);s.Draw();s.Destroy();
    }
    
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui(){
    // HUD
    ImGui::SetNextWindowPos(ImVec2(10,10));
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("##HUD",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove);
    ImGui::Text("Ch24: 3D Character Controller");
    const char*modeN[]={"Orbit","FPS","TPS"};
    ImGui::Text("Camera: %s",modeN[(int)g_cam.mode]);
    ImGui::Text("Pos: (%.1f, %.1f, %.1f)",g_player.position.x,g_player.position.y,g_player.position.z);
    ImGui::Text("Vel: (%.1f, %.1f, %.1f)",g_player.state.velocity.x,g_player.state.velocity.y,g_player.state.velocity.z);
    ImGui::Text("Ground:%s Jump:%d/%d %s%s",
        g_player.state.grounded?"Y":"N",g_player.state.jumpCount,g_player.params.maxJumps,
        g_player.state.sprinting?"[SPRINT]":"",g_player.state.dashing?"[DASH]":"");
    if(g_player.state.dashCooldownTimer>0)ImGui::Text("Dash CD: %.1f",g_player.state.dashCooldownTimer);
    ImGui::Text("FPS: %.0f",Engine::Instance().GetFPS());
    ImGui::Separator();
    ImGui::Text("WASD:Move Space:Jump Shift:Sprint F:Dash");
    ImGui::Text("LMB+Drag:Camera Scroll:Distance Tab:Mode");
    ImGui::End();
    
    if(!g_showEditor)return;
    
    ImGui::Begin("Character Editor [E]",&g_showEditor,ImGuiWindowFlags_AlwaysAutoResize);
    if(ImGui::BeginTabBar("Ch24Tabs")){
        if(ImGui::BeginTabItem("Physics")){
            ImGui::Text("=== Movement ===");
            ImGui::SliderFloat("Move Speed",&g_player.params.moveSpeed,1,20);
            ImGui::SliderFloat("Sprint Speed",&g_player.params.sprintSpeed,5,30);
            ImGui::SliderFloat("Acceleration",&g_player.params.acceleration,5,50);
            ImGui::SliderFloat("Deceleration",&g_player.params.deceleration,5,50);
            ImGui::SliderFloat("Air Control",&g_player.params.airControl,0,1);
            ImGui::Separator();
            ImGui::Text("=== Jump ===");
            ImGui::SliderFloat("Jump Force",&g_player.params.jumpForce,3,20);
            ImGui::SliderFloat("Gravity",&g_player.params.gravity,5,40);
            ImGui::SliderFloat("Max Fall Speed",&g_player.params.maxFallSpeed,10,50);
            ImGui::SliderInt("Max Jumps",&g_player.params.maxJumps,1,3);
            ImGui::Separator();
            ImGui::Text("=== Dash ===");
            ImGui::SliderFloat("Dash Speed",&g_player.params.dashSpeed,10,40);
            ImGui::SliderFloat("Dash Duration",&g_player.params.dashDuration,0.05f,0.5f);
            ImGui::SliderFloat("Dash Cooldown",&g_player.params.dashCooldown,0.2f,5);
            ImGui::Separator();
            ImGui::SliderFloat("Turn Speed",&g_player.params.turnSpeed,3,30);
            if(ImGui::Button("Reset Position"))g_player.Reset();
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Camera")){
            int m=(int)g_cam.mode;
            ImGui::RadioButton("Orbit",m==0);ImGui::SameLine();ImGui::RadioButton("TPS",m==2);
            ImGui::SliderFloat("Yaw",&g_cam.yaw,-180,180);
            ImGui::SliderFloat("Pitch",&g_cam.pitch,-20,89);
            ImGui::SliderFloat("FOV",&g_cam.fov,10,120);
            if(g_cam.mode==CameraMode::TPS){
                ImGui::SliderFloat("Distance",&g_cam.tpsDistance,2,20);
                ImGui::SliderFloat("Height",&g_cam.tpsHeight,0,10);
                ImGui::SliderFloat("Smooth",&g_cam.tpsSmooth,1,20);
            }else{
                ImGui::SliderFloat("Orbit Dist",&g_cam.orbitDistance,2,30);
            }
            ImGui::SliderFloat("Mouse Sens",&g_cam.mouseSensitivity,0.01f,0.5f);
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Objects")){
            for(int i=0;i<(int)g_objects.size();i++){
                auto&o=g_objects[i];ImGui::PushID(i);
                if(ImGui::TreeNode("##o","[%d] %s",i,o.name.c_str())){
                    ImGui::Checkbox("Visible",&o.visible);
                    ImGui::DragFloat3("Pos",&o.pos.x,0.05f);
                    ImGui::DragFloat3("Scale",&o.scl.x,0.01f,0.01f,100);
                    const char*mn[]={"None","WoodFloor","Bricks","Metal","MetalPlates","Wood"};
                    int sel=o.matIdx+1;if(ImGui::Combo("Material",&sel,mn,6))o.matIdx=sel-1;
                    ImGui::SliderFloat("Tiling",&o.tiling,0.1f,10);
                    ImGui::TreePop();
                }ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        
        if(ImGui::BeginTabItem("Lighting")){
            ImGui::SliderFloat("Ambient",&g_ambient,0,0.5f);
            ImGui::Checkbox("Dir Light",&g_dirLight.on);
            ImGui::DragFloat3("Dir",&g_dirLight.dir.x,0.01f,-1,1);
            ImGui::ColorEdit3("Dir Color",&g_dirLight.color.x);
            ImGui::SliderFloat("Dir Int",&g_dirLight.intensity,0,3);
            ImGui::Separator();
            ImGui::Checkbox("Point Light",&g_ptLight.on);
            ImGui::DragFloat3("Pt Pos",&g_ptLight.pos.x,0.1f);
            ImGui::ColorEdit3("Pt Color",&g_ptLight.color.x);
            ImGui::SliderFloat("Pt Int",&g_ptLight.intensity,0,5);
            ImGui::Separator();
            if(ImGui::Button("Daylight")){g_dirLight.dir=Vec3(0.4f,0.7f,0.3f);g_dirLight.color=Vec3(1,0.95f,0.9f);g_ambient=0.15f;g_bgColor=Vec4(0.35f,0.45f,0.6f,1);}
            ImGui::SameLine();
            if(ImGui::Button("Night")){g_dirLight.dir=Vec3(-0.2f,0.5f,-0.3f);g_dirLight.color=Vec3(0.3f,0.35f,0.5f);g_ambient=0.03f;g_bgColor=Vec4(0.02f,0.02f,0.05f,1);}
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

// ============================================================================
// main
// ============================================================================
void ScrollCB(GLFWwindow*,double,double y){g_cam.ProcessScroll((float)y);}

void Cleanup(){
    for(auto&o:g_objects){o.model.Destroy();o.mesh.Destroy();}
    for(auto&m:g_materials)m.Destroy();
    g_playerModel.Destroy();
    g_objects.clear();g_materials.clear();g_shader.reset();
}

int main(){
    std::cout<<"=== Example 24: 3D Character Controller ==="<<std::endl;
    Engine&engine=Engine::Instance();
    EngineConfig cfg;cfg.windowWidth=1280;cfg.windowHeight=720;
    cfg.windowTitle="Mini Engine - 3D Character Controller";
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
