#pragma once

#include "math/Math.h"
#include <string>
#include <vector>
#include <cmath>
#include <functional>

namespace MiniEngine {

using Math::Vec3;

struct Trigger3D {
    Vec3 position;
    Vec3 halfSize;
    std::string action;
    bool triggered = false;
    bool oneShot = true;
    std::string message;

    bool Contains(Vec3 point) const {
        return std::abs(point.x - position.x) < halfSize.x &&
               std::abs(point.y - position.y) < halfSize.y &&
               std::abs(point.z - position.z) < halfSize.z;
    }

    void Reset() { triggered = false; }
};

struct SceneConfig {
    std::string name;
    Vec3 playerSpawn = Vec3(0, 0.9f, 0);
    Vec3 skyTop    = Vec3(0.2f, 0.4f, 0.8f);
    Vec3 skyBottom = Vec3(0.8f, 0.85f, 0.9f);
    Vec3 skyHorizon = Vec3(0.9f, 0.9f, 0.95f);
    float ambient = 0.25f;
    Vec3 sunDir   = Vec3(0.4f, 0.7f, 0.3f);
    Vec3 sunColor = Vec3(1, 0.95f, 0.9f);
    float sunIntensity = 1.2f;
};

struct FadeTransition {
    float timer = 0;
    float duration = 1.0f;
    bool active = false;
    bool fadingOut = true;
    int targetScene = -1;

    void Start(int scene, float dur = 1.0f) {
        active = true;
        fadingOut = true;
        timer = 0;
        duration = dur;
        targetScene = scene;
    }

    void Update(float dt) {
        if (!active) return;
        timer += dt;
        if (fadingOut && timer >= duration * 0.5f) {
            fadingOut = false;
        }
        if (timer >= duration) {
            active = false;
            timer = 0;
        }
    }

    bool ShouldSwitch() const {
        return active && !fadingOut && timer < duration * 0.5f + 0.05f;
    }

    float GetAlpha() const {
        if (!active) return 0;
        float half = duration * 0.5f;
        if (fadingOut) return timer / half;
        return 1.0f - (timer - half) / half;
    }
};

} // namespace MiniEngine
