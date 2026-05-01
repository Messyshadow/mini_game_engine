#pragma once
#include "math/Math.h"
#include <imgui.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>

namespace MiniEngine {

using Math::Vec3;
using Math::Vec4;
using Math::Mat4;

struct DamageNumber {
    Vec3 worldPos;
    float damage;
    float life;
    float maxLife;
    bool critical;
    bool active;
};

class GameUI3D {
public:
    float hudBarWidth = 220;
    float hudBarHeight = 22;
    float hudX = 15;
    float hudY = 15;
    float mpBarHeight = 16;

    float enemyBarWidth = 70;
    float enemyBarHeight = 8;

    float floatSpeed = 2.0f;
    float floatLife = 1.2f;
    float normalFontScale = 1.0f;
    float critFontScale = 1.6f;

    void SpawnDamageNumber(Vec3 pos, float damage, bool critical = false) {
        for (auto& d : m_dmgNumbers) {
            if (!d.active) {
                d.worldPos = pos;
                d.damage = damage;
                d.life = floatLife;
                d.maxLife = floatLife;
                d.critical = critical;
                d.active = true;
                return;
            }
        }
        DamageNumber d;
        d.worldPos = pos;
        d.damage = damage;
        d.life = floatLife;
        d.maxLife = floatLife;
        d.critical = critical;
        d.active = true;
        m_dmgNumbers.push_back(d);
    }

    void Update(float dt) {
        for (auto& d : m_dmgNumbers) {
            if (!d.active) continue;
            d.worldPos.y += floatSpeed * dt;
            d.life -= dt;
            if (d.life <= 0) d.active = false;
        }
        if (m_tipTimer > 0) m_tipTimer -= dt;
    }

    void ShowTip(const std::string& text, float duration = 3.0f) {
        m_tipText = text;
        m_tipTimer = duration;
    }

    void RenderPlayerHUD(float hp, float maxHp, float mp, float maxMp, int screenW, int screenH) {
        float hpRatio = (maxHp > 0) ? hp / maxHp : 0;
        float mpRatio = (maxMp > 0) ? mp / maxMp : 0;

        ImU32 hpColor;
        if (hpRatio > 0.6f) hpColor = IM_COL32(50, 200, 50, 230);
        else if (hpRatio > 0.3f) hpColor = IM_COL32(220, 200, 30, 230);
        else hpColor = IM_COL32(220, 40, 40, 230);

        auto* dl = ImGui::GetForegroundDrawList();

        dl->AddRectFilled(ImVec2(hudX, hudY), ImVec2(hudX + hudBarWidth, hudY + hudBarHeight),
                          IM_COL32(30, 30, 30, 200), 3);
        dl->AddRectFilled(ImVec2(hudX, hudY), ImVec2(hudX + hudBarWidth * hpRatio, hudY + hudBarHeight),
                          hpColor, 3);
        dl->AddRect(ImVec2(hudX, hudY), ImVec2(hudX + hudBarWidth, hudY + hudBarHeight),
                    IM_COL32(180, 180, 180, 200), 3);

        char hpText[64];
        snprintf(hpText, sizeof(hpText), "HP %.0f / %.0f", hp, maxHp);
        dl->AddText(ImVec2(hudX + 6, hudY + 3), IM_COL32(255, 255, 255, 230), hpText);

        float mpY = hudY + hudBarHeight + 4;
        dl->AddRectFilled(ImVec2(hudX, mpY), ImVec2(hudX + hudBarWidth, mpY + mpBarHeight),
                          IM_COL32(30, 30, 30, 200), 3);
        dl->AddRectFilled(ImVec2(hudX, mpY), ImVec2(hudX + hudBarWidth * mpRatio, mpY + mpBarHeight),
                          IM_COL32(60, 120, 230, 230), 3);
        dl->AddRect(ImVec2(hudX, mpY), ImVec2(hudX + hudBarWidth, mpY + mpBarHeight),
                    IM_COL32(150, 150, 180, 200), 3);

        char mpText[64];
        snprintf(mpText, sizeof(mpText), "MP %.0f / %.0f", mp, maxMp);
        dl->AddText(ImVec2(hudX + 6, mpY + 1), IM_COL32(220, 220, 255, 230), mpText);
    }

