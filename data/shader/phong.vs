/**
 * phong.vs - Phong光照顶点着色器
 *
 * 职责：
 * 1. MVP变换（模型空间→裁剪空间）
 * 2. 计算世界空间的片段位置和法线（传给片段着色器做光照计算）
 * 3. 法线使用逆转置矩阵变换（防止非均匀缩放导致法线错误）
 */

#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aTexCoord;

out vec3 vFragPos;    // 世界空间片段位置
out vec3 vNormal;     // 世界空间法线
out vec3 vColor;      // 顶点颜色
out vec2 vTexCoord;   // 纹理坐标

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;  // Model矩阵的逆转置（CPU预计算，比GPU算更高效）

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = vec3(worldPos);
    vNormal = uNormalMatrix * aNormal;
    vColor = aColor;
    vTexCoord = aTexCoord;
    
    gl_Position = uProjection * uView * worldPos;
}
