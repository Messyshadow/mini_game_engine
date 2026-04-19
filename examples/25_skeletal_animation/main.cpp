/**
 * 第25章：骨骼动画 (Skeletal Animation)
 *
 * 加载Mixamo角色模型(X Bot)和动画(Idle/Run/Punch/Death)，
 * 实现骨骼树遍历、关键帧插值、顶点蒙皮，以及动画切换和混合。
 *
 * 操作：
 * - 鼠标左键拖动：旋转摄像机
 * - 滚轮：缩放
 * - WASD：移动角色（D=-1, A=1）
 * - 1/2/3/4：切换动画（Idle/Run/Punch/Death）
 * - Space：播放/暂停
 * - +/-：调整播放速度
 * - B：切换骨骼可视化
 * - E：编辑面板
 * - R：重置
 */

#include "engine/Engine.h"
#include "engine/Shader.h"
#include "engine3d/Mesh3D.h"
#include "engine3d/Texture3D.h"
#include "engine3d/Camera3D.h"
#include "engine3d/Animator.h"
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
// 全局状态
// ============================================================================
std::unique_ptr<Shader> g_skinShader;   // 蒙皮着色器
std::unique_ptr<Shader> g_matShader;    // 材质着色器（地面等静态物体）
Camera3D g_cam;

// 骨骼模型 & 动画
SkeletalModel g_skelModel;
std::vector<AnimClip> g_anims;
int g_currentAnim = 0;
float g_animTime = 0;
float g_animSpeed = 1.0f;
bool g_playing = true;
Mat4 g_boneMatrices[MAX_BONES];

// 角色
Vec3 g_playerPos(0, 0, 0);
float g_playerYaw = 0;
float g_playerScale = 0.01f;  // Mixamo模型通常很大，需要缩小

// 场景
struct SceneObj {
    Mesh3D mesh;
    Vec3 pos, scl=Vec3(1,1,1);
    int matIdx=-1; float tiling=1, shininess=32;
    std::string name;
};
std::vector<SceneObj> g_sceneObjs;
std::vector<Material3D> g_materials;

// 光照
struct{Vec3 dir=Vec3(0.4f,0.7f,0.3f);Vec3 color=Vec3(1,0.95f,0.9f);float intensity=0.8f;}g_dirLight;
struct{Vec3 pos=Vec3(2,4,3);Vec3 color=Vec3(1,0.85f,0.7f);float intensity=1.2f;float lin=0.09f,quad=0.032f;}g_ptLight;
float g_ambient = 0.2f;

// 显示
bool g_showEditor = true;
bool g_showBones = false;
bool g_wireframe = false;
Vec4 g_bgColor(0.12f, 0.14f, 0.2f, 1);
bool g_mouseLDown = false;
double g_lastMX=0, g_lastMY=0;
float g_fps=0; int g_frameCnt=0; float g_fpsTimer=0;

// 动画混合
bool g_useBlend = true;
float g_blendDuration = 0.3f;
float g_blendTimer = 0;
int g_prevAnim = 0;
Mat4 g_prevBoneMatrices[MAX_BONES];
Mat4 g_nextBoneMatrices[MAX_BONES];

// ============================================================================
// 滚轮回调
// ============================================================================
void ScrollCB(GLFWwindow*, double, double dy) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    g_cam.ProcessScroll((float)dy);
}

// ============================================================================
// 设置光照Uniform（复用material.fs的接口）
// ============================================================================
void SetLightUniforms(Shader* s) {
    s->SetFloat("uAmbientStrength", g_ambient);
    s->SetInt("uNumDirLights", 1);
    s->SetVec3("uDirLights[0].direction", g_dirLight.dir);
    s->SetVec3("uDirLights[0].color", g_dirLight.color);
    s->SetFloat("uDirLights[0].intensity", g_dirLight.intensity);
    s->SetInt("uNumPointLights", 1);
    s->SetVec3("uPointLights[0].position", g_ptLight.pos);
    s->SetVec3("uPointLights[0].color", g_ptLight.color);
    s->SetFloat("uPointLights[0].intensity", g_ptLight.intensity);
    s->SetFloat("uPointLights[0].constant", 1);
    s->SetFloat("uPointLights[0].linear", g_ptLight.lin);
    s->SetFloat("uPointLights[0].quadratic", g_ptLight.quad);
}

