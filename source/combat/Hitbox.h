#pragma once

#include "physics/AABB.h"
#include "math/Math.h"
#include <vector>
#include <functional>

namespace MiniEngine {

using Math::Vec2;

/**
 * Hitbox类型
 */
enum class HitboxType {
    Hurtbox,    // 受击框（被动，总是检测）
    Hitbox,     // 攻击框（主动，仅攻击时激活）
    Trigger     // 触发器（进入/离开检测）
};

/**
 * Hitbox - 攻击/受击碰撞框
 * 
 * 相对于所属实体的局部坐标系
 */
class Hitbox {
public:
    HitboxType type = HitboxType::Hurtbox;
    Vec2 offset;        // 相对于实体中心的偏移
    Vec2 size;          // 尺寸
    bool active = true; // 是否激活
    bool flipWithOwner = true; // 是否随主人翻转
    
    // 用于识别
    int layer = 0;      // 碰撞层
    int id = 0;         // Hitbox ID（用于区分多个hitbox）
    
    Hitbox() = default;
    
    Hitbox(HitboxType t, const Vec2& off, const Vec2& sz)
        : type(t), offset(off), size(sz) {}
    
    /**
     * 获取世界坐标AABB
     * @param ownerPos 所属实体位置
     * @param facingRight 实体是否朝右
     */
    AABB GetWorldAABB(const Vec2& ownerPos, bool facingRight = true) const {
        Vec2 actualOffset = offset;
        if (flipWithOwner && !facingRight) {
            actualOffset.x = -actualOffset.x;
        }
        
        Vec2 center = ownerPos + actualOffset;
        Vec2 halfSize = size * 0.5f;
        
        return AABB(
            center.x - halfSize.x,
            center.y - halfSize.y,
            center.x + halfSize.x,
            center.y + halfSize.y
        );
    }
    
    /**
     * 检测与另一个Hitbox是否碰撞
     */
    bool Overlaps(const Hitbox& other, 
                  const Vec2& myPos, bool myFacingRight,
                  const Vec2& otherPos, bool otherFacingRight) const {
        if (!active || !other.active) return false;
        
        AABB myAABB = GetWorldAABB(myPos, myFacingRight);
        AABB otherAABB = other.GetWorldAABB(otherPos, otherFacingRight);
        
        return myAABB.Intersects(otherAABB);
    }
};

/**
 * Hitbox集合 - 管理一个实体的多个Hitbox
 */
class HitboxSet {
public:
    std::vector<Hitbox> boxes;
    
    void AddHurtbox(const Vec2& offset, const Vec2& size, int layer = 0) {
        Hitbox hb(HitboxType::Hurtbox, offset, size);
        hb.layer = layer;
        hb.id = (int)boxes.size();
        boxes.push_back(hb);
    }
    
    void AddHitbox(const Vec2& offset, const Vec2& size, int layer = 0) {
        Hitbox hb(HitboxType::Hitbox, offset, size);
        hb.layer = layer;
        hb.id = (int)boxes.size();
        hb.active = false; // 攻击框默认不激活
        boxes.push_back(hb);
    }
    
    void ActivateHitboxes(bool active) {
        for (auto& box : boxes) {
            if (box.type == HitboxType::Hitbox) {
                box.active = active;
            }
        }
    }
    
    void SetAllActive(bool active) {
        for (auto& box : boxes) {
            box.active = active;
        }
    }
    
    /**
     * 获取所有激活的指定类型hitbox
     */
    std::vector<Hitbox*> GetActiveBoxes(HitboxType type) {
        std::vector<Hitbox*> result;
        for (auto& box : boxes) {
            if (box.type == type && box.active) {
                result.push_back(&box);
            }
        }
        return result;
    }
    
    /**
     * 检测我的Hitbox是否命中目标的Hurtbox
     */
    bool CheckHit(const HitboxSet& target,
                  const Vec2& myPos, bool myFacingRight,
                  const Vec2& targetPos, bool targetFacingRight) const {
        for (const auto& myBox : boxes) {
            if (myBox.type != HitboxType::Hitbox || !myBox.active) continue;
            
            for (const auto& targetBox : target.boxes) {
                if (targetBox.type != HitboxType::Hurtbox || !targetBox.active) continue;
                
                if (myBox.Overlaps(targetBox, myPos, myFacingRight, targetPos, targetFacingRight)) {
                    return true;
                }
            }
        }
        return false;
    }
};

} // namespace MiniEngine
