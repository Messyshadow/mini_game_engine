/**
 * Mat4.h - 4x4矩阵类
 * 《3D数学基础》第6章 - 更多矩阵
 * 
 * ============================================================================
 * 4x4矩阵结构
 * ============================================================================
 * 
 *         ┌                     ┐
 *         │ Xx  Yx  Zx  Tx │    X, Y, Z = 旋转/缩放部分（3x3）
 *     M = │ Xy  Yy  Zy  Ty │    T = 平移部分
 *         │ Xz  Yz  Zz  Tz │    最后一行通常是 [0, 0, 0, 1]
 *         │  0   0   0   1 │
 *         └                     ┘
 * 
 * ============================================================================
 * 坐标系统
 * ============================================================================
 * 
 * OpenGL使用右手坐标系：
 *        Y
 *        ↑
 *        │
 *        │
 *        └────→ X
 *       /
 *      Z (指向屏幕外)
 * 
 * ============================================================================
 * MVP变换管线
 * ============================================================================
 * 
 *   gl_Position = Projection * View * Model * vec4(position, 1.0)
 * 
 *   Model矩阵：物体本地坐标 → 世界坐标
 *   View矩阵：世界坐标 → 相机坐标
 *   Projection矩阵：相机坐标 → 裁剪坐标（NDC）
 */

#ifndef MINI_ENGINE_MAT4_H
#define MINI_ENGINE_MAT4_H

#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Mat3.h"
#include "MathUtils.h"
#include <cmath>
#include <cstring>
#include <iostream>

namespace MiniEngine {
namespace Math {

class Mat4 {
public:
    // 列主序存储：16个float
    float m[16];
    
    // ========================================================================
    // 构造函数
    // ========================================================================
    
    // 默认构造：单位矩阵
    Mat4() {
        m[0] = 1;  m[1] = 0;  m[2] = 0;  m[3] = 0;   // 第0列
        m[4] = 0;  m[5] = 1;  m[6] = 0;  m[7] = 0;   // 第1列
        m[8] = 0;  m[9] = 0;  m[10] = 1; m[11] = 0;  // 第2列
        m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;  // 第3列
    }
    
    // 从16个值构造（列主序）
    Mat4(float m0,  float m1,  float m2,  float m3,
         float m4,  float m5,  float m6,  float m7,
         float m8,  float m9,  float m10, float m11,
         float m12, float m13, float m14, float m15) {
        m[0] = m0;   m[1] = m1;   m[2] = m2;   m[3] = m3;
        m[4] = m4;   m[5] = m5;   m[6] = m6;   m[7] = m7;
        m[8] = m8;   m[9] = m9;   m[10] = m10; m[11] = m11;
        m[12] = m12; m[13] = m13; m[14] = m14; m[15] = m15;
    }
    
    // 从数组构造
    explicit Mat4(const float* data) {
        std::memcpy(m, data, 16 * sizeof(float));
    }
    
    // 从Mat3构造（嵌入到左上角3x3）
    explicit Mat4(const Mat3& mat3) {
        m[0] = mat3.m[0]; m[1] = mat3.m[1]; m[2] = mat3.m[2]; m[3] = 0;
        m[4] = mat3.m[3]; m[5] = mat3.m[4]; m[6] = mat3.m[5]; m[7] = 0;
        m[8] = mat3.m[6]; m[9] = mat3.m[7]; m[10] = mat3.m[8]; m[11] = 0;
        m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
    }
    
    // ========================================================================
    // 静态工厂方法 - 创建变换矩阵
    // ========================================================================
    
    static Mat4 Identity() {
        return Mat4();
    }
    
    /**
     * 平移矩阵
     */
    static Mat4 Translate(float tx, float ty, float tz) {
        Mat4 result;
        result.m[12] = tx;
        result.m[13] = ty;
        result.m[14] = tz;
        return result;
    }
    
    static Mat4 Translate(const Vec3& translation) {
        return Translate(translation.x, translation.y, translation.z);
    }
    
    // 2D平移（z=0）
    static Mat4 Translate(const Vec2& translation) {
        return Translate(translation.x, translation.y, 0.0f);
    }
    
    /**
     * 缩放矩阵
     */
    static Mat4 Scale(float sx, float sy, float sz) {
        Mat4 result;
        result.m[0] = sx;
        result.m[5] = sy;
        result.m[10] = sz;
        return result;
    }
    
