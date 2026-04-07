/**
 * Mesh3D.h - 3D网格数据
 *
 * ============================================================================
 * 什么是Mesh？
 * ============================================================================
 *
 * Mesh（网格）是3D物体的"骨架"，由顶点和三角形面组成。
 *
 *   一个立方体 = 8个顶点 + 12个三角形（每个面2个三角形×6个面）
 *
 *          7--------6
 *         /|       /|
 *        4--------5 |
 *        | |      | |     Y
 *        | 3------|-2     |
 *        |/       |/      |___X
 *        0--------1      /
 *                        Z
 *
 * ============================================================================
 * 顶点数据结构
 * ============================================================================
 *
 *   struct Vertex3D {
 *       Vec3 position;   // 位置（必须）
 *       Vec3 normal;     // 法线（光照必须）
 *       Vec3 color;      // 颜色（可选，调试用）
 *       Vec2 texCoord;   // UV坐标（纹理映射用）
 *   };
 *
 * ============================================================================
 * OpenGL缓冲对象
 * ============================================================================
 *
 *   VAO (Vertex Array Object) — 记录顶点属性格式
 *   VBO (Vertex Buffer Object) — 存储顶点数据
 *   EBO (Element Buffer Object) — 存储索引数据（避免重复顶点）
 *
 *   渲染流程：
 *   1. 绑定VAO
 *   2. glDrawElements() 按索引绘制三角形
 *   3. 解绑VAO
 */

#pragma once

#include "math/Math.h"
#include <vector>
#include <cstdint>

namespace MiniEngine {

using Math::Vec2;
using Math::Vec3;
using Math::Vec4;

/**
 * 3D顶点
 */
struct Vertex3D {
    Vec3 position;               // 位置
    Vec3 normal = Vec3(0, 1, 0); // 法线
    Vec3 color = Vec3(1, 1, 1);  // 颜色
    Vec2 texCoord = Vec2(0, 0);  // UV
};

/**
 * 3D网格
 */
class Mesh3D {
public:
    Mesh3D() = default;
    ~Mesh3D();
    
    // 禁止拷贝（持有OpenGL资源）
    Mesh3D(const Mesh3D&) = delete;
    Mesh3D& operator=(const Mesh3D&) = delete;
    // 允许移动
    Mesh3D(Mesh3D&& other) noexcept;
    Mesh3D& operator=(Mesh3D&& other) noexcept;
    
    /**
     * 从顶点和索引数据创建Mesh
     */
    void Create(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices);
    
    /**
     * 渲染此Mesh
     */
    void Draw() const;
    
    /**
     * 释放GPU资源
     */
    void Destroy();
    
    /**
     * 顶点/索引数量
     */
    int GetVertexCount() const { return m_vertexCount; }
    int GetIndexCount() const { return m_indexCount; }
    bool IsValid() const { return m_VAO != 0; }
    
    // ========================================================================
    // 预设形状工厂方法
    // ========================================================================
    
    /** 创建立方体（带顶点颜色和法线） */
    static Mesh3D CreateCube(float size = 1.0f);
    
    /** 创建彩色立方体（每面不同颜色） */
    static Mesh3D CreateColorCube(float size = 1.0f);
    
    /** 创建平面（XZ平面） */
    static Mesh3D CreatePlane(float width = 1.0f, float depth = 1.0f, int subdivX = 1, int subdivZ = 1);
    
    /** 创建球体 */
    static Mesh3D CreateSphere(float radius = 0.5f, int segments = 16, int rings = 12);
    
private:
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_EBO = 0;
    int m_vertexCount = 0;
    int m_indexCount = 0;
};

} // namespace MiniEngine
