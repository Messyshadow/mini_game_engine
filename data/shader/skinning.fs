/**
 * skinning.fs - 蒙皮片段着色器
 *
 * 与material.fs完全相同：Phong光照 + 漫反射/法线/粗糙度贴图。
 * 蒙皮计算已在顶点着色器完成，片段着色器只负责光照和着色。
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

uniform sampler2D uDiffuseMap;
uniform sampler2D uNormalMap;
uniform sampler2D uRoughnessMap;

uniform bool uUseDiffuseMap;
uniform bool uUseNormalMap;
uniform bool uUseRoughnessMap;

uniform vec4  uBaseColor;
uniform bool  uUseVertexColor;
uniform float uShininess;
uniform float uAmbientStrength;
uniform float uTexTiling;

uniform vec3 uViewPos;

uniform int uNumDirLights;
uniform int uNumPointLights;
uniform DirLight  uDirLights[MAX_DIR_LIGHTS];
uniform PointLight uPointLights[MAX_POINT_LIGHTS];

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 albedo, float shininess) {
    vec3 lightDir = normalize(light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 ambient  = uAmbientStrength * light.color;
    vec3 diffuse  = diff * light.color;
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
    vec3 ambient  = uAmbientStrength * light.color * attenuation;
    vec3 diffuse  = diff * light.color * attenuation;
    vec3 specular = spec * light.color * 0.5 * attenuation;
    return (ambient + diffuse + specular) * albedo * light.intensity;
}

void main() {
    vec2 uv = vTexCoord * uTexTiling;

    // 1. Albedo
    vec3 albedo;
    if (uUseDiffuseMap)
        albedo = texture(uDiffuseMap, uv).rgb;
    else if (uUseVertexColor)
        albedo = vColor;
    else
        albedo = uBaseColor.rgb;

    // 2. Normal
    vec3 normal;
    if (uUseNormalMap) {
        vec3 tangentNormal = texture(uNormalMap, uv).rgb * 2.0 - 1.0;
        normal = normalize(vTBN * tangentNormal);
    } else {
        normal = normalize(vNormal);
    }

    // 3. Shininess
    float shininess = uShininess;
    if (uUseRoughnessMap) {
        float roughness = texture(uRoughnessMap, uv).r;
        shininess = max(2.0, (1.0 - roughness) * 256.0);
    }

    // 4. Lighting
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
