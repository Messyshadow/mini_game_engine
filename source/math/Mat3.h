/**
 * Mat3.h - 3x3矩阵类
 * 《3D数学基础》第4-5章 - 矩阵
 * 
 * ============================================================================
 * 为什么2D变换需要3x3矩阵？
 * ============================================================================
 * 
 * 2x2矩阵可以表示：旋转、缩放、错切
 * 但2x2矩阵 **无法表示平移**！
 * 
 * 解决方案：齐次坐标（Homogeneous Coordinates）
 * 
 * 2D点 (x, y) → 齐次坐标 (x, y, 1)
 * 
 * 这样我们可以用3x3矩阵同时表示：旋转、缩放、平移
 * 
 * ============================================================================
 * 矩阵内存布局
 * ============================================================================
 * 
 * OpenGL使用 **列主序（Column-Major）**：
 * 
 *              数学表示                    内存布局
 *         ┌         ┐
 *         │ m0 m3 m6│              m[0] m[1] m[2] m[3] m[4] m[5] m[6] m[7] m[8]
 *     M = │ m1 m4 m7│      →        ↓    ↓    ↓    ↓    ↓    ↓    ↓    ↓    ↓
 *         │ m2 m5 m8│              第0列      第1列      第2列
 *         └         ┘
 * 
 * 变换矩阵结构（2D）：
 *         ┌                   ┐
 *         │ scaleX  shearX  tx│    tx, ty = 平移
 *     M = │ shearY  scaleY  ty│    scaleX, scaleY = 缩放
 *         │   0       0     1 │    shearX, shearY = 错切
 *         └                   ┘
 * 
 * ============================================================================
 * 矩阵乘法顺序
 * ============================================================================
 * 
 * 变换组合：从右到左读
 * 
 *   Final = Translation * Rotation * Scale * vertex
 *              ③           ②         ①
 * 
 * 意思是：先缩放，再旋转，最后平移
 */

#ifndef MINI_ENGINE_MAT3_H
#define MINI_ENGINE_MAT3_H

#include "Vec2.h"
#include "Vec3.h"
#include "MathUtils.h"
#include <cmath>
#include <cstring>
#include <iostream>

namespace MiniEngine {
namespace Math {

class Mat3 {
public:
    // 列主序存储：m[列][行] 或 m[列*3 + 行]
    float m[9];
    
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    // 默认构造：单位矩阵
    Mat3() {
        // 单位矩阵：对角线为1，其余为0
        m[0] = 1; m[1] = 0; m[2] = 0;  // 第0列
        m[3] = 0; m[4] = 1; m[5] = 0;  // 第1列
        m[6] = 0; m[7] = 0; m[8] = 1;  // 第2列
    }
    
    // 从9个值构造（列主序）
    Mat3(float m0, float m1, float m2,
         float m3, float m4, float m5,
         float m6, float m7, float m8) {
        m[0] = m0; m[1] = m1; m[2] = m2;
        m[3] = m3; m[4] = m4; m[5] = m5;
        m[6] = m6; m[7] = m7; m[8] = m8;
    }
    
    // 从数组构造
    explicit Mat3(const float* data) {
        std::memcpy(m, data, 9 * sizeof(float));
    }
    
    // ========================================================================
    // 静态工厂方法 - 创建变换矩阵
    // ========================================================================
    
    /**
     * 单位矩阵
     * ┌       ┐
     * │ 1 0 0 │
     * │ 0 1 0 │
     * │ 0 0 1 │
     * └       ┘
     */
    static Mat3 Identity() {
        return Mat3();
    }
    
    /**
     * 平移矩阵
     * ┌         ┐
     * │ 1  0  tx│
     * │ 0  1  ty│
     * │ 0  0  1 │
     * └         ┘
     * 
     * 效果：点 (x, y) → (x + tx, y + ty)
     */
    static Mat3 Translate(float tx, float ty) {
        Mat3 result;
        result.m[6] = tx;
        result.m[7] = ty;
        return result;
    }
    
    static Mat3 Translate(const Vec2& translation) {
        return Translate(translation.x, translation.y);
    }
    
    /**
     * 缩放矩阵
     * ┌          ┐
     * │ sx  0  0 │
     * │ 0  sy  0 │
     * │ 0   0  1 │
     * └          ┘
     * 
     * 效果：点 (x, y) → (x * sx, y * sy)
     */
    static Mat3 Scale(float sx, float sy) {
        Mat3 result;
        result.m[0] = sx;
        result.m[4] = sy;
        return result;
    }
    
