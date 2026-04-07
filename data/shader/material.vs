/**
 * material.vs - 材质纹理顶点着色器
 *
 * 新增：计算TBN矩阵（Tangent-Bitangent-Normal）
 * TBN矩阵用于将法线贴图中的切线空间法线转换到世界空间
 */

#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aTexCoord;

out vec3 vFragPos;
out vec3 vNormal;
out vec3 vColor;
out vec2 vTexCoord;
out mat3 vTBN;       // 切线空间→世界空间的变换矩阵

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = vec3(worldPos);
    vNormal = normalize(uNormalMatrix * aNormal);
    vColor = aColor;
    vTexCoord = aTexCoord;
    
    // 计算TBN矩阵
    // Tangent和Bitangent的简化计算（基于法线生成）
    // 完整版应从顶点属性读取（Assimp的aiProcess_CalcTangentSpace可以生成）
    vec3 N = vNormal;
    vec3 T;
    // 选择一个不平行于法线的向量来叉乘
    if (abs(N.y) < 0.999)
        T = normalize(cross(N, vec3(0.0, 1.0, 0.0)));
    else
        T = normalize(cross(N, vec3(1.0, 0.0, 0.0)));
    vec3 B = normalize(cross(N, T));
    vTBN = mat3(T, B, N);
    
    gl_Position = uProjection * uView * worldPos;
}