    static Mat4 Scale(const Vec3& scale) {
        return Scale(scale.x, scale.y, scale.z);
    }
    
    static Mat4 Scale(float uniformScale) {
        return Scale(uniformScale, uniformScale, uniformScale);
    }
    
    // 2D缩放（z=1）
    static Mat4 Scale(const Vec2& scale) {
        return Scale(scale.x, scale.y, 1.0f);
    }
    
    /**
     * 绕X轴旋转
     */
    static Mat4 RotateX(float angleRadians) {
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        
        Mat4 result;
        result.m[5] = c;   result.m[6] = s;
        result.m[9] = -s;  result.m[10] = c;
        return result;
    }
    
    /**
     * 绕Y轴旋转
     */
    static Mat4 RotateY(float angleRadians) {
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        
        Mat4 result;
        result.m[0] = c;   result.m[2] = -s;
        result.m[8] = s;   result.m[10] = c;
        return result;
    }
    
    /**
     * 绕Z轴旋转（2D旋转）
     */
    static Mat4 RotateZ(float angleRadians) {
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        
        Mat4 result;
        result.m[0] = c;  result.m[1] = s;
        result.m[4] = -s; result.m[5] = c;
        return result;
    }
    
    /**
     * 绕任意轴旋转（罗德里格斯公式）
     */
    static Mat4 Rotate(const Vec3& axis, float angleRadians) {
        Vec3 n = axis.Normalized();
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        float t = 1.0f - c;
        
        Mat4 result;
        result.m[0] = t * n.x * n.x + c;
        result.m[1] = t * n.x * n.y + s * n.z;
        result.m[2] = t * n.x * n.z - s * n.y;
        
        result.m[4] = t * n.x * n.y - s * n.z;
        result.m[5] = t * n.y * n.y + c;
        result.m[6] = t * n.y * n.z + s * n.x;
        
        result.m[8] = t * n.x * n.z + s * n.y;
        result.m[9] = t * n.y * n.z - s * n.x;
        result.m[10] = t * n.z * n.z + c;
        
        return result;
    }
    
    /**
     * 2D正交投影矩阵
     * 用于2D游戏：将像素坐标映射到NDC
     * 
     * 例：Ortho2D(0, 1280, 0, 720) 将 (0,0)-(1280,720) 映射到 (-1,-1)-(1,1)
     */
    static Mat4 Ortho2D(float left, float right, float bottom, float top) {
        return Ortho(left, right, bottom, top, -1.0f, 1.0f);
    }
    
    /**
     * 3D正交投影矩阵
     */
    static Mat4 Ortho(float left, float right, float bottom, float top, 
                      float nearPlane, float farPlane) {
        Mat4 result;
        
        result.m[0] = 2.0f / (right - left);
        result.m[5] = 2.0f / (top - bottom);
        result.m[10] = -2.0f / (farPlane - nearPlane);
        
        result.m[12] = -(right + left) / (right - left);
        result.m[13] = -(top + bottom) / (top - bottom);
        result.m[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        
        return result;
    }
    
    /**
     * 透视投影矩阵
     * 
     * @param fovY 垂直视野角度（弧度）
     * @param aspect 宽高比 (width / height)
     * @param nearPlane 近裁剪面
     * @param farPlane 远裁剪面
     */
    static Mat4 Perspective(float fovY, float aspect, float nearPlane, float farPlane) {
        float tanHalfFov = std::tan(fovY / 2.0f);
        
        Mat4 result;
        std::memset(result.m, 0, 16 * sizeof(float));
        
        result.m[0] = 1.0f / (aspect * tanHalfFov);
        result.m[5] = 1.0f / tanHalfFov;
        result.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result.m[11] = -1.0f;
        result.m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        
        return result;
    }
    
    /**
     * 观察矩阵（LookAt）
     * 
     * @param eye 相机位置
     * @param target 观察目标点
     * @param up 上方向（通常是 (0, 1, 0)）
     */
    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        Vec3 zAxis = (eye - target).Normalized();  // 相机看向-Z
        Vec3 xAxis = up.Cross(zAxis).Normalized();
        Vec3 yAxis = zAxis.Cross(xAxis);
        
        Mat4 result;
        result.m[0] = xAxis.x;  result.m[1] = yAxis.x;  result.m[2] = zAxis.x;
        result.m[4] = xAxis.y;  result.m[5] = yAxis.y;  result.m[6] = zAxis.y;
        result.m[8] = xAxis.z;  result.m[9] = yAxis.z;  result.m[10] = zAxis.z;
        
        result.m[12] = -xAxis.Dot(eye);
        result.m[13] = -yAxis.Dot(eye);
        result.m[14] = -zAxis.Dot(eye);
        
        return result;
    }
    
