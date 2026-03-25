/**
 * LevelEditor.h - 关卡编辑模式
 *
 * 按E键切换。提供ImGui面板编辑：
 * - 物理参数（速度、跳跃、重力等）
 * - 角色属性（HP/MP/攻击力）
 * - 敌人编辑（添加/删除/修改）
 * - 平台编辑（鼠标拖动/放置/删除）
 * - 特效参数
 */

#pragma once

#include "game/PlatformController.h"
#include "scene/RoomSystem.h"
#include "ai/EnemyAI.h"
#include <imgui.h>
#include <vector>
#include <string>
#include <functional>

namespace MiniEngine {

class LevelEditor {
public:
    bool active = false;
    
    // 引用外部数据（由main设置）
    PlatformParams* playerParams = nullptr;
    int* playerHP = nullptr; int* playerMaxHP = nullptr;
    int* playerMP = nullptr; int* playerMaxMP = nullptr;
    int* playerAtk = nullptr;
    float* teleportCD = nullptr; int* teleportCost = nullptr;
    float* skillCD = nullptr; int* skillCost = nullptr;
    
    // 能力开关指针
    bool* hasDoubleJump = nullptr;
    bool* hasWallJump = nullptr;
    bool* hasTeleport = nullptr;
    bool* hasWeapons = nullptr; // 指向bool[3]数组
    
    // 平台编辑
    struct EditPlatform {
        int x, y, w, h, tileID;
        bool selected = false;
    };
    std::vector<EditPlatform> platforms;
    
    // 敌人编辑
    struct EditEnemy {
        Vec2 pos; int type; int hp, atk;
        float patrolMin, patrolMax;
        bool selected = false;
    };
    std::vector<EditEnemy> enemies;
    
    // 回调
    std::function<void()> onRebuildLevel;  // 编辑后重建关卡
    
    // 鼠标状态
    enum class MouseMode { Select, PlacePlatform, PlaceEnemy };
    MouseMode mouseMode = MouseMode::Select;
    int placeTileID = 9;
    int placeEnemyType = 0;
    