    void RenderEnemyBar(Vec3 worldPos, float hp, float maxHp, const char* name, bool isHit,
                        const Mat4& vp, int screenW, int screenH) {
        Vec4 clip(
            vp.m[0]*worldPos.x + vp.m[4]*worldPos.y + vp.m[8]*worldPos.z  + vp.m[12],
            vp.m[1]*worldPos.x + vp.m[5]*worldPos.y + vp.m[9]*worldPos.z  + vp.m[13],
            vp.m[2]*worldPos.x + vp.m[6]*worldPos.y + vp.m[10]*worldPos.z + vp.m[14],
            vp.m[3]*worldPos.x + vp.m[7]*worldPos.y + vp.m[11]*worldPos.z + vp.m[15]
        );
        if (clip.w < 0.01f) return;
        float sx = (clip.x / clip.w * 0.5f + 0.5f) * screenW;
        float sy = (1.0f - (clip.y / clip.w * 0.5f + 0.5f)) * screenH;

        float hpRatio = (maxHp > 0) ? hp / maxHp : 0;
        auto* dl = ImGui::GetForegroundDrawList();

        dl->AddRectFilled(ImVec2(sx - enemyBarWidth/2, sy),
                          ImVec2(sx + enemyBarWidth/2, sy + enemyBarHeight),
                          IM_COL32(40, 40, 40, 200));
        ImU32 barCol = isHit ? IM_COL32(255, 100, 100, 255) : IM_COL32(220, 50, 50, 230);
        dl->AddRectFilled(ImVec2(sx - enemyBarWidth/2, sy),
                          ImVec2(sx - enemyBarWidth/2 + enemyBarWidth * hpRatio, sy + enemyBarHeight),
                          barCol);
        dl->AddRect(ImVec2(sx - enemyBarWidth/2, sy),
                    ImVec2(sx + enemyBarWidth/2, sy + enemyBarHeight),
                    IM_COL32(200, 200, 200, 180));

        char hpLabel[64];
        snprintf(hpLabel, sizeof(hpLabel), "%s %.0f/%.0f", name, hp, maxHp);
        dl->AddText(ImVec2(sx - 50, sy - 16), IM_COL32(255, 255, 255, 220), hpLabel);
    }

    void RenderDamageNumbers(const Mat4& vp, int screenW, int screenH) {
        auto* dl = ImGui::GetForegroundDrawList();
        for (auto& d : m_dmgNumbers) {
            if (!d.active) continue;
            Vec4 clip(
                vp.m[0]*d.worldPos.x + vp.m[4]*d.worldPos.y + vp.m[8]*d.worldPos.z  + vp.m[12],
                vp.m[1]*d.worldPos.x + vp.m[5]*d.worldPos.y + vp.m[9]*d.worldPos.z  + vp.m[13],
                vp.m[2]*d.worldPos.x + vp.m[6]*d.worldPos.y + vp.m[10]*d.worldPos.z + vp.m[14],
                vp.m[3]*d.worldPos.x + vp.m[7]*d.worldPos.y + vp.m[11]*d.worldPos.z + vp.m[15]
            );
            if (clip.w < 0.01f) continue;
            float sx = (clip.x / clip.w * 0.5f + 0.5f) * screenW;
            float sy = (1.0f - (clip.y / clip.w * 0.5f + 0.5f)) * screenH;

            float alpha = std::min(1.0f, d.life / (d.maxLife * 0.3f));
            int a = (int)(alpha * 255);

            char buf[32];
            snprintf(buf, sizeof(buf), "%.0f", d.damage);

            if (d.critical) {
                float scale = critFontScale;
                ImGui::SetWindowFontScale(scale);
                ImVec2 tsz = ImGui::CalcTextSize(buf);
                dl->AddText(ImVec2(sx - tsz.x/2 + 1, sy + 1), IM_COL32(0, 0, 0, a), buf);
                dl->AddText(ImVec2(sx - tsz.x/2, sy), IM_COL32(255, 220, 50, a), buf);
                ImGui::SetWindowFontScale(1.0f);
            } else {
                ImVec2 tsz = ImGui::CalcTextSize(buf);
                dl->AddText(ImVec2(sx - tsz.x/2 + 1, sy + 1), IM_COL32(0, 0, 0, a), buf);
                dl->AddText(ImVec2(sx - tsz.x/2, sy), IM_COL32(255, 255, 255, a), buf);
            }
        }
    }

    void RenderTip(int screenW, int screenH) {
        if (m_tipTimer <= 0 || m_tipText.empty()) return;
        auto* dl = ImGui::GetForegroundDrawList();
        ImVec2 tsz = ImGui::CalcTextSize(m_tipText.c_str());
        float tx = ((float)screenW - tsz.x) * 0.5f;
        float ty = (float)screenH * 0.3f;
        float alpha = std::min(1.0f, m_tipTimer) * 255;
        dl->AddRectFilled(ImVec2(tx - 12, ty - 6), ImVec2(tx + tsz.x + 12, ty + tsz.y + 6),
                          IM_COL32(0, 0, 0, (int)(alpha * 0.6f)), 5);
        dl->AddText(ImVec2(tx, ty), IM_COL32(255, 255, 200, (int)alpha), m_tipText.c_str());
    }

private:
    std::vector<DamageNumber> m_dmgNumbers;
    std::string m_tipText;
    float m_tipTimer = 0;
};

} // namespace MiniEngine
