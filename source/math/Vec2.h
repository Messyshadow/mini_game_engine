/**
 * Vec2.h - 二维向量类
 * 《3D数学基础》第2章 - 向量
 */

#ifndef MINI_ENGINE_VEC2_H
#define MINI_ENGINE_VEC2_H

#include <cmath>
#include <iostream>

namespace MiniEngine {
namespace Math {

class Vec2 {
public:
    float x, y;
    
    // 构造函数
    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    explicit Vec2(float value) : x(value), y(value) {}
    
    // 静态工厂方法
    static Vec2 Zero()  { return Vec2(0.0f, 0.0f); }
    static Vec2 One()   { return Vec2(1.0f, 1.0f); }
    static Vec2 Up()    { return Vec2(0.0f, 1.0f); }
    static Vec2 Down()  { return Vec2(0.0f, -1.0f); }
    static Vec2 Left()  { return Vec2(-1.0f, 0.0f); }
    static Vec2 Right() { return Vec2(1.0f, 0.0f); }
    
    // 基本运算
    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    Vec2 operator-() const { return Vec2(-x, -y); }
    Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    Vec2 operator/(float scalar) const { return Vec2(x / scalar, y / scalar); }
    Vec2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }
    Vec2 operator*(const Vec2& other) const { return Vec2(x * other.x, y * other.y); }
    
    // 长度
    float Length() const { return std::sqrt(x * x + y * y); }
    float LengthSquared() const { return x * x + y * y; }
    
    // 归一化
    Vec2 Normalized() const {
        float len = Length();
        if (len > 0.0f) return Vec2(x / len, y / len);
        return Vec2::Zero();
    }
    void Normalize() {
        float len = Length();
        if (len > 0.0f) { x /= len; y /= len; }
    }
    
    // 点积
    float Dot(const Vec2& other) const { return x * other.x + y * other.y; }
    static float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
    
    // 2D叉积（返回标量）
    float Cross(const Vec2& other) const { return x * other.y - y * other.x; }
    static float Cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }
    
    // 实用函数
    static float Distance(const Vec2& a, const Vec2& b) { return (b - a).Length(); }
    static float DistanceSquared(const Vec2& a, const Vec2& b) { return (b - a).LengthSquared(); }
    static Vec2 Lerp(const Vec2& a, const Vec2& b, float t) {
        return Vec2(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y));
    }
    
    Vec2 Perpendicular() const { return Vec2(-y, x); }
    
    Vec2 Rotated(float angleRadians) const {
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        return Vec2(x * c - y * s, x * s + y * c);
    }
    
    float Angle() const { return std::atan2(y, x); }
    
    static float AngleBetween(const Vec2& a, const Vec2& b) {
        float dot = a.Dot(b);
        float lenProduct = a.Length() * b.Length();
        if (lenProduct > 0.0f) {
            float cosAngle = dot / lenProduct;
            cosAngle = (std::max)(-1.0f, (std::min)(1.0f, cosAngle));
            return std::acos(cosAngle);
        }
        return 0.0f;
    }
    
    bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Vec2& other) const { return !(*this == other); }
    
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        os << "Vec2(" << v.x << ", " << v.y << ")";
        return os;
    }
};

inline Vec2 operator*(float scalar, const Vec2& v) { return v * scalar; }

} // namespace Math
} // namespace MiniEngine

#endif // MINI_ENGINE_VEC2_H
