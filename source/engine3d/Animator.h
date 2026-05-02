/**
 * Animator.h - 骨骼动画系统
 *
 * 第25章核心引擎组件。Header-Only。
 *
 * ============================================================================
 * 骨骼动画管线概览
 * ============================================================================
 *
 *   FBX文件(Assimp加载)
 *       ↓
 *   提取骨骼树 + 动画数据
 *       ↓
 *   每帧：在动画关键帧之间线性插值 → 计算各骨骼的局部变换
 *       ↓
 *   沿骨骼树从根到叶递归累乘 → 得到世界空间变换
 *       ↓
 *   乘以逆绑定矩阵(offsetMatrix) → 得到蒙皮矩阵(finalBoneMatrices)
 *       ↓
 *   上传到Shader的 uniform mat4 uBones[N]
 *       ↓
 *   顶点着色器：用骨骼权重混合多个骨骼矩阵变换顶点位置
 *
 * ============================================================================
 * 核心公式
 * ============================================================================
 *
 * 顶点蒙皮公式：
 *
 *   skinnedPos = Σ (weight_i × BoneFinal_i × bindPosePos)
 *
 * 其中 BoneFinal_i = GlobalTransform_i × OffsetMatrix_i
 *
 *   GlobalTransform = parent.GlobalTransform × localTransform(t)
 *   OffsetMatrix    = 骨骼的逆绑定姿态矩阵（把顶点从模型空间变到骨骼空间）
 *   localTransform(t) = 从动画关键帧插值得到的平移×旋转×缩放
 *
 * ============================================================================
 * 简化设计
 * ============================================================================
 *
 * 本章使用简化的骨骼动画实现：
 * - 仅使用Mat4（不引入四元数），旋转通过Assimp的矩阵直接转换
 * - 关键帧插值使用线性插值（LERP），不使用SLERP
 * - 最大骨骼数 MAX_BONES = 100
 * - 每顶点最多受4根骨骼影响
 */

#pragma once

#include "math/Math.h"
#include "engine3d/Mesh3D.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace MiniEngine {

using Math::Vec2;
using Math::Vec3;
using Math::Vec4;
using Math::Mat4;

// ============================================================================
// 常量
// ============================================================================
static const int MAX_BONES = 100;
static const int MAX_BONE_INFLUENCE = 4;

// ============================================================================
// 蒙皮顶点（扩展Vertex3D，增加骨骼ID和权重）
// ============================================================================
struct SkinnedVertex {
    Vec3 position;
    Vec3 normal   = Vec3(0, 1, 0);
    Vec3 color    = Vec3(1, 1, 1);
    Vec2 texCoord = Vec2(0, 0);
    int   boneIDs[MAX_BONE_INFLUENCE]    = {-1, -1, -1, -1};
    float boneWeights[MAX_BONE_INFLUENCE] = {0, 0, 0, 0};
};

// ============================================================================
// 骨骼信息
// ============================================================================
struct BoneInfo {
    int   id = -1;         // 骨骼在数组中的索引
    Mat4  offsetMatrix;    // 逆绑定矩阵（模型空间→骨骼空间）
};

// ============================================================================
// 动画关键帧
// ============================================================================
struct KeyPosition {
    Vec3  value;
    float time;
};

struct KeyRotation {
    // 用四元数存旋转（x,y,z,w）
    float x, y, z, w;
    float time;
};

struct KeyScale {
    Vec3  value;
    float time;
};

// ============================================================================
// 动画通道（一根骨骼在一个动画中的所有关键帧）
// ============================================================================
struct AnimChannel {
    std::string boneName;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale>    scales;
};

// ============================================================================
// 动画片段
// ============================================================================
struct AnimClip {
    std::string name;
    float duration    = 0;      // 动画总时长（tick）
    float ticksPerSec = 25.0f;  // 每秒tick数
    std::vector<AnimChannel> channels;
    
    float GetDurationSeconds() const {
        return (ticksPerSec > 0) ? duration / ticksPerSec : 0;
    }
};

