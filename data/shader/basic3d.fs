/**
 * basic3d.fs - 3D基础片段着色器
 *
 * 功能：
 * 1. 使用顶点颜色渲染
 * 2. 简单的方向光照（为后续章节铺垫）
 * 3. 支持纯色模式和顶点色模式切换
 *
 * 光照计算（简化版Lambertian）：
 *   diffuse = max(dot(normal, lightDir), 0.0)
 *   最终颜色 = (ambient + diffuse) * 物体颜色
 */

#version 330 core

// 从顶点着色器接收
in vec3 vColor;
in vec3 vNormal;
in vec3 vFragPos;
in vec2 vTexCoord;

// 输出颜色
out vec4 FragColor;

// Uniform
uniform vec3 uLightDir;        // 光照方向（世界空间，已归一化）
uniform vec3 uLightColor;      // 光照颜色
uniform float uAmbientStrength; // 环境光强度
uniform vec4 uBaseColor;       // 基础颜色（当useVertexColor=false时使用）
uniform bool uUseVertexColor;  // 是否使用顶点颜色
uniform bool uUseLighting;     // 是否启用光照

void main() {
    // 确定物体颜色
    vec3 objectColor = uUseVertexColor ? vColor : uBaseColor.rgb;
    float alpha = uUseVertexColor ? 1.0 : uBaseColor.a;
    
    if (uUseLighting) {
        // 环境光
        vec3 ambient = uAmbientStrength * uLightColor;
        
        // 漫反射（Lambertian）
        vec3 norm = normalize(vNormal);
        vec3 lightDir = normalize(uLightDir);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * uLightColor;
        
        // 最终颜色 = (环境光 + 漫反射) × 物体颜色
        vec3 result = (ambient + diffuse) * objectColor;
        FragColor = vec4(result, alpha);
    } else {
        // 无光照，直接输出颜色
        FragColor = vec4(objectColor, alpha);
    }
}