// ============================================================================
// 初始化
// ============================================================================
bool Initialize() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // ---- 蒙皮着色器 ----
    g_skinShader = std::make_unique<Shader>();
    const char* svs[] = {"data/shader/skinning.vs","../data/shader/skinning.vs","../../data/shader/skinning.vs"};
    const char* sfs[] = {"data/shader/skinning.fs","../data/shader/skinning.fs","../../data/shader/skinning.fs"};
    bool ok=false; for(int i=0;i<3;i++){if(g_skinShader->LoadFromFile(svs[i],sfs[i])){ok=true;break;}}
    if(!ok){std::cerr<<"Failed to load skinning shader!"<<std::endl;return false;}

    // ---- 材质着色器（地面） ----
    g_matShader = std::make_unique<Shader>();
    const char* mvs[] = {"data/shader/material.vs","../data/shader/material.vs","../../data/shader/material.vs"};
    const char* mfs[] = {"data/shader/material.fs","../data/shader/material.fs","../../data/shader/material.fs"};
    ok=false; for(int i=0;i<3;i++){if(g_matShader->LoadFromFile(mvs[i],mfs[i])){ok=true;break;}}
    if(!ok){std::cerr<<"Failed to load material shader!"<<std::endl;return false;}

    // 纹理单元绑定
    g_skinShader->Bind();
    g_skinShader->SetInt("uDiffuseMap",0); g_skinShader->SetInt("uNormalMap",1); g_skinShader->SetInt("uRoughnessMap",2);
    g_matShader->Bind();
    g_matShader->SetInt("uDiffuseMap",0); g_matShader->SetInt("uNormalMap",1); g_matShader->SetInt("uRoughnessMap",2);

    // ---- 加载骨骼模型 ----
    if (!Animator::LoadModelFallback("fbx/X Bot.fbx", g_skelModel)) {
        std::cerr << "Failed to load X Bot model!" << std::endl;
        return false;
    }

    // ---- 加载动画 ----
    const char* animFiles[] = {"fbx/Idle.fbx", "fbx/Running.fbx", "fbx/Punching.fbx", "fbx/Death.fbx"};
    const char* animNames[] = {"Idle", "Running", "Punching", "Death"};
    for (int i = 0; i < 4; i++) {
        AnimClip clip;
        if (Animator::LoadAnimFallback(animFiles[i], clip)) {
            clip.name = animNames[i];
            g_anims.push_back(std::move(clip));
        } else {
            std::cerr << "Warning: failed to load anim " << animFiles[i] << std::endl;
        }
    }
    if (g_anims.empty()) {
        std::cerr << "No animations loaded!" << std::endl;
        return false;
    }

    // ---- 材质 ----
    {Material3D m; m.LoadFromDirectory("woodfloor","Wood Floor"); m.tiling=6; g_materials.push_back(std::move(m));}

    // ---- 场景物体：地面 ----
    {SceneObj o; o.name="Ground"; o.mesh=Mesh3D::CreatePlane(20,20,1,1); o.matIdx=0; o.tiling=6; o.shininess=8;
     g_sceneObjs.push_back(std::move(o));}

    // ---- 摄像机 ----
    g_cam.mode = CameraMode::Orbit;
    g_cam.orbitDistance = 5.0f; g_cam.tpsDistance = 5.0f;
    g_cam.orbitTarget = Vec3(0, 1, 0);
    g_cam.yaw = -30.0f;
    g_cam.pitch = 25.0f;

    // ---- 初始化骨骼矩阵 ----
    for(int i=0;i<MAX_BONES;i++) g_boneMatrices[i] = Mat4::Identity();

    std::cout << "[Ch25] Skeletal animation initialized" << std::endl;
    std::cout << "[Ch25] 1-4=Switch Anim  Space=Play/Pause  +/-=Speed" << std::endl;
    return true;
}