    static Mat3 Scale(const Vec2& scale) {
        return Scale(scale.x, scale.y);
    }
    
    static Mat3 Scale(float uniformScale) {
        return Scale(uniformScale, uniformScale);
    }
    
    /**
     * 旋转矩阵（绕原点逆时针旋转）
     * ┌                  ┐
     * │ cos(θ) -sin(θ) 0 │
     * │ sin(θ)  cos(θ) 0 │
     * │   0       0    1 │
     * └                  ┘
     * 
     * 推导：
     * 点 (x, y) 可以表示为 (r*cos(α), r*sin(α))
     * 旋转θ后：(r*cos(α+θ), r*sin(α+θ))
     * 展开后得到上述矩阵
     */
    static Mat3 Rotate(float angleRadians) {
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        
        Mat3 result;
        result.m[0] = c;  result.m[1] = s;   // 第0列
        result.m[3] = -s; result.m[4] = c;   // 第1列
        return result;
    }
    
    static Mat3 RotateDegrees(float angleDegrees) {
        return Rotate(ToRadians(angleDegrees));
    }
    
    /**
     * 绕指定点旋转
     * 
     * 步骤：
     * 1. 平移使旋转中心到原点
     * 2. 旋转
     * 3. 平移回原位置
     */
    static Mat3 RotateAround(const Vec2& center, float angleRadians) {
        return Translate(center) * Rotate(angleRadians) * Translate(-center.x, -center.y);
    }
    
    /**
     * 错切矩阵（Shear）
     * ┌            ┐
     * │ 1   shx  0 │
     * │ shy  1   0 │
     * │ 0    0   1 │
     * └            ┘
     */
    static Mat3 Shear(float shx, float shy) {
        Mat3 result;
        result.m[3] = shx;  // x方向错切
        result.m[1] = shy;  // y方向错切
        return result;
    }
    
    /**
     * 2D正交投影矩阵
     * 将世界坐标 [left, right] x [bottom, top] 映射到 NDC [-1, 1] x [-1, 1]
     * 
     * ┌                              ┐
     * │ 2/(r-l)    0     -(r+l)/(r-l)│
     * │    0    2/(t-b)  -(t+b)/(t-b)│
     * │    0       0          1      │
     * └                              ┘
     */
    static Mat3 Ortho(float left, float right, float bottom, float top) {
        Mat3 result;
        result.m[0] = 2.0f / (right - left);
        result.m[4] = 2.0f / (top - bottom);
        result.m[6] = -(right + left) / (right - left);
        result.m[7] = -(top + bottom) / (top - bottom);
        return result;
    }
    
    // ========================================================================
    // 访问器
    // ========================================================================
    
    // 获取列
    Vec3 GetColumn(int index) const {
        return Vec3(m[index * 3], m[index * 3 + 1], m[index * 3 + 2]);
    }
    
    // 设置列
    void SetColumn(int index, const Vec3& col) {
        m[index * 3] = col.x;
        m[index * 3 + 1] = col.y;
        m[index * 3 + 2] = col.z;
    }
    
    // 获取平移部分
    Vec2 GetTranslation() const {
        return Vec2(m[6], m[7]);
    }
    
    // 设置平移部分
    void SetTranslation(const Vec2& t) {
        m[6] = t.x;
        m[7] = t.y;
    }
    
    // 获取缩放部分（假设没有错切）
    Vec2 GetScale() const {
        return Vec2(
            Vec2(m[0], m[1]).Length(),
            Vec2(m[3], m[4]).Length()
        );
    }
    
    // 获取旋转角度（假设是纯旋转+缩放矩阵）
    float GetRotation() const {
        return std::atan2(m[1], m[0]);
    }
    
    // 获取原始数据指针（用于传给OpenGL）
    const float* Data() const { return m; }
    float* Data() { return m; }
    
    // ========================================================================
    // 矩阵运算
    // ========================================================================
    
