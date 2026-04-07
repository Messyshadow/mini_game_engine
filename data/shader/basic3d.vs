/**
 * basic3d.vs - 3D基础顶点着色器
 *
 * 功能：
 * 1. 将顶点从模型空间变换到裁剪空间（Model → View → Projection）
 * 2. 传递顶点颜色和法线给片段着色器
 *
 * MVP管线：
 *   gl_Position = Projection * View * Model * vec4(position, 1.0)
 *
 *   Model矩阵：物体本地坐标 → 世界坐标（位置、旋转、缩放）
 *   View矩阵：世界坐标 → 摄像机坐标（LookAt生成）
 *   Projection矩阵：摄像机坐标 → 裁剪坐标（透视或正交）
 */

#version 330 core

// 顶点属性输入
layout (location = 0) in vec3 aPos;       // 顶点位置（模型空间）
layout (location = 1) in vec3 aNormal;    // 顶点法线
layout (location = 2) in vec3 aColor;     // 顶点颜色
layout (location = 3) in vec2 aTexCoord;  // 纹理坐标（预留）

// 传递给片段着色器
out vec3 vColor;
out vec3 vNormal;
out vec3 vFragPos;    // 世界空间中的片段位置
out vec2 vTexCoord;

// Uniform矩阵
uniform mat4 uModel;       // 模型矩阵
uniform mat4 uView;        // 视图矩阵
uniform mat4 uProjection;  // 投影矩阵

void main() {
    // MVP变换：模型空间 → 裁剪空间
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    gl_Position = uProjection * uView * worldPos;
    
    // 传递数据给片段着色器
    vColor = aColor;
    vFragPos = vec3(worldPos);
    
    // 法线变换：需要用Model矩阵的逆转置（防止非均匀缩放导致法线错误）
    // 简单情况下（无非均匀缩放）直接用mat3(uModel)即可
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    
    vTexCoord = aTexCoord;
}
