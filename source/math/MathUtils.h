/**
 * MathUtils.h - 数学工具函数
 */

#ifndef MINI_ENGINE_MATH_UTILS_H
#define MINI_ENGINE_MATH_UTILS_H

#include <cmath>
#include <algorithm>

namespace MiniEngine {
namespace Math {

// 常量
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = PI * 2.0f;
constexpr float HALF_PI = PI / 2.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float EPSILON = 1e-6f;

// 角度转换
inline float ToRadians(float degrees) { return degrees * DEG_TO_RAD; }
inline float ToDegrees(float radians) { return radians * RAD_TO_DEG; }

// 数值限制
template<typename T>
inline T Clamp(T value, T minVal, T maxVal) { 
    return (std::max)(minVal, (std::min)(maxVal, value)); 
}

// 插值
inline float Lerp(float a, float b, float t) { return a + t * (b - a); }
inline float InverseLerp(float a, float b, float value) {
    if (std::abs(b - a) < EPSILON) return 0.0f;
    return (value - a) / (b - a);
}

// 平滑插值
inline float SmoothStep(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float SmootherStep(float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// 浮点数比较
inline bool ApproximatelyEqual(float a, float b, float epsilon = EPSILON) {
    return std::abs(a - b) < epsilon;
}

inline bool IsNearZero(float value, float epsilon = EPSILON) {
    return std::abs(value) < epsilon;
}

// 符号
template<typename T>
inline int Sign(T value) { return (T(0) < value) - (value < T(0)); }

// 随机数
inline float Random01() { return static_cast<float>(rand()) / static_cast<float>(RAND_MAX); }
inline float RandomRange(float minVal, float maxVal) { return Lerp(minVal, maxVal, Random01()); }

} // namespace Math
} // namespace MiniEngine

#endif // MINI_ENGINE_MATH_UTILS_H
