/**
 * Vec3.h - 三维向量类
 */

#ifndef MINI_ENGINE_VEC3_H
#define MINI_ENGINE_VEC3_H

#include <cmath>
#include <iostream>
#include "Vec2.h"

namespace MiniEngine {
namespace Math {

class Vec3 {
public:
    float x, y, z;
    
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    explicit Vec3(float value) : x(value), y(value), z(value) {}
    explicit Vec3(const Vec2& v, float z_ = 0.0f) : x(v.x), y(v.y), z(z_) {}
    
    static Vec3 Zero()    { return Vec3(0.0f, 0.0f, 0.0f); }
    static Vec3 One()     { return Vec3(1.0f, 1.0f, 1.0f); }
    static Vec3 Up()      { return Vec3(0.0f, 1.0f, 0.0f); }
    static Vec3 Down()    { return Vec3(0.0f, -1.0f, 0.0f); }
    static Vec3 Left()    { return Vec3(-1.0f, 0.0f, 0.0f); }
    static Vec3 Right()   { return Vec3(1.0f, 0.0f, 0.0f); }
    static Vec3 Forward() { return Vec3(0.0f, 0.0f, -1.0f); }
    static Vec3 Back()    { return Vec3(0.0f, 0.0f, 1.0f); }
    
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3& operator-=(const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    Vec3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    Vec3 operator/(float scalar) const { return Vec3(x / scalar, y / scalar, z / scalar); }
    Vec3 operator*(const Vec3& other) const { return Vec3(x * other.x, y * other.y, z * other.z); }
    
    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float LengthSquared() const { return x * x + y * y + z * z; }
    
    Vec3 Normalized() const {
        float len = Length();
        if (len > 0.0f) return Vec3(x / len, y / len, z / len);
        return Vec3::Zero();
    }
    
    void Normalize() {
        float len = Length();
        if (len > 0.0f) {
            x /= len; y /= len; z /= len;
        }
    }
    
    float Dot(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }
    static float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    
    Vec3 Cross(const Vec3& other) const {
        return Vec3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    }
    static Vec3 Cross(const Vec3& a, const Vec3& b) { return a.Cross(b); }
    
    static float Distance(const Vec3& a, const Vec3& b) { return (b - a).Length(); }
    static Vec3 Lerp(const Vec3& a, const Vec3& b, float t) {
        return Vec3(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), a.z + t * (b.z - a.z));
    }
    
    static Vec3 Reflect(const Vec3& incident, const Vec3& normal) {
        return incident - normal * (2.0f * incident.Dot(normal));
    }
    
    Vec2 XY() const { return Vec2(x, y); }
    
    bool operator==(const Vec3& other) const { return x == other.x && y == other.y && z == other.z; }
    bool operator!=(const Vec3& other) const { return !(*this == other); }
    
    friend std::ostream& operator<<(std::ostream& os, const Vec3& v) {
        os << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

inline Vec3 operator*(float scalar, const Vec3& v) { return v * scalar; }

} // namespace Math
} // namespace MiniEngine

#endif // MINI_ENGINE_VEC3_H
