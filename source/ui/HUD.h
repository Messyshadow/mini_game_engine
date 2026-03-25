/**
 * HUD.h - 游戏UI系统
 *
 * 功能：
 * 1. HP血条 + MP能量条（屏幕左下角）
 * 2. 伤害飘字（从3D世界坐标弹出）
 * 3. 房间名称提示（进入新房间时淡入淡出）
 * 4. 技能提示栏
 */

#pragma once

#include "math/Math.h"
#include "engine/SpriteRenderer.h"
#include <vector>
#include <string>

namespace MiniEngine {

using Math::Vec2;
using Math::Vec4;
using Math::Mat4;

// ============================================================================
// 伤害飘字
// ============================================================================

struct DamagePopup {
    Vec2 worldPos;      // 世界坐标位置
    int damage;         // 伤害数值
    float lifetime;     // 剩余存活时间
    float maxLife;      // 最大存活时间
    Vec4 color;         // 颜色
    float velY;         // 上飘速度
    bool critical;      // 是否暴击
    
    DamagePopup(Vec2 pos, int dmg, bool crit = false)
        : worldPos(pos), damage(dmg), lifetime(1.2f), maxLife(1.2f),
          velY(80.0f), critical(crit) {
        color = crit ? Vec4(1, 0.8f, 0, 1) : Vec4(1, 1, 1, 1);
    }
};

// ============================================================================
// 房间名称提示
// ============================================================================

struct RoomTitle {
    std::string text;
    float timer = 0;
    float duration = 2.5f;
    float fadeIn = 0.5f;
    float fadeOut = 0.5f;
    bool active = false;
    
    void Show(const std::string& name, float dur = 2.5f) {
        text = name; timer = 0; duration = dur; active = true;
    }
    
    float GetAlpha() const {
        if (!active) return 0;
        if (timer < fadeIn) return timer / fadeIn;
        if (timer > duration - fadeOut) return (duration - timer) / fadeOut;
        return 1.0f;
    }
};

// ============================================================================
// HUD主类
// ============================================================================

class HUD {
public:
    // 玩家状态（外部每帧设置）
    int playerHP = 100, playerMaxHP = 100;
    int playerMP = 50, playerMaxMP = 50;
    
    // 技能冷却显示
    float teleportCooldown = 0;
    float teleportMaxCooldown = 3.0f;
    float skillCooldown = 0;
    float skillMaxCooldown = 4.0f;
    
    // 武器信息
    int currentWeapon = 0;
    bool hasWeapon[3] = {true, false, false};
    
    // 暂停状态
    bool isPaused = false;
    
    // 房间名称提示
    RoomTitle roomTitle;
    
    // ========================================================================
    // 更新
    // ========================================================================
    
    void Update(float dt) {
        // 更新伤害飘字
        for (auto& p : m_popups) {
            p.lifetime -= dt;
            p.worldPos.y += p.velY * dt;
            p.velY *= 0.95f;  // 减速
        }
        // 移除过期的
        m_popups.erase(
            std::remove_if(m_popups.begin(), m_popups.end(),
                [](const DamagePopup& p) { return p.lifetime <= 0; }),
            m_popups.end());
        
        // 更新房间标题
        if (roomTitle.active) {
            roomTitle.timer += dt;
            if (roomTitle.timer >= roomTitle.duration) roomTitle.active = false;
        }
    }
    
    // ========================================================================
    // 添加伤害飘字
    // ========================================================================
    
    void AddDamagePopup(Vec2 worldPos, int damage, bool critical = false) {
        // 稍微随机偏移避免重叠
        float offX = ((float)(rand() % 40) - 20);
        worldPos.x += offX;
        worldPos.y += 30;
        m_popups.emplace_back(worldPos, damage, critical);
    }
    
    // ========================================================================
    // 渲染HUD（使用屏幕空间正交投影，不跟随摄像机）
    // ========================================================================
    