// ============================================================================
// 骨骼节点（骨骼树中的一个节点）
// ============================================================================
struct BoneNode {
    std::string name;
    Mat4 localTransform;           // 默认变换（来自aiNode）
    int  boneIndex = -1;           // 对应BoneInfo的索引，-1表示非骨骼节点
    std::vector<int> childIndices; // 子节点在nodes数组中的索引
};

// ============================================================================
// 蒙皮网格（带骨骼权重的网格，使用独立VAO）
// ============================================================================
class SkinnedMesh {
public:
    SkinnedMesh() = default;
    ~SkinnedMesh() { Destroy(); }
    
    SkinnedMesh(SkinnedMesh&& o) noexcept
        : m_VAO(o.m_VAO), m_VBO(o.m_VBO), m_EBO(o.m_EBO),
          m_indexCount(o.m_indexCount), m_vertexCount(o.m_vertexCount)
    { o.m_VAO = o.m_VBO = o.m_EBO = 0; o.m_indexCount = o.m_vertexCount = 0; }
    
    SkinnedMesh& operator=(SkinnedMesh&& o) noexcept {
        if (this != &o) {
            Destroy();
            m_VAO = o.m_VAO; m_VBO = o.m_VBO; m_EBO = o.m_EBO;
            m_indexCount = o.m_indexCount; m_vertexCount = o.m_vertexCount;
            o.m_VAO = o.m_VBO = o.m_EBO = 0; o.m_indexCount = o.m_vertexCount = 0;
        }
        return *this;
    }
    
    SkinnedMesh(const SkinnedMesh&) = delete;
    SkinnedMesh& operator=(const SkinnedMesh&) = delete;
    
    void Create(const std::vector<SkinnedVertex>& verts, const std::vector<uint32_t>& idxs);
    void Draw() const;
    void Destroy();
    bool IsValid() const { return m_VAO != 0; }
    int GetVertexCount() const { return m_vertexCount; }
    int GetIndexCount() const { return m_indexCount; }

private:
    unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;
    int m_indexCount = 0, m_vertexCount = 0;
};

// ============================================================================
// 骨骼动画模型
// ============================================================================
class SkeletalModel {
public:
    std::vector<SkinnedMesh> meshes;
    std::vector<BoneInfo>    bones;
    std::map<std::string, int> boneNameToIndex;
    std::vector<BoneNode>    nodes;   // 骨骼树
    int rootNodeIndex = 0;
    Mat4 globalInverseTransform;
    
    // 统计
    int totalVertices  = 0;
    int totalTriangles = 0;
    std::string name;
    
    bool IsValid() const { return !meshes.empty(); }
    
    void Draw() const {
        for (auto& m : meshes) m.Draw();
    }
    
    void Destroy() {
        for (auto& m : meshes) m.Destroy();
        meshes.clear();
        bones.clear();
        boneNameToIndex.clear();
        nodes.clear();
    }
};

// ============================================================================
// Animator（骨骼动画控制器）
// ============================================================================
class Animator {
public:
    // ========================================================================
    // 加载
    // ========================================================================
    
    /**
     * 从FBX加载骨骼模型（网格 + 骨骼树）
     */
    static bool LoadModel(const std::string& path, SkeletalModel& out) {
        Assimp::Importer importer;
        unsigned int flags = aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs
                           | aiProcess_LimitBoneWeights;
        const aiScene* scene = importer.ReadFile(path, flags);
        if (!scene || !scene->mRootNode) {
            std::cerr << "[Animator] Failed to load model: " << path << "\n"
                      << "  " << importer.GetErrorString() << std::endl;
            return false;
        }
        
        out = SkeletalModel();
        
        // 全局逆变换
        out.globalInverseTransform = AiToMat4(scene->mRootNode->mTransformation).Inverse();
        
        // 构建节点树
        BuildNodeTree(scene->mRootNode, out, -1);
        
        // 提取网格和骨骼
        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
            out.meshes.push_back(ProcessSkinnedMesh(scene->mMeshes[i], out));
        }
        
