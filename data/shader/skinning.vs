/**
 * skinning.vs - 蒙皮顶点着色器
 *
 * 基于material.vs扩展，增加骨骼矩阵变换。
 *
 * 顶点蒙皮公式：
 *   skinnedPos = Σ(weight_i × uBones[boneID_i] × vec4(aPos, 1.0))
 *   每个顶点最多受4根骨骼影响（boneIDs + boneWeights）
 *
 * 新增输入属性（相比material.vs）：
 *   layout(location=4) in ivec4 aBoneIDs;     // 4个骨骼索引
 *   layout(location=5) in vec4  aBoneWeights;  // 4个骨骼权重
 *
 * 新增Uniform：
 *   uniform mat4 uBones[MAX_BONES];  // 骨骼最终变换矩阵数组
 *   uniform bool uUseSkinning;       // 是否启用蒙皮（方便fallback）
 */

#version 330 core

#define MAX_BONES 100

// 顶点属性
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aTexCoord;
layout (location = 4) in ivec4 aBoneIDs;
layout (location = 5) in vec4  aBoneWeights;

// 输出到片段着色器
out vec3 vFragPos;
out vec3 vNormal;
out vec3 vColor;
out vec2 vTexCoord;
out mat3 vTBN;

// 变换矩阵
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

// 骨骼动画
uniform mat4 uBones[MAX_BONES];
uniform bool uUseSkinning;

void main() {
    vec4 localPos;
    vec3 localNormal;

    if (uUseSkinning) {
        // ---- 蒙皮变换 ----
        // 用4个骨骼的加权矩阵变换顶点位置和法线
        mat4 boneTransform = mat4(0.0);
        bool hasBone = false;

        for (int i = 0; i < 4; i++) {
            if (aBoneIDs[i] >= 0 && aBoneIDs[i] < MAX_BONES) {
                boneTransform += aBoneWeights[i] * uBones[aBoneIDs[i]];
                hasBone = true;
            }
        }

        if (!hasBone) {
            // 没有骨骼影响的顶点，用单位矩阵
            boneTransform = mat4(1.0);
        }

        localPos    = boneTransform * vec4(aPos, 1.0);
        localNormal = mat3(boneTransform) * aNormal;
    } else {
        // ---- 无蒙皮，直接使用原始顶点 ----
        localPos    = vec4(aPos, 1.0);
        localNormal = aNormal;
    }

    // 世界空间变换
    vec4 worldPos = uModel * localPos;
    vFragPos  = vec3(worldPos);
    vNormal   = normalize(uNormalMatrix * localNormal);
    vColor    = aColor;
    vTexCoord = aTexCoord;

    // TBN矩阵（简化计算，与material.vs相同）
    vec3 N = vNormal;
    vec3 T;
    if (abs(N.y) < 0.999)
        T = normalize(cross(N, vec3(0.0, 1.0, 0.0)));
    else
        T = normalize(cross(N, vec3(1.0, 0.0, 0.0)));
    vec3 B = normalize(cross(N, T));
    vTBN = mat3(T, B, N);

    gl_Position = uProjection * uView * worldPos;
}