    /**
     * 矩阵乘法
     * C = A * B
     * C[i][j] = Σ A[i][k] * B[k][j]
     */
    Mat3 operator*(const Mat3& other) const {
        Mat3 result;
        
        for (int col = 0; col < 3; col++) {
            for (int row = 0; row < 3; row++) {
                result.m[col * 3 + row] = 
                    m[0 * 3 + row] * other.m[col * 3 + 0] +
                    m[1 * 3 + row] * other.m[col * 3 + 1] +
                    m[2 * 3 + row] * other.m[col * 3 + 2];
            }
        }
        
        return result;
    }
    
    Mat3& operator*=(const Mat3& other) {
        *this = *this * other;
        return *this;
    }
    
    /**
     * 矩阵与向量相乘（变换点）
     * 将Vec2视为齐次坐标 (x, y, 1)
     */
    Vec2 operator*(const Vec2& v) const {
        float x = m[0] * v.x + m[3] * v.y + m[6];
        float y = m[1] * v.x + m[4] * v.y + m[7];
        // w = m[2] * v.x + m[5] * v.y + m[8]; // 对于仿射变换，w始终为1
        return Vec2(x, y);
    }
    
    /**
     * 矩阵与Vec3相乘
     */
    Vec3 operator*(const Vec3& v) const {
        return Vec3(
            m[0] * v.x + m[3] * v.y + m[6] * v.z,
            m[1] * v.x + m[4] * v.y + m[7] * v.z,
            m[2] * v.x + m[5] * v.y + m[8] * v.z
        );
    }
    
    /**
     * 变换方向向量（不受平移影响）
     * 将Vec2视为方向 (x, y, 0)
     */
    Vec2 TransformDirection(const Vec2& dir) const {
        return Vec2(
            m[0] * dir.x + m[3] * dir.y,
            m[1] * dir.x + m[4] * dir.y
        );
    }
    
    /**
     * 转置
     */
    Mat3 Transposed() const {
        return Mat3(
            m[0], m[3], m[6],
            m[1], m[4], m[7],
            m[2], m[5], m[8]
        );
    }
    
    /**
     * 行列式
     */
    float Determinant() const {
        return m[0] * (m[4] * m[8] - m[5] * m[7])
             - m[3] * (m[1] * m[8] - m[2] * m[7])
             + m[6] * (m[1] * m[5] - m[2] * m[4]);
    }
    
    /**
     * 逆矩阵
     * 使用伴随矩阵法：A^(-1) = adj(A) / det(A)
     */
    Mat3 Inverse() const {
        float det = Determinant();
        if (std::abs(det) < EPSILON) {
            // 矩阵不可逆，返回单位矩阵
            return Mat3::Identity();
        }
        
        float invDet = 1.0f / det;
        
        Mat3 result;
        result.m[0] = (m[4] * m[8] - m[5] * m[7]) * invDet;
        result.m[1] = (m[2] * m[7] - m[1] * m[8]) * invDet;
        result.m[2] = (m[1] * m[5] - m[2] * m[4]) * invDet;
        result.m[3] = (m[5] * m[6] - m[3] * m[8]) * invDet;
        result.m[4] = (m[0] * m[8] - m[2] * m[6]) * invDet;
        result.m[5] = (m[2] * m[3] - m[0] * m[5]) * invDet;
        result.m[6] = (m[3] * m[7] - m[4] * m[6]) * invDet;
        result.m[7] = (m[1] * m[6] - m[0] * m[7]) * invDet;
        result.m[8] = (m[0] * m[4] - m[1] * m[3]) * invDet;
        
        return result;
    }
    
    // ========================================================================
    // 比较运算符
    // ========================================================================
    
    bool operator==(const Mat3& other) const {
        for (int i = 0; i < 9; i++) {
            if (std::abs(m[i] - other.m[i]) > EPSILON) return false;
        }
        return true;
    }
    
    bool operator!=(const Mat3& other) const {
        return !(*this == other);
    }
    
    // ========================================================================
    // 调试输出
    // ========================================================================
    
    friend std::ostream& operator<<(std::ostream& os, const Mat3& mat) {
        os << "Mat3(\n";
        for (int row = 0; row < 3; row++) {
            os << "  ";
            for (int col = 0; col < 3; col++) {
                os << mat.m[col * 3 + row];
                if (col < 2) os << ", ";
            }
            os << "\n";
        }
        os << ")";
        return os;
    }
};

} // namespace Math
} // namespace MiniEngine

#endif // MINI_ENGINE_MAT3_H