    /**
     * 组合变换：平移 * 旋转 * 缩放
     * 这是最常用的变换组合顺序
     */
    static Mat4 TRS(const Vec3& translation, const Vec3& rotationEuler, const Vec3& scale) {
        return Translate(translation) * 
               RotateZ(rotationEuler.z) * RotateY(rotationEuler.y) * RotateX(rotationEuler.x) *
               Scale(scale);
    }
    
    // 2D版本的TRS
    static Mat4 TRS2D(const Vec2& translation, float rotationRadians, const Vec2& scale) {
        return Translate(translation) * RotateZ(rotationRadians) * Scale(scale);
    }
    
    // ========================================================================
    // 访问器
    // ========================================================================
    
    Vec4 GetColumn(int index) const {
        return Vec4(m[index * 4], m[index * 4 + 1], m[index * 4 + 2], m[index * 4 + 3]);
    }
    
    void SetColumn(int index, const Vec4& col) {
        m[index * 4] = col.x;
        m[index * 4 + 1] = col.y;
        m[index * 4 + 2] = col.z;
        m[index * 4 + 3] = col.w;
    }
    
    Vec3 GetTranslation() const {
        return Vec3(m[12], m[13], m[14]);
    }
    
    void SetTranslation(const Vec3& t) {
        m[12] = t.x;
        m[13] = t.y;
        m[14] = t.z;
    }
    
    Vec3 GetScale() const {
        return Vec3(
            Vec3(m[0], m[1], m[2]).Length(),
            Vec3(m[4], m[5], m[6]).Length(),
            Vec3(m[8], m[9], m[10]).Length()
        );
    }
    
    // 获取3x3旋转部分
    Mat3 GetRotationMatrix() const {
        Mat3 result;
        result.m[0] = m[0]; result.m[1] = m[1]; result.m[2] = m[2];
        result.m[3] = m[4]; result.m[4] = m[5]; result.m[5] = m[6];
        result.m[6] = m[8]; result.m[7] = m[9]; result.m[8] = m[10];
        return result;
    }
    
    const float* Data() const { return m; }
    float* Data() { return m; }
    
    // ========================================================================
    // 矩阵运算
    // ========================================================================
    
    Mat4 operator*(const Mat4& other) const {
        Mat4 result;
        
        for (int col = 0; col < 4; col++) {
            for (int row = 0; row < 4; row++) {
                result.m[col * 4 + row] = 
                    m[0 * 4 + row] * other.m[col * 4 + 0] +
                    m[1 * 4 + row] * other.m[col * 4 + 1] +
                    m[2 * 4 + row] * other.m[col * 4 + 2] +
                    m[3 * 4 + row] * other.m[col * 4 + 3];
            }
        }
        
        return result;
    }
    
    Mat4& operator*=(const Mat4& other) {
        *this = *this * other;
        return *this;
    }
    
    // 变换点（w=1）
    Vec3 TransformPoint(const Vec3& point) const {
        Vec4 result = *this * Vec4(point, 1.0f);
        if (std::abs(result.w) > EPSILON) {
            return Vec3(result.x / result.w, result.y / result.w, result.z / result.w);
        }
        return result.XYZ();
    }
    
    // 变换方向（w=0，不受平移影响）
    Vec3 TransformDirection(const Vec3& dir) const {
        return Vec3(
            m[0] * dir.x + m[4] * dir.y + m[8] * dir.z,
            m[1] * dir.x + m[5] * dir.y + m[9] * dir.z,
            m[2] * dir.x + m[6] * dir.y + m[10] * dir.z
        );
    }
    
    // 变换Vec4
    Vec4 operator*(const Vec4& v) const {
        return Vec4(
            m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
            m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
            m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
            m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w
        );
    }
    
    // 转置
    Mat4 Transposed() const {
        return Mat4(
            m[0], m[4], m[8], m[12],
            m[1], m[5], m[9], m[13],
            m[2], m[6], m[10], m[14],
            m[3], m[7], m[11], m[15]
        );
    }
    
