/**
 * phong.fs - Phong光照片段着色器
 *
 * ============================================================================
 * Phong反射模型三个分量
 * ============================================================================
 *
 *   最终颜色 = Ambient + Diffuse + Specular
 *
 *   1. Ambient（环境光）：无方向的基础亮度，防止阴影全黑
 *      ambient = ambientStrength * lightColor
 *
 *   2. Diffuse（漫反射）：光照越垂直于表面越亮
 *      diffuse = max(dot(normal, lightDir), 0.0) * lightColor
 *
 *   3. Specular（镜面反射）：光照在光滑表面上的高光
 *      reflectDir = reflect(-lightDir, normal)
 *      spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess)
 *      specular = specularStrength * spec * lightColor
 *
 * ============================================================================
 * 三种光源类型
 * ============================================================================
 *
 *   方向光（Directional）：太阳光，无衰减，全局平行光线
 *   点光源（Point）：灯泡，有距离衰减，向所有方向发光
 *   聚光灯（Spot）：手电筒，有方向+锥角+衰减
 *
 * ============================================================================
 * 点光源衰减公式
 * ============================================================================
 *
 *   attenuation = 1.0 / (constant + linear * d + quadratic * d²)
 *
 *   d = 距离光源的距离
 *   constant = 1.0（保证近处不会超过1）
 *   linear = 线性衰减系数
 *   quadratic = 二次衰减系数
 *
 *   常用参数（覆盖半径）：
 *     7:   linear=0.7,    quadratic=1.8
 *     13:  linear=0.35,   quadratic=0.44
 *     20:  linear=0.22,   quadratic=0.20
 *     50:  linear=0.09,   quadratic=0.032
 *     100: linear=0.045,  quadratic=0.0075
 */

#version 330 core

// 最大光源数量
#define MAX_DIR_LIGHTS   2
#define MAX_POINT_LIGHTS 4
#define MAX_SPOT_LIGHTS  2

// ---- 光源结构 ----

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

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float cutOff;      // cos(内锥角)
    float outerCutOff; // cos(外锥角)
};

// ---- 输入 ----
in vec3 vFragPos;
in vec3 vNormal;
in vec3 vColor;
in vec2 vTexCoord;

// ---- 输出 ----
out vec4 FragColor;

// ---- Uniform ----
uniform vec3 uViewPos;         // 摄像机位置（计算镜面反射方向用）
uniform vec4 uBaseColor;       // 物体基础颜色
uniform bool uUseVertexColor;  // 是否使用顶点颜色
uniform float uShininess;     // 镜面反射光泽度（越大高光越小越锐利）
uniform float uAmbientStrength;

// 光源数据
uniform int uNumDirLights;
uniform int uNumPointLights;
uniform int uNumSpotLights;

uniform DirLight uDirLights[MAX_DIR_LIGHTS];
uniform PointLight uPointLights[MAX_POINT_LIGHTS];
uniform SpotLight uSpotLights[MAX_SPOT_LIGHTS];

// ---- 光照计算函数 ----

/**
 * 计算方向光
 */
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 objColor) {
    vec3 lightDir = normalize(light.direction);
    
    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
    
    // 合并
    vec3 ambient = uAmbientStrength * light.color;
    vec3 diffuse = diff * light.color;
    vec3 specular = spec * light.color * 0.5;
    
    return (ambient + diffuse + specular) * objColor * light.intensity;
}

/**
 * 计算点光源
 */
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
    
    // 距离衰减
    float dist = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    
    // 合并
    vec3 ambient = uAmbientStrength * light.color * attenuation;
    vec3 diffuse = diff * light.color * attenuation;
    vec3 specular = spec * light.color * 0.5 * attenuation;
    
    return (ambient + diffuse + specular) * objColor * light.intensity;
}

/**
 * 计算聚光灯
 */
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    // 聚光锥角衰减
    float theta = dot(lightDir, normalize(light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float spotIntensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
    
    // 距离衰减
    float dist = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    
    // 合并
    vec3 ambient = uAmbientStrength * light.color * attenuation;
    vec3 diffuse = diff * light.color * attenuation * spotIntensity;
    vec3 specular = spec * light.color * 0.5 * attenuation * spotIntensity;
    
    return (ambient + diffuse + specular) * objColor * light.intensity;
}

// ---- 主函数 ----

void main() {
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 objColor = uUseVertexColor ? vColor : uBaseColor.rgb;
    
    vec3 result = vec3(0.0);
    
    // 累加所有方向光
    for (int i = 0; i < uNumDirLights; i++) {
        result += CalcDirLight(uDirLights[i], normal, viewDir, objColor);
    }
    
    // 累加所有点光源
    for (int i = 0; i < uNumPointLights; i++) {
        result += CalcPointLight(uPointLights[i], normal, vFragPos, viewDir, objColor);
    }
    
    // 累加所有聚光灯
    for (int i = 0; i < uNumSpotLights; i++) {
        result += CalcSpotLight(uSpotLights[i], normal, vFragPos, viewDir, objColor);
    }
    
    // 如果没有任何光源，使用简单环境光
    if (uNumDirLights == 0 && uNumPointLights == 0 && uNumSpotLights == 0) {
        result = objColor * 0.3;
    }
    
    FragColor = vec4(result, uUseVertexColor ? 1.0 : uBaseColor.a);
}
