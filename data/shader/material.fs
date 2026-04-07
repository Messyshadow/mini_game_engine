/**
 * material.fs - 材质纹理片段着色器
 *
 * 支持三种贴图：
 * 1. 漫反射贴图（Diffuse/Color Map）— 表面颜色
 * 2. 法线贴图（Normal Map）— 表面凹凸细节
 * 3. 粗糙度贴图（Roughness Map）— 控制高光大小
 *
 * 法线贴图原理：
 *   法线贴图存储的是切线空间(Tangent Space)的法线方向
 *   RGB值映射：R→X轴, G→Y轴, B→Z轴
 *   范围从[0,1]映射到[-1,1]：normal = texture * 2.0 - 1.0
 *   蓝紫色 = (0.5, 0.5, 1.0) = 朝上的法线(0, 0, 1) → 平坦表面
 *   偏红/偏绿 = 有倾斜 → 凹凸细节
 */

#version 330 core

#define MAX_DIR_LIGHTS   2
#define MAX_POINT_LIGHTS 4

struct DirLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
};

in vec3 vFragPos;
in vec3 vNormal;
in vec3 vColor;
in vec2 vTexCoord;
in mat3 vTBN;

out vec4 FragColor;

// 纹理采样器
uniform sampler2D uDiffuseMap;    // 漫反射贴图
uniform sampler2D uNormalMap;     // 法线贴图
uniform sampler2D uRoughnessMap;  // 粗糙度贴图

// 纹理开关
uniform bool uUseDiffuseMap;
uniform bool uUseNormalMap;
uniform bool uUseRoughnessMap;

// 材质参数
uniform vec4 uBaseColor;
uniform bool uUseVertexColor;
uniform float uShininess;
uniform float uAmbientStrength;
uniform float uTexTiling;  // 纹理平铺次数

// 摄像机
uniform vec3 uViewPos;

// 光源
uniform int uNumDirLights;
uniform int uNumPointLights;
uniform DirLight uDirLights[MAX_DIR_LIGHTS];
uniform PointLight uPointLights[MAX_POINT_LIGHTS];

// ---- 光照计算 ----

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo, float shininess) {
    vec3 lightDir = normalize(light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    vec3 ambient = uAmbientStrength * light.color;
    vec3 diffuse = diff * light.color;
    vec3 specular = spec * light.color * 0.5;
    
    return (ambient + diffuse + specular) * albedo * light.intensity;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float shininess) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    float dist = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    
    vec3 ambient = uAmbientStrength * light.color * attenuation;
    vec3 diffuse = diff * light.color * attenuation;
    vec3 specular = spec * light.color * 0.5 * attenuation;
    
    return (ambient + diffuse + specular) * albedo * light.intensity;
}

void main() {
    vec2 uv = vTexCoord * uTexTiling;
    
    // 1. 确定表面颜色（Albedo）
    vec3 albedo;
    if (uUseDiffuseMap) {
        albedo = texture(uDiffuseMap, uv).rgb;
    } else if (uUseVertexColor) {
        albedo = vColor;
    } else {
        albedo = uBaseColor.rgb;
    }
    
    // 2. 确定法线方向
    vec3 normal;
    if (uUseNormalMap) {
        // 从法线贴图读取切线空间法线
        vec3 tangentNormal = texture(uNormalMap, uv).rgb;
        tangentNormal = tangentNormal * 2.0 - 1.0; // [0,1] → [-1,1]
        // 用TBN矩阵转换到世界空间
        normal = normalize(vTBN * tangentNormal);
    } else {
        normal = normalize(vNormal);
    }
    
    // 3. 确定光泽度（从粗糙度贴图）
    float shininess = uShininess;
    if (uUseRoughnessMap) {
        float roughness = texture(uRoughnessMap, uv).r;
        // 粗糙度→光泽度：roughness越大=越粗糙=shininess越小
        shininess = max(2.0, (1.0 - roughness) * 256.0);
    }
    
    // 4. 光照计算
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 result = vec3(0.0);
    
    for (int i = 0; i < uNumDirLights; i++)
        result += CalcDirLight(uDirLights[i], normal, viewDir, albedo, shininess);
    for (int i = 0; i < uNumPointLights; i++)
        result += CalcPointLight(uPointLights[i], normal, vFragPos, viewDir, albedo, shininess);
    
    if (uNumDirLights == 0 && uNumPointLights == 0)
        result = albedo * 0.3;
    
    FragColor = vec4(result, uUseVertexColor ? 1.0 : uBaseColor.a);
}