    // 行列式
    float Determinant() const {
        float a = m[0], b = m[1], c = m[2], d = m[3];
        float e = m[4], f = m[5], g = m[6], h = m[7];
        float i = m[8], j = m[9], k = m[10], l = m[11];
        float n = m[12], o = m[13], p = m[14], q = m[15];
        
        float kq_lp = k * q - l * p;
        float jq_lo = j * q - l * o;
        float jp_ko = j * p - k * o;
        float iq_ln = i * q - l * n;
        float ip_kn = i * p - k * n;
        float io_jn = i * o - j * n;
        
        return a * (f * kq_lp - g * jq_lo + h * jp_ko)
             - b * (e * kq_lp - g * iq_ln + h * ip_kn)
             + c * (e * jq_lo - f * iq_ln + h * io_jn)
             - d * (e * jp_ko - f * ip_kn + g * io_jn);
    }
    
    // 逆矩阵
    Mat4 Inverse() const {
        float det = Determinant();
        if (std::abs(det) < EPSILON) {
            return Mat4::Identity();
        }
        
        float invDet = 1.0f / det;
        Mat4 result;
        
        // 使用伴随矩阵法计算（这里省略详细计算，用标准公式）
        float a = m[0], b = m[1], c = m[2], d = m[3];
        float e = m[4], f = m[5], g = m[6], h = m[7];
        float i = m[8], j = m[9], k = m[10], l = m[11];
        float n = m[12], o = m[13], p = m[14], q = m[15];
        
        result.m[0] = (f*(k*q - l*p) - g*(j*q - l*o) + h*(j*p - k*o)) * invDet;
        result.m[1] = -(b*(k*q - l*p) - c*(j*q - l*o) + d*(j*p - k*o)) * invDet;
        result.m[2] = (b*(g*q - h*p) - c*(f*q - h*o) + d*(f*p - g*o)) * invDet;
        result.m[3] = -(b*(g*l - h*k) - c*(f*l - h*j) + d*(f*k - g*j)) * invDet;
        
        result.m[4] = -(e*(k*q - l*p) - g*(i*q - l*n) + h*(i*p - k*n)) * invDet;
        result.m[5] = (a*(k*q - l*p) - c*(i*q - l*n) + d*(i*p - k*n)) * invDet;
        result.m[6] = -(a*(g*q - h*p) - c*(e*q - h*n) + d*(e*p - g*n)) * invDet;
        result.m[7] = (a*(g*l - h*k) - c*(e*l - h*i) + d*(e*k - g*i)) * invDet;
        
        result.m[8] = (e*(j*q - l*o) - f*(i*q - l*n) + h*(i*o - j*n)) * invDet;
        result.m[9] = -(a*(j*q - l*o) - b*(i*q - l*n) + d*(i*o - j*n)) * invDet;
        result.m[10] = (a*(f*q - h*o) - b*(e*q - h*n) + d*(e*o - f*n)) * invDet;
        result.m[11] = -(a*(f*l - h*j) - b*(e*l - h*i) + d*(e*j - f*i)) * invDet;
        
        result.m[12] = -(e*(j*p - k*o) - f*(i*p - k*n) + g*(i*o - j*n)) * invDet;
        result.m[13] = (a*(j*p - k*o) - b*(i*p - k*n) + c*(i*o - j*n)) * invDet;
        result.m[14] = -(a*(f*p - g*o) - b*(e*p - g*n) + c*(e*o - f*n)) * invDet;
        result.m[15] = (a*(f*k - g*j) - b*(e*k - g*i) + c*(e*j - f*i)) * invDet;
        
        return result;
    }
    
    // ========================================================================
    // 比较运算符
    // ========================================================================
    
    bool operator==(const Mat4& other) const {
        for (int i = 0; i < 16; i++) {
            if (std::abs(m[i] - other.m[i]) > EPSILON) return false;
        }
        return true;
    }
    
    bool operator!=(const Mat4& other) const {
        return !(*this == other);
    }
    
    // ========================================================================
    // 调试输出
    // ========================================================================
    
    friend std::ostream& operator<<(std::ostream& os, const Mat4& mat) {
        os << "Mat4(\n";
        for (int row = 0; row < 4; row++) {
            os << "  ";
            for (int col = 0; col < 4; col++) {
                os << mat.m[col * 4 + row];
                if (col < 3) os << ", ";
            }
            os << "\n";
        }
        os << ")";
        return os;
    }
};

} // namespace Math
} // namespace MiniEngine

#endif // MINI_ENGINE_MAT4_H