    void RenderPanel() {
        if (!active) return;
        
        ImGui::Begin("Level Editor [E to close]", &active, ImGuiWindowFlags_AlwaysAutoResize);
        
        if (ImGui::BeginTabBar("EditorTabs")) {
            // ---- 物理参数 ----
            if (ImGui::BeginTabItem("Physics")) {
                if (playerParams) {
                    ImGui::Text("=== Movement ===");
                    ImGui::SliderFloat("Move Speed", &playerParams->moveSpeed, 100, 500);
                    ImGui::SliderFloat("Acceleration", &playerParams->acceleration, 500, 4000);
                    ImGui::SliderFloat("Deceleration", &playerParams->deceleration, 500, 4000);
                    ImGui::SliderFloat("Air Accel", &playerParams->airAccel, 200, 2000);
                    
                    ImGui::Separator();
                    ImGui::Text("=== Jump ===");
                    ImGui::SliderFloat("Jump Force", &playerParams->jumpForce, 200, 800);
                    ImGui::SliderFloat("Jump Cut", &playerParams->jumpCutMultiplier, 0.1f, 1.0f);
                    ImGui::SliderFloat("Air Jump Force", &playerParams->airJumpForce, 200, 700);
                    
                    ImGui::Separator();
                    ImGui::Text("=== Gravity ===");
                    ImGui::SliderFloat("Gravity", &playerParams->gravity, 500, 3000);
                    ImGui::SliderFloat("Fall Gravity x", &playerParams->fallGravityMult, 1.0f, 3.0f);
                    ImGui::SliderFloat("Max Fall Speed", &playerParams->maxFallSpeed, 300, 1200);
                    
                    ImGui::Separator();
                    ImGui::Text("=== Advanced ===");
                    ImGui::Checkbox("Coyote Time", &playerParams->enableCoyoteTime);
                    if (playerParams->enableCoyoteTime)
                        ImGui::SliderFloat("Coyote Duration", &playerParams->coyoteTime, 0.02f, 0.3f);
                    ImGui::Checkbox("Jump Buffer", &playerParams->enableJumpBuffer);
                    if (playerParams->enableJumpBuffer)
                        ImGui::SliderFloat("Buffer Duration", &playerParams->jumpBufferTime, 0.02f, 0.3f);
                    ImGui::Checkbox("Double Jump", &playerParams->enableDoubleJump);
                    ImGui::Checkbox("Wall Jump", &playerParams->enableWallJump);
                    if (playerParams->enableWallJump) {
                        ImGui::SliderFloat("Wall Slide Speed", &playerParams->wallSlideSpeed, 20, 200);
                        ImGui::SliderFloat("Wall Jump X", &playerParams->wallJumpForceX, 100, 500);
                        ImGui::SliderFloat("Wall Jump Y", &playerParams->wallJumpForceY, 200, 700);
                    }
                }
                ImGui::EndTabItem();
            }
            
            // ---- 角色属性 ----
            if (ImGui::BeginTabItem("Player")) {
                ImGui::Text("=== HP / MP ===");
                if (playerHP && playerMaxHP) {
                    ImGui::SliderInt("HP", playerHP, 1, *playerMaxHP);
                    ImGui::SliderInt("Max HP", playerMaxHP, 50, 500);
                    if (ImGui::Button("Full Heal")) { *playerHP = *playerMaxHP; }
                }
                if (playerMP && playerMaxMP) {
                    ImGui::SliderInt("MP", playerMP, 0, *playerMaxMP);
                    ImGui::SliderInt("Max MP", playerMaxMP, 20, 200);
                    ImGui::SameLine();
                    if (ImGui::Button("Full MP")) { *playerMP = *playerMaxMP; }
                }
                
                ImGui::Separator();
                ImGui::Text("=== Combat ===");
                if (playerAtk) ImGui::SliderInt("Attack", playerAtk, 1, 50);
                if (teleportCD) ImGui::SliderFloat("Teleport CD", teleportCD, 0.5f, 10);
                if (teleportCost) ImGui::SliderInt("Teleport MP Cost", teleportCost, 5, 50);
                if (skillCD) ImGui::SliderFloat("Skill CD", skillCD, 0.5f, 10);
                if (skillCost) ImGui::SliderInt("Skill MP Cost", skillCost, 5, 50);
                
                ImGui::Separator();
                ImGui::Text("=== Abilities (override pickup) ===");
                if (hasDoubleJump) ImGui::Checkbox("Double Jump", hasDoubleJump);
                if (hasWallJump) ImGui::Checkbox("Wall Jump", hasWallJump);
                if (hasTeleport) ImGui::Checkbox("Teleport (K)", hasTeleport);
                
                ImGui::Separator();
                ImGui::Text("=== Weapons ===");
                if (hasWeapons) {
                    ImGui::Checkbox("Fist (default)", &hasWeapons[0]);
                    ImGui::Checkbox("Sword", &hasWeapons[1]);
                    ImGui::Checkbox("Hammer", &hasWeapons[2]);
                }
                
                ImGui::EndTabItem();
            }
            
            // ---- 敌人编辑 ----
            if (ImGui::BeginTabItem("Enemies")) {
                ImGui::Text("Enemies: %d", (int)enemies.size());
                ImGui::Separator();
                
                for (int i = 0; i < (int)enemies.size(); i++) {
                    auto& e = enemies[i];
                    ImGui::PushID(i);
                    const char* types[] = {"Melee", "Ranged", "Hybrid"};
                    bool open = ImGui::TreeNode("##e", "[%d] %s HP:%d", i, types[e.type], e.hp);
                    if (open) {
                        ImGui::Combo("Type", &e.type, types, 3);
                        ImGui::DragFloat2("Position", &e.pos.x, 1.0f);
                        ImGui::SliderInt("HP", &e.hp, 10, 500);
                        ImGui::SliderInt("Attack", &e.atk, 5, 50);
                        ImGui::DragFloat("Patrol Min", &e.patrolMin, 1.0f);
                        ImGui::DragFloat("Patrol Max", &e.patrolMax, 1.0f);
                        if (ImGui::Button("Delete")) {
                            enemies.erase(enemies.begin() + i);
                            ImGui::TreePop(); ImGui::PopID();
                            break;
                        }
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                
                ImGui::Separator();
                if (ImGui::Button("+ Add Melee Enemy")) {
                    enemies.push_back({Vec2(400, 86), 0, 40, 8, 300, 600});
                }
                ImGui::SameLine();
                if (ImGui::Button("+ Add Ranged")) {
                    enemies.push_back({Vec2(500, 86), 1, 30, 12, 400, 700});
                }
                ImGui::SameLine();
                if (ImGui::Button("+ Add Hybrid")) {
                    enemies.push_back({Vec2(600, 86), 2, 60, 15, 400, 800});
                }
                
                if (ImGui::Button("Apply Changes")) {
                    if (onRebuildLevel) onRebuildLevel();
                }
                ImGui::EndTabItem();
            }
            
            // ---- 关卡编辑 ----
            if (ImGui::BeginTabItem("Level")) {
                ImGui::Text("Platforms: %d", (int)platforms.size());
                ImGui::Separator();
                
                for (int i = 0; i < (int)platforms.size(); i++) {
                    auto& p = platforms[i];
                    ImGui::PushID(i + 1000);
                    bool open = ImGui::TreeNode("##p", "[%d] (%d,%d) %dx%d tile:%d", i, p.x, p.y, p.w, p.h, p.tileID);
                    if (open) {
                        ImGui::DragInt("X", &p.x); ImGui::DragInt("Y", &p.y);
                        ImGui::DragInt("Width", &p.w, 1, 1, 80);
                        ImGui::DragInt("Height", &p.h, 1, 1, 20);
                        ImGui::SliderInt("Tile ID", &p.tileID, 1, 40);
                        if (ImGui::Button("Delete")) {
                            platforms.erase(platforms.begin() + i);
                            ImGui::TreePop(); ImGui::PopID(); break;
                        }
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                
                ImGui::Separator();
                static int newX=10, newY=5, newW=6, newH=1, newTile=9;
                ImGui::DragInt("New X", &newX); ImGui::SameLine(); ImGui::DragInt("New Y", &newY);
                ImGui::DragInt("New W", &newW); ImGui::SameLine(); ImGui::DragInt("New H", &newH);
                ImGui::SliderInt("New Tile", &newTile, 1, 40);
                if (ImGui::Button("+ Add Platform")) {
                    platforms.push_back({newX, newY, newW, newH, newTile});
                }
                
                ImGui::Separator();
                if (ImGui::Button("Rebuild Level")) {
                    if (onRebuildLevel) onRebuildLevel();
                }
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
};

} // namespace MiniEngine