    void Render(SpriteRenderer* renderer, int screenW, int screenH,
                const Mat4& worldProjView = Mat4::Identity()) {
        // HUD用屏幕空间投影
        Mat4 hudProj = Mat4::Ortho(0, (float)screenW, 0, (float)screenH, -1, 1);
        renderer->SetProjection(hudProj);
        renderer->Begin();
        
        // ---- HP条（左下角）----
        float barX = 30, barY = screenH - 40.f;
        float barW = 200, barH = 18;
        
        // HP背景
        renderer->DrawRect(Vec2(barX + barW * 0.5f, barY), Vec2(barW + 4, barH + 4),
                          Vec4(0.15f, 0.15f, 0.15f, 0.9f));
        // HP条
        float hpPct = playerMaxHP > 0 ? (float)playerHP / playerMaxHP : 0;
        Vec4 hpColor = hpPct > 0.5f ? Vec4(0.1f, 0.85f, 0.2f, 1) :
                       hpPct > 0.25f ? Vec4(0.9f, 0.8f, 0.1f, 1) :
                       Vec4(0.9f, 0.15f, 0.1f, 1);
        if (hpPct > 0) {
            renderer->DrawRect(Vec2(barX + barW * hpPct * 0.5f, barY),
                              Vec2(barW * hpPct, barH), hpColor);
        }
        // HP标签装饰线
        renderer->DrawRect(Vec2(barX - 4, barY), Vec2(4, barH + 8),
                          Vec4(0.9f, 0.2f, 0.2f, 1));
        
        // ---- MP条（HP下方）----
        float mpY = barY - 24;
        float mpW = 160, mpH = 12;
        
        // MP背景
        renderer->DrawRect(Vec2(barX + mpW * 0.5f, mpY), Vec2(mpW + 4, mpH + 4),
                          Vec4(0.15f, 0.15f, 0.15f, 0.9f));
        // MP条
        float mpPct = playerMaxMP > 0 ? (float)playerMP / playerMaxMP : 0;
        if (mpPct > 0) {
            renderer->DrawRect(Vec2(barX + mpW * mpPct * 0.5f, mpY),
                              Vec2(mpW * mpPct, mpH), Vec4(0.2f, 0.4f, 0.95f, 1));
        }
        // MP标签装饰线
        renderer->DrawRect(Vec2(barX - 4, mpY), Vec2(4, mpH + 8),
                          Vec4(0.2f, 0.4f, 0.95f, 1));
        
        // ---- 技能栏（MP下方）— 武器槽 + 瞬移[K] + 技能[L] ----
        float skillY = mpY - 32;
        float slotSize = 28;
        float slotGap = 4;
        
        // 武器槽（3个，当前武器高亮）
        Vec4 wColors[] = {Vec4(0.9f,0.9f,0.9f,1), Vec4(0.3f,0.7f,1,1), Vec4(0.9f,0.5f,0.1f,1)};
        for (int i = 0; i < 3; i++) {
            float sx = barX + i * (slotSize + slotGap) + slotSize * 0.5f;
            if (!hasWeapon[i]) {
                renderer->DrawRect(Vec2(sx, skillY), Vec2(slotSize, slotSize), Vec4(0.15f,0.15f,0.15f,0.5f));
            } else if (i == currentWeapon) {
                renderer->DrawRect(Vec2(sx, skillY), Vec2(slotSize+4, slotSize+4), Vec4(1,1,1,0.3f));
                renderer->DrawRect(Vec2(sx, skillY), Vec2(slotSize, slotSize), wColors[i] * Vec4(1,1,1,0.9f));
            } else {
                renderer->DrawRect(Vec2(sx, skillY), Vec2(slotSize, slotSize), wColors[i] * Vec4(1,1,1,0.4f));
            }
        }
        
        // 瞬移[K]图标
        float kX = barX + 3 * (slotSize + slotGap) + slotSize * 0.5f + 8;
        {
            Vec4 kBg = teleportCooldown > 0 ? Vec4(0.3f,0.3f,0.3f,0.7f) : Vec4(0.2f,0.6f,0.9f,0.8f);
            renderer->DrawRect(Vec2(kX, skillY), Vec2(slotSize, slotSize), kBg);
            if (teleportCooldown > 0 && teleportMaxCooldown > 0) {
                float cdPct = teleportCooldown / teleportMaxCooldown;
                renderer->DrawRect(Vec2(kX, skillY - slotSize*0.5f*(1-cdPct)), Vec2(slotSize, slotSize*cdPct), Vec4(0,0,0,0.6f));
            }
            renderer->DrawRect(Vec2(kX, skillY+slotSize*0.5f), Vec2(slotSize, 2), Vec4(0.5f,0.7f,1,0.6f));
            renderer->DrawRect(Vec2(kX, skillY-slotSize*0.5f), Vec2(slotSize, 2), Vec4(0.5f,0.7f,1,0.6f));
        }
        
        // 技能[L]图标
        float lX = kX + slotSize + slotGap + 4;
        {
            Vec4 lBg = skillCooldown > 0 ? Vec4(0.3f,0.3f,0.3f,0.7f) : Vec4(0.8f,0.4f,0.1f,0.8f);
            renderer->DrawRect(Vec2(lX, skillY), Vec2(slotSize, slotSize), lBg);
            if (skillCooldown > 0 && skillMaxCooldown > 0) {
                float cdPct = skillCooldown / skillMaxCooldown;
                renderer->DrawRect(Vec2(lX, skillY - slotSize*0.5f*(1-cdPct)), Vec2(slotSize, slotSize*cdPct), Vec4(0,0,0,0.6f));
            }
            renderer->DrawRect(Vec2(lX, skillY+slotSize*0.5f), Vec2(slotSize, 2), Vec4(1,0.6f,0.2f,0.6f));
            renderer->DrawRect(Vec2(lX, skillY-slotSize*0.5f), Vec2(slotSize, 2), Vec4(1,0.6f,0.2f,0.6f));
        }
        
        // ---- 房间名称提示（屏幕上方居中）----
        if (roomTitle.active) {
            float alpha = roomTitle.GetAlpha();
            float titleW = 300, titleH = 36;
            float titleY = screenH - 80.f;
            renderer->DrawRect(Vec2(screenW * 0.5f, titleY),
                              Vec2(titleW, titleH), Vec4(0, 0, 0, 0.6f * alpha));
            // 装饰横线
            renderer->DrawRect(Vec2(screenW * 0.5f, titleY + titleH * 0.5f + 2),
                              Vec2(titleW * 0.8f, 2), Vec4(0.8f, 0.7f, 0.3f, alpha));
            renderer->DrawRect(Vec2(screenW * 0.5f, titleY - titleH * 0.5f - 2),
                              Vec2(titleW * 0.8f, 2), Vec4(0.8f, 0.7f, 0.3f, alpha));
        }
        
        // ---- 暂停菜单遮罩 ----
        if (isPaused) {
            renderer->DrawRect(Vec2(screenW * 0.5f, screenH * 0.5f),
                              Vec2((float)screenW, (float)screenH),
                              Vec4(0, 0, 0, 0.6f));
            // PAUSED文字框
            renderer->DrawRect(Vec2(screenW * 0.5f, screenH * 0.5f),
                              Vec2(200, 60), Vec4(0.15f, 0.15f, 0.2f, 0.95f));
            renderer->DrawRect(Vec2(screenW * 0.5f, screenH * 0.5f + 32),
                              Vec2(180, 3), Vec4(0.8f, 0.7f, 0.3f, 1));
            renderer->DrawRect(Vec2(screenW * 0.5f, screenH * 0.5f - 32),
                              Vec2(180, 3), Vec4(0.8f, 0.7f, 0.3f, 1));
        }
        
        renderer->End();
        
        // ---- 伤害飘字（世界空间，需要跟随摄像机）----
        if (!m_popups.empty()) {
            renderer->SetProjection(worldProjView);
            renderer->Begin();
            for (auto& p : m_popups) {
                float alpha = std::min(1.0f, p.lifetime / (p.maxLife * 0.3f));
                float scale = p.critical ? 14.0f : 10.0f;
                // 用矩形近似显示伤害数值的每一位
                int num = p.damage;
                int digits[5]; int nDigits = 0;
                if (num == 0) { digits[0] = 0; nDigits = 1; }
                else { while (num > 0 && nDigits < 5) { digits[nDigits++] = num % 10; num /= 10; } }
                
                float totalW = nDigits * (scale + 2);
                float startX = p.worldPos.x - totalW * 0.5f;
                
                Vec4 col = p.color;
                col.w = alpha;
                
                for (int i = nDigits - 1; i >= 0; i--) {
                    float dx = startX + (nDigits - 1 - i) * (scale + 2);
                    // 用小方块表示数字（简化版，后续可替换为字体纹理）
                    renderer->DrawRect(Vec2(dx + scale * 0.5f, p.worldPos.y),
                                      Vec2(scale * 0.8f, scale * 1.2f), col);
                }
            }
            renderer->End();
        }
    }
    
private:
    std::vector<DamagePopup> m_popups;
};

} // namespace MiniEngine
