/**
 * Vec4.h - 四维向量类（用于齐次坐标和颜色）
 */

#ifndef MINI_ENGINE_VEC4_H
#define MINI_ENGINE_VEC4_H

#include <cmath>
#include <iostream>
#include "Vec2.h"
#include "Vec3.h"

namespace MiniEngine {
namespace Math {

class Vec4 {
public:
    float x, y, z, w;
    
    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    explicit Vec4(float value) : x(value), y(value), z(value), w(value) {}
    Vec4(const Vec3& v, float w_ = 1.0f) : x(v.x), y(v.y), z(v.z), w(w_) {}
    Vec4(const Vec2& v, float z_ = 0.0f, float w_ = 1.0f) : x(v.x), y(v.y), z(z_), w(w_) {}
    
    static Vec4 Zero() { return Vec4(0.0f, 0.0f, 0.0f, 0.0f); }
    static Vec4 One()  { return Vec4(1.0f, 1.0f, 1.0f, 1.0f); }
    
    // 颜色常量
    static Vec4 White()       { return Vec4(1.0f, 1.0f, 1.0f, 1.0f); }
    static Vec4 Black()       { return Vec4(0.0f, 0.0f, 0.0f, 1.0f); }
    static Vec4 Red()         { return Vec4(1.0f, 0.0f, 0.0f, 1.0f); }
    static Vec4 Green()       { return Vec4(0.0f, 1.0f, 0.0f, 1.0f); }
    static Vec4 Blue()        { return Vec4(0.0f, 0.0f, 1.0f, 1.0f); }
    static Vec4 Yellow()      { return Vec4(1.0f, 1.0f, 0.0f, 1.0f); }
    static Vec4 Cyan()        { return Vec4(0.0f, 1.0f, 1.0f, 1.0f); }
    static Vec4 Magenta()     { return Vec4(1.0f, 0.0f, 1.0f, 1.0f); }
    static Vec4 Transparent() { return Vec4(0.0f, 0.0f, 0.0f, 0.0f); }
    
    Vec4 operator+(const Vec4& other) const { return Vec4(x + other.x, y + other.y, z + other.z, w + other.w); }
    Vec4& operator+=(const Vec4& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
    Vec4 operator-(const Vec4& other) const { return Vec4(x - other.x, y - other.y, z - other.z, w - other.w); }
    Vec4 operator-() const { return Vec4(-x, -y, -z, -w); }
    Vec4 operator*(float scalar) const { return Vec4(x * scalar, y * scalar, z * scalar, w * scalar); }
    Vec4& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
    Vec4 operator/(float scalar) const { return Vec4(x / scalar, y / scalar, z / scalar, w / scalar); }
    Vec4 operator*(const Vec4& other) const { return Vec4(x * other.x, y * other.y, z * other.z, w * other.w); }
    
    float Length() const { return std::sqrt(x*x + y*y + z*z + w*w); }
    float LengthSquared() const { return x*x + y*y + z*z + w*w; }
    
    Vec4 Normalized() const {
        float len = Length();
        if (len > 0.0f) return Vec4(x/len, y/len, z/len, w/len);
        return Vec4::Zero();
    }
    
    float Dot(const Vec4& other) const { return x*other.x + y*other.y + z*other.z + w*other.w; }
    
    Vec3 PerspectiveDivide() const {
        if (std::abs(w) > 0.0001f) return Vec3(x/w, y/w, z/w);
        return Vec3(x, y, z);
    }
    
    Vec2 XY() const { return Vec2(x, y); }
    Vec3 XYZ() const { return Vec3(x, y, z); }
    Vec3 RGB() const { return Vec3(x, y, z); }
    
    static Vec4 FromRGBA(int r, int g, int b, int a = 255) {
        return Vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }
    
    static Vec4 Lerp(const Vec4& a, const Vec4& b, float t) {
        return Vec4(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), 
                    a.z + t * (b.z - a.z), a.w + t * (b.w - a.w));
    }
    
    bool operator==(const Vec4& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
    bool operator!=(const Vec4& other) const { return !(*this == other); }
    
    friend std::ostream& operator<<(std::ostream& os, const Vec4& v) {
        os << "Vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
        return os;
    }
};

inline Vec4 operator*(float scalar, const Vec4& v) { return v * scalar; }

} // namespace Math
} // namespace MiniEngine

#endif // MINI_ENGINE_VEC4_H