// ============================================================================
// 切换动画（带混合）
// ============================================================================
void SwitchAnim(int newIdx) {
    if (newIdx < 0 || newIdx >= (int)g_anims.size() || newIdx == g_currentAnim) return;

    if (g_useBlend) {
        // 保存当前动画的骨骼矩阵作为混合起始
        for (int i = 0; i < MAX_BONES; i++) g_prevBoneMatrices[i] = g_boneMatrices[i];
        g_prevAnim = g_currentAnim;
        g_blendTimer = g_blendDuration;
    }

    g_currentAnim = newIdx;
    g_animTime = 0;
}

// ============================================================================
// 更新
// ============================================================================
void Update(float dt) {
    Engine& engine = Engine::Instance();
    GLFWwindow* w = engine.GetWindow();

    // FPS
    g_frameCnt++; g_fpsTimer+=dt;
    if(g_fpsTimer>=1){g_fps=g_frameCnt/g_fpsTimer;g_frameCnt=0;g_fpsTimer=0;}

    // ---- 快捷键 ----
    static bool eL=false, bL=false, spL=false, rL=false;
    {bool d=glfwGetKey(w,GLFW_KEY_E)==GLFW_PRESS;if(d&&!eL)g_showEditor=!g_showEditor;eL=d;}
    {bool d=glfwGetKey(w,GLFW_KEY_B)==GLFW_PRESS;if(d&&!bL)g_showBones=!g_showBones;bL=d;}
    {bool d=glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS;if(d&&!spL)g_playing=!g_playing;spL=d;}
    {bool d=glfwGetKey(w,GLFW_KEY_R)==GLFW_PRESS;if(d&&!rL){g_animTime=0;g_playerPos=Vec3(0,0,0);g_playerYaw=0;}rL=d;}

    // 数字键切换动画
    for (int i = 0; i < 4 && i < (int)g_anims.size(); i++) {
        static bool numLast[4]={};
        bool d = glfwGetKey(w, GLFW_KEY_1 + i) == GLFW_PRESS;
        if (d && !numLast[i]) SwitchAnim(i);
        numLast[i] = d;
    }

    // 速度调整
    if(glfwGetKey(w,GLFW_KEY_EQUAL)==GLFW_PRESS) g_animSpeed=std::min(5.0f, g_animSpeed+dt*2);
    if(glfwGetKey(w,GLFW_KEY_MINUS)==GLFW_PRESS) g_animSpeed=std::max(0.1f, g_animSpeed-dt*2);

    // ---- 鼠标 ----
    double mx, my; glfwGetCursorPos(w, &mx, &my);
    if(glfwGetMouseButton(w,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse){
        if(!g_mouseLDown){g_mouseLDown=true;g_lastMX=mx;g_lastMY=my;}
        g_cam.ProcessMouseMove((float)(mx-g_lastMX),(float)(my-g_lastMY));
        g_lastMX=mx;g_lastMY=my;
    } else g_mouseLDown=false;
    g_lastMX=mx;g_lastMY=my;

    // ---- WASD移动角色 ----
    float inputFwd=0, inputRt=0;
    if(glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS) inputFwd-=1;
    if(glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS) inputFwd+=1;
    if(glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS) inputRt=-1;
    if(glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS) inputRt=1;

    bool isMoving = (inputFwd!=0 || inputRt!=0);
    if (isMoving) {
        float _yr=g_cam.yaw*0.0174533f; Vec3 cf(std::sin(_yr),0,std::cos(_yr));
        Vec3 cr(-std::cos(_yr),0,std::sin(_yr));
        float rl=std::sqrt(cr.x*cr.x+cr.z*cr.z);
        if(rl>0.001f){cr.x/=rl;cr.z/=rl;}
        Vec3 dir = cf*inputFwd + cr*inputRt;
        float dl=std::sqrt(dir.x*dir.x+dir.z*dir.z);
        if(dl>0.001f){
            dir.x/=dl; dir.z/=dl;
            g_playerPos = g_playerPos + dir*(3.0f*dt);
            g_playerYaw = std::atan2(dir.x, dir.z);
        }
        // 自动切换到Run
        if (g_anims.size()>1 && g_currentAnim==0) SwitchAnim(1);
    } else {
        // 自动切换到Idle
        if (g_anims.size()>0 && g_currentAnim==1) SwitchAnim(0);
    }

    // 摄像机跟随
    g_cam.tpsTarget = g_playerPos + Vec3(0, 1, 0); g_cam.orbitTarget = g_playerPos + Vec3(0, 1, 0);
    g_cam.UpdateTPS(dt);

    // ---- 动画更新 ----
    if (g_playing && !g_anims.empty()) {
        g_animTime += dt * g_animSpeed;

        // 计算当前动画的骨骼矩阵
        Animator::ComputeBoneMatrices(g_skelModel, g_anims[g_currentAnim], g_animTime, g_boneMatrices);

        // 混合
        if (g_blendTimer > 0) {
            g_blendTimer -= dt;
            float blendT = 1.0f - std::max(0.0f, g_blendTimer / g_blendDuration);

            // 计算新动画的矩阵
            Animator::ComputeBoneMatrices(g_skelModel, g_anims[g_currentAnim], g_animTime, g_nextBoneMatrices);

            // 混合：从prevBoneMatrices过渡到nextBoneMatrices
            Animator::BlendBoneMatrices(g_prevBoneMatrices, g_nextBoneMatrices, blendT, g_boneMatrices);

            if (g_blendTimer <= 0) g_blendTimer = 0;
        }
    }
}

// ============================================================================
// 渲染
// ============================================================================
void Render() {
    glClearColor(g_bgColor.x, g_bgColor.y, g_bgColor.z, g_bgColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(g_wireframe)glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

    int ww,wh; glfwGetFramebufferSize(Engine::Instance().GetWindow(),&ww,&wh);
    float aspect=(ww>0&&wh>0)?(float)ww/(float)wh:1;
    Mat4 view = g_cam.GetViewMatrix();
    Mat4 proj = g_cam.GetProjectionMatrix(aspect);

    // ---- 渲染地面（用材质着色器） ----
    g_matShader->Bind();
    g_matShader->SetMat4("uView", view.m);
    g_matShader->SetMat4("uProjection", proj.m);
    g_matShader->SetVec3("uViewPos", g_cam.GetPosition());
    SetLightUniforms(g_matShader.get());

    for(auto&o:g_sceneObjs){
        Mat4 model=Mat4::Translate(o.pos.x,o.pos.y,o.pos.z)*Mat4::Scale(o.scl.x,o.scl.y,o.scl.z);
        g_matShader->SetMat4("uModel",model.m);
        Mat3 nm; nm.m[0]=nm.m[4]=nm.m[8]=1; nm.m[1]=nm.m[2]=nm.m[3]=nm.m[5]=nm.m[6]=nm.m[7]=0;
        g_matShader->SetMat3("uNormalMatrix",nm.m);
        bool hasMat=(o.matIdx>=0&&o.matIdx<(int)g_materials.size());
        if(hasMat){auto&m=g_materials[o.matIdx];m.Bind();
            g_matShader->SetBool("uUseDiffuseMap",m.hasDiffuse);g_matShader->SetBool("uUseNormalMap",m.hasNormal);
            g_matShader->SetBool("uUseRoughnessMap",m.hasRoughness);g_matShader->SetFloat("uTexTiling",o.tiling);}
        else{g_matShader->SetBool("uUseDiffuseMap",false);g_matShader->SetBool("uUseNormalMap",false);
            g_matShader->SetBool("uUseRoughnessMap",false);g_matShader->SetFloat("uTexTiling",1);}
        g_matShader->SetBool("uUseVertexColor",false);
        g_matShader->SetVec4("uBaseColor",Vec4(0.3f,0.3f,0.35f,1));
        g_matShader->SetFloat("uShininess",o.shininess);
        o.mesh.Draw();
    }

    // ---- 渲染角色（用蒙皮着色器） ----
    if (g_skelModel.IsValid()) {
        g_skinShader->Bind();
        g_skinShader->SetMat4("uView", view.m);
        g_skinShader->SetMat4("uProjection", proj.m);
        g_skinShader->SetVec3("uViewPos", g_cam.GetPosition());
        SetLightUniforms(g_skinShader.get());

        Mat4 model = Mat4::Translate(g_playerPos.x, g_playerPos.y, g_playerPos.z)
                   * Mat4::RotateY(g_playerYaw)
                   * Mat4::Scale(g_playerScale, g_playerScale, g_playerScale);
        g_skinShader->SetMat4("uModel", model.m);

        // NormalMatrix
        Mat3 nm; nm.m[0]=nm.m[4]=nm.m[8]=1; nm.m[1]=nm.m[2]=nm.m[3]=nm.m[5]=nm.m[6]=nm.m[7]=0;
        g_skinShader->SetMat3("uNormalMatrix", nm.m);

        // 骨骼矩阵
        g_skinShader->SetBool("uUseSkinning", true);
        for (int i = 0; i < MAX_BONES && i < (int)g_skelModel.bones.size(); i++) {
            std::string name = "uBones[" + std::to_string(i) + "]";
            g_skinShader->SetMat4(name, g_boneMatrices[i].m);
        }

        // 材质设置（角色无贴图，用顶点颜色）
        g_skinShader->SetBool("uUseDiffuseMap", false);
        g_skinShader->SetBool("uUseNormalMap", false);
        g_skinShader->SetBool("uUseRoughnessMap", false);
        g_skinShader->SetBool("uUseVertexColor", true);
        g_skinShader->SetFloat("uShininess", 32);
        g_skinShader->SetFloat("uTexTiling", 1);
        g_skinShader->SetVec4("uBaseColor", Vec4(0.5f, 0.5f, 0.6f, 1));

        g_skelModel.Draw();
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// ============================================================================
// ImGui
// ============================================================================
void RenderImGui() {
    // ---- HUD ----
    ImGui::SetNextWindowPos(ImVec2(10,10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::Begin("##hud",nullptr,
        ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|
        ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav);
    ImGui::Text("Ch25: Skeletal Animation");
    ImGui::Separator();
    if(!g_anims.empty()){
        ImGui::Text("Anim: %s", g_anims[g_currentAnim].name.c_str());
        float dur = g_anims[g_currentAnim].GetDurationSeconds();
        float curTime = (dur > 0) ? std::fmod(g_animTime * g_animSpeed, dur) : 0;
        ImGui::Text("Time: %.2f / %.2fs", curTime, dur);
    }
    ImGui::Text("Speed: %.1fx  %s", g_animSpeed, g_playing?"Playing":"Paused");
    ImGui::Text("Bones: %d  FPS: %.0f", (int)g_skelModel.bones.size(), g_fps);
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f,0.8f,1,1), "[1-4] Anim  [Space] Play/Pause");
    ImGui::TextColored(ImVec4(0.6f,0.8f,1,1), "[+/-] Speed  [B] Bones  [E] Editor");
    ImGui::TextColored(ImVec4(0.6f,0.8f,1,1), "WASD=Move  LMB=Rotate  Scroll=Zoom");
    ImGui::End();

    if(!g_showEditor) return;

    // ---- 编辑面板 ----
    ImGui::SetNextWindowPos(ImVec2(10,260), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340,500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Editor", &g_showEditor);

    if(ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)){
        
        ImGui::SliderFloat("Yaw",&g_cam.yaw,-180,180);
        ImGui::SliderFloat("Pitch",&g_cam.pitch,-85,85);
        ImGui::SliderFloat("Distance",&g_cam.orbitDistance, 1.0f, 50.0f);
        ImGui::SliderFloat("FOV",&g_cam.fov,20,120);
    }

    if(ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen)){
        // 动画列表
        for(int i=0;i<(int)g_anims.size();i++){
            bool sel=(i==g_currentAnim);
            if(ImGui::Selectable(g_anims[i].name.c_str(), sel))
                SwitchAnim(i);
        }
        ImGui::Separator();
        ImGui::SliderFloat("Speed", &g_animSpeed, 0.1f, 5.0f);
        ImGui::Checkbox("Playing", &g_playing);

        // 时间轴
        if(!g_anims.empty()){
            float dur = g_anims[g_currentAnim].GetDurationSeconds();
            float curTime = (dur > 0) ? std::fmod(g_animTime, dur) : 0;
            if(ImGui::SliderFloat("Time", &curTime, 0, dur))
                g_animTime = curTime;
        }

        ImGui::Separator();
        ImGui::Checkbox("Blend Transitions", &g_useBlend);
        ImGui::SliderFloat("Blend Duration", &g_blendDuration, 0.05f, 1.0f);
    }

    if(ImGui::CollapsingHeader("Objects")){
        ImGui::Text("Model: %s", g_skelModel.name.c_str());
        ImGui::Text("Meshes: %d  Bones: %d", (int)g_skelModel.meshes.size(), (int)g_skelModel.bones.size());
        ImGui::Text("Vertices: %d  Triangles: %d", g_skelModel.totalVertices, g_skelModel.totalTriangles);
        ImGui::Separator();
        ImGui::DragFloat3("Player Pos", &g_playerPos.x, 0.1f);
        float yawDeg = g_playerYaw * 57.3f;
        if(ImGui::SliderFloat("Player Yaw", &yawDeg, -180, 180)) g_playerYaw = yawDeg * 0.01745f;
        ImGui::DragFloat("Scale", &g_playerScale, 0.001f, 0.001f, 1.0f, "%.4f");
        ImGui::Separator();
        ImGui::Checkbox("Show Bones", &g_showBones);
        ImGui::Checkbox("Wireframe", &g_wireframe);
        ImGui::ColorEdit4("Background", &g_bgColor.x);
    }

    if(ImGui::CollapsingHeader("Lighting")){
        ImGui::DragFloat3("Dir Light Dir", &g_dirLight.dir.x, 0.01f);
        ImGui::ColorEdit3("Dir Light Color", &g_dirLight.color.x);
        ImGui::SliderFloat("Dir Intensity", &g_dirLight.intensity, 0, 2);
        ImGui::Separator();
        ImGui::DragFloat3("Pt Light Pos", &g_ptLight.pos.x, 0.1f);
        ImGui::ColorEdit3("Pt Light Color", &g_ptLight.color.x);
        ImGui::SliderFloat("Pt Intensity", &g_ptLight.intensity, 0, 3);
        ImGui::Separator();
        ImGui::SliderFloat("Ambient", &g_ambient, 0, 1);
    }

    ImGui::End();
}

// ============================================================================
// 清理 & 主函数
// ============================================================================
void Cleanup() {
    g_skelModel.Destroy();
    for(auto&o:g_sceneObjs) o.mesh.Destroy();
    for(auto&m:g_materials) m.Destroy();
    g_skinShader.reset();
    g_matShader.reset();
}

int main() {
    std::cout << "=== Example 25: Skeletal Animation ===" << std::endl;

    Engine& engine = Engine::Instance();
    EngineConfig cfg;
    cfg.windowWidth=1280; cfg.windowHeight=720;
    cfg.windowTitle="Mini Engine - Skeletal Animation";

    if(!engine.Initialize(cfg)) return -1;
    glfwSetScrollCallback(engine.GetWindow(), ScrollCB);

    if(!Initialize()){engine.Shutdown();return -1;}

    engine.SetUpdateCallback(Update);
    engine.SetRenderCallback(Render);
    engine.SetImGuiCallback(RenderImGui);
    engine.Run();

    Cleanup();
    engine.Shutdown();
    return 0;
}