        size_t lastSlash = path.find_last_of("/\\");
        out.name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        
        std::cout << "[Animator] Model: " << out.name
                  << " | Meshes:" << out.meshes.size()
                  << " | Bones:" << out.bones.size()
                  << " | Verts:" << out.totalVertices
                  << " | Tris:" << out.totalTriangles << std::endl;
        return true;
    }
    
    /**
     * 从FBX加载动画片段
     */
    static bool LoadAnimation(const std::string& path, AnimClip& out) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
        if (!scene || scene->mNumAnimations == 0) {
            std::cerr << "[Animator] No animation in: " << path << std::endl;
            return false;
        }
        
        aiAnimation* anim = scene->mAnimations[0];
        out.name = anim->mName.C_Str();
        if (out.name.empty()) {
            size_t sl = path.find_last_of("/\\");
            out.name = (sl != std::string::npos) ? path.substr(sl + 1) : path;
        }
        out.duration    = (float)anim->mDuration;
        out.ticksPerSec = (anim->mTicksPerSecond > 0) ? (float)anim->mTicksPerSecond : 25.0f;
        
        out.channels.resize(anim->mNumChannels);
        for (unsigned int i = 0; i < anim->mNumChannels; i++) {
            aiNodeAnim* ch = anim->mChannels[i];
            AnimChannel& ac = out.channels[i];
            ac.boneName = ch->mNodeName.C_Str();
            
            ac.positions.resize(ch->mNumPositionKeys);
            for (unsigned int j = 0; j < ch->mNumPositionKeys; j++) {
                ac.positions[j].value = Vec3(ch->mPositionKeys[j].mValue.x,
                                             ch->mPositionKeys[j].mValue.y,
                                             ch->mPositionKeys[j].mValue.z);
                ac.positions[j].time = (float)ch->mPositionKeys[j].mTime;
            }
            
            ac.rotations.resize(ch->mNumRotationKeys);
            for (unsigned int j = 0; j < ch->mNumRotationKeys; j++) {
                auto& q = ch->mRotationKeys[j].mValue;
                ac.rotations[j] = {q.x, q.y, q.z, q.w, (float)ch->mRotationKeys[j].mTime};
            }
            
            ac.scales.resize(ch->mNumScalingKeys);
            for (unsigned int j = 0; j < ch->mNumScalingKeys; j++) {
                ac.scales[j].value = Vec3(ch->mScalingKeys[j].mValue.x,
                                          ch->mScalingKeys[j].mValue.y,
                                          ch->mScalingKeys[j].mValue.z);
                ac.scales[j].time = (float)ch->mScalingKeys[j].mTime;
            }
        }
        
        std::cout << "[Animator] Anim: " << out.name
                  << " | Duration:" << out.GetDurationSeconds() << "s"
                  << " | Channels:" << out.channels.size() << std::endl;
        return true;
    }
    
    /**
     * 尝试多路径加载模型
     */
    static bool LoadModelFallback(const std::string& rel, SkeletalModel& out) {
        std::string paths[] = {"data/models/"+rel, "../data/models/"+rel, "../../data/models/"+rel, rel};
        for (auto& p : paths) { if (LoadModel(p, out)) return true; }
        return false;
    }
    
    /**
     * 尝试多路径加载动画
     */
    static bool LoadAnimFallback(const std::string& rel, AnimClip& out) {
        std::string paths[] = {"data/models/"+rel, "../data/models/"+rel, "../../data/models/"+rel, rel};
        for (auto& p : paths) { if (LoadAnimation(p, out)) return true; }
        return false;
    }
    
    // ========================================================================
    // 动画更新
    // ========================================================================
    
    /**
     * 更新动画：计算当前时间下所有骨骼的最终变换矩阵
     *
     * @param model   骨骼模型
     * @param clip    当前播放的动画片段
     * @param time    当前动画时间（秒）
     * @param output  输出的骨骼矩阵数组（至少MAX_BONES个）
     */
    static void ComputeBoneMatrices(const SkeletalModel& model,
                                    const AnimClip& clip,
                                    float time,
                                    Mat4* output,
                                    bool loop = true)
    {
        for (int i = 0; i < MAX_BONES; i++) output[i] = Mat4::Identity();

        if (model.nodes.empty()) return;

        float tps = (clip.ticksPerSec > 0) ? clip.ticksPerSec : 25.0f;
        float tickTime;
        if (loop) {
            tickTime = std::fmod(time * tps, clip.duration);
            if (tickTime < 0) tickTime += clip.duration;
        } else {
            tickTime = time * tps;
            if (tickTime >= clip.duration) tickTime = clip.duration - 0.001f;
            if (tickTime < 0) tickTime = 0;
        }

        ComputeNodeTransform(model, clip, tickTime, model.rootNodeIndex,
                             Mat4::Identity(), output);
    }
    
    // ========================================================================
    // 动画混合（线性混合两个动画的骨骼矩阵）
    // ========================================================================
    
    static void BlendBoneMatrices(const Mat4* a, const Mat4* b,
                                  float blendFactor, Mat4* output)
    {
        float t = std::max(0.0f, std::min(1.0f, blendFactor));
        for (int i = 0; i < MAX_BONES; i++) {
            for (int j = 0; j < 16; j++) {
                output[i].m[j] = a[i].m[j] * (1.0f - t) + b[i].m[j] * t;
            }
        }
    }

private:
    // ========================================================================
    // Assimp矩阵 → 引擎Mat4
    // ========================================================================
    static Mat4 AiToMat4(const aiMatrix4x4& m) {
        Mat4 r;
        // Assimp是行主序，我们的Mat4是列主序
        r.m[0]=m.a1; r.m[1]=m.b1; r.m[2]=m.c1; r.m[3]=m.d1;
        r.m[4]=m.a2; r.m[5]=m.b2; r.m[6]=m.c2; r.m[7]=m.d2;
        r.m[8]=m.a3; r.m[9]=m.b3; r.m[10]=m.c3; r.m[11]=m.d3;
        r.m[12]=m.a4; r.m[13]=m.b4; r.m[14]=m.c4; r.m[15]=m.d4;
        return r;
    }
    
    // ========================================================================
    // 构建节点树
    // ========================================================================
    static int BuildNodeTree(aiNode* aiN, SkeletalModel& model, int parentIdx) {
        int myIdx = (int)model.nodes.size();
        BoneNode node;
        node.name = aiN->mName.C_Str();
        node.localTransform = AiToMat4(aiN->mTransformation);
        
        auto it = model.boneNameToIndex.find(node.name);
        node.boneIndex = (it != model.boneNameToIndex.end()) ? it->second : -1;
        
        model.nodes.push_back(node);
        
        for (unsigned int i = 0; i < aiN->mNumChildren; i++) {
            int childIdx = BuildNodeTree(aiN->mChildren[i], model, myIdx);
            model.nodes[myIdx].childIndices.push_back(childIdx);
        }
        
        return myIdx;
    }
    
    // ========================================================================
    // 处理蒙皮网格
    // ========================================================================
    static SkinnedMesh ProcessSkinnedMesh(aiMesh* mesh, SkeletalModel& model) {
        std::vector<SkinnedVertex> verts(mesh->mNumVertices);
        std::vector<uint32_t> indices;
        
        // 顶点基础属性
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            SkinnedVertex& v = verts[i];
            v.position = Vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            if (mesh->mNormals)
                v.normal = Vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            if (mesh->mTextureCoords[0])
                v.texCoord = Vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            if (mesh->mColors[0])
                v.color = Vec3(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b);
            else
                v.color = Vec3(std::abs(v.normal.x)*0.5f+0.5f,
                               std::abs(v.normal.y)*0.5f+0.5f,
                               std::abs(v.normal.z)*0.5f+0.5f);
            
            for (int j = 0; j < MAX_BONE_INFLUENCE; j++) {
                v.boneIDs[j] = -1;
                v.boneWeights[j] = 0;
            }
        }
        
        // 索引
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace& f = mesh->mFaces[i];
            for (unsigned int j = 0; j < f.mNumIndices; j++)
                indices.push_back(f.mIndices[j]);
        }
        
        // 骨骼权重
        for (unsigned int i = 0; i < mesh->mNumBones; i++) {
            aiBone* bone = mesh->mBones[i];
            std::string boneName = bone->mName.C_Str();
            
            int boneIdx;
            auto it = model.boneNameToIndex.find(boneName);
            if (it == model.boneNameToIndex.end()) {
                boneIdx = (int)model.bones.size();
                BoneInfo bi;
                bi.id = boneIdx;
                bi.offsetMatrix = AiToMat4(bone->mOffsetMatrix);
                model.bones.push_back(bi);
                model.boneNameToIndex[boneName] = boneIdx;
            } else {
                boneIdx = it->second;
            }
            
            // 将权重分配给顶点
            for (unsigned int j = 0; j < bone->mNumWeights; j++) {
                unsigned int vid = bone->mWeights[j].mVertexId;
                float weight = bone->mWeights[j].mWeight;
                if (vid >= verts.size()) continue;
                
                // 找空位
                for (int k = 0; k < MAX_BONE_INFLUENCE; k++) {
                    if (verts[vid].boneIDs[k] < 0) {
                        verts[vid].boneIDs[k] = boneIdx;
                        verts[vid].boneWeights[k] = weight;
                        break;
                    }
                }
            }
        }
        
        // 更新节点树中的boneIndex
        for (auto& node : model.nodes) {
            auto it = model.boneNameToIndex.find(node.name);
            if (it != model.boneNameToIndex.end())
                node.boneIndex = it->second;
        }
        
        model.totalVertices += (int)verts.size();
        model.totalTriangles += (int)indices.size() / 3;
        
        SkinnedMesh sm;
        sm.Create(verts, indices);
        return sm;
    }
    
    // ========================================================================
    // 关键帧插值
    // ========================================================================
    
    static Vec3 InterpolatePosition(const AnimChannel& ch, float time) {
        if (ch.positions.empty()) return Vec3(0, 0, 0);
        if (ch.positions.size() == 1) return ch.positions[0].value;
        
        int idx = 0;
        for (int i = 0; i < (int)ch.positions.size() - 1; i++) {
            if (time < ch.positions[i + 1].time) { idx = i; break; }
            idx = i;
        }
        int next = std::min(idx + 1, (int)ch.positions.size() - 1);
        if (idx == next) return ch.positions[idx].value;
        
        float dt = ch.positions[next].time - ch.positions[idx].time;
        float t = (dt > 0.0001f) ? (time - ch.positions[idx].time) / dt : 0;
        t = std::max(0.0f, std::min(1.0f, t));
        
        Vec3 a = ch.positions[idx].value, b = ch.positions[next].value;
        return Vec3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
    }
    
    static Vec3 InterpolateScale(const AnimChannel& ch, float time) {
        if (ch.scales.empty()) return Vec3(1, 1, 1);
        if (ch.scales.size() == 1) return ch.scales[0].value;
        
        int idx = 0;
        for (int i = 0; i < (int)ch.scales.size() - 1; i++) {
            if (time < ch.scales[i + 1].time) { idx = i; break; }
            idx = i;
        }
        int next = std::min(idx + 1, (int)ch.scales.size() - 1);
        if (idx == next) return ch.scales[idx].value;
        
        float dt = ch.scales[next].time - ch.scales[idx].time;
        float t = (dt > 0.0001f) ? (time - ch.scales[idx].time) / dt : 0;
        t = std::max(0.0f, std::min(1.0f, t));
        
        Vec3 a = ch.scales[idx].value, b = ch.scales[next].value;
        return Vec3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
    }
    
    /**
     * 四元数NLERP插值 → 转Mat4
     */
    static Mat4 InterpolateRotation(const AnimChannel& ch, float time) {
        if (ch.rotations.empty()) return Mat4::Identity();
        if (ch.rotations.size() == 1) return QuatToMat4(ch.rotations[0]);
        
        int idx = 0;
        for (int i = 0; i < (int)ch.rotations.size() - 1; i++) {
            if (time < ch.rotations[i + 1].time) { idx = i; break; }
            idx = i;
        }
        int next = std::min(idx + 1, (int)ch.rotations.size() - 1);
        if (idx == next) return QuatToMat4(ch.rotations[idx]);
        
        float dt = ch.rotations[next].time - ch.rotations[idx].time;
        float t = (dt > 0.0001f) ? (time - ch.rotations[idx].time) / dt : 0;
        t = std::max(0.0f, std::min(1.0f, t));
        
        // NLERP（归一化线性插值）
        auto& a = ch.rotations[idx];
        auto& b = ch.rotations[next];
        
        // 确保最短路径
        float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
        float sign = (dot < 0) ? -1.0f : 1.0f;
        
        float rx = a.x * (1 - t) + b.x * sign * t;
        float ry = a.y * (1 - t) + b.y * sign * t;
        float rz = a.z * (1 - t) + b.z * sign * t;
        float rw = a.w * (1 - t) + b.w * sign * t;
        
        // 归一化
        float len = std::sqrt(rx*rx + ry*ry + rz*rz + rw*rw);
        if (len > 0.0001f) { rx /= len; ry /= len; rz /= len; rw /= len; }
        
        KeyRotation result = {rx, ry, rz, rw, 0};
        return QuatToMat4(result);
    }
    
    /**
     * 四元数 → 旋转矩阵
     */
    static Mat4 QuatToMat4(const KeyRotation& q) {
        float xx = q.x*q.x, yy = q.y*q.y, zz = q.z*q.z;
        float xy = q.x*q.y, xz = q.x*q.z, yz = q.y*q.z;
        float wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;
        
        Mat4 r = Mat4::Identity();
        r.m[0] = 1 - 2*(yy + zz);  r.m[4] = 2*(xy - wz);      r.m[8]  = 2*(xz + wy);
        r.m[1] = 2*(xy + wz);      r.m[5] = 1 - 2*(xx + zz);   r.m[9]  = 2*(yz - wx);
        r.m[2] = 2*(xz - wy);      r.m[6] = 2*(yz + wx);       r.m[10] = 1 - 2*(xx + yy);
        return r;
    }
    
    /**
     * 从AnimClip中查找骨骼名对应的通道
     */
    static const AnimChannel* FindChannel(const AnimClip& clip, const std::string& name) {
        for (auto& ch : clip.channels) {
            if (ch.boneName == name) return &ch;
        }
        return nullptr;
    }
    
    // ========================================================================
    // 递归计算节点变换
    // ========================================================================
    static void ComputeNodeTransform(const SkeletalModel& model,
                                     const AnimClip& clip,
                                     float tickTime,
                                     int nodeIdx,
                                     const Mat4& parentTransform,
                                     Mat4* output)
    {
        if (nodeIdx < 0 || nodeIdx >= (int)model.nodes.size()) return;
        const BoneNode& node = model.nodes[nodeIdx];
        
        Mat4 localTransform = node.localTransform;
        
        // 如果这个节点有动画通道，用插值替换默认变换
        const AnimChannel* ch = FindChannel(clip, node.name);
        if (ch) {
            Vec3 pos   = InterpolatePosition(*ch, tickTime);
            Mat4 rotM  = InterpolateRotation(*ch, tickTime);
            Vec3 scl   = InterpolateScale(*ch, tickTime);
            
            // 禁用root motion：所有非叶节点锁定水平位移
            // 角色的水平移动由CharacterController控制，不由动画驱动
            // 只保留Y轴位移（让角色有上下浮动效果）
            if (node.childIndices.size() > 0) {
                pos.x = 0;
                pos.z = 0;
            }
            
            localTransform = Mat4::Translate(pos.x, pos.y, pos.z) * rotM * Mat4::Scale(scl.x, scl.y, scl.z);
        }
        
        Mat4 globalTransform = parentTransform * localTransform;
        
        // 如果是骨骼节点，计算最终矩阵
        if (node.boneIndex >= 0 && node.boneIndex < (int)model.bones.size()) {
            output[node.boneIndex] = model.globalInverseTransform
                                   * globalTransform
                                   * model.bones[node.boneIndex].offsetMatrix;
        }
        
        // 递归子节点
        for (int childIdx : node.childIndices) {
            ComputeNodeTransform(model, clip, tickTime, childIdx, globalTransform, output);
        }
    }
};

} // namespace MiniEngine

