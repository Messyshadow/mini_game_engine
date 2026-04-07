/**
 * ModelLoader.h - 3D模型加载器（基于Assimp）
 *
 * ============================================================================
 * 模型加载管线
 * ============================================================================
 *
 *   .obj/.fbx 文件
 *         ↓
 *     Assimp解析  (aiScene → aiMesh → aiMaterial)
 *         ↓
 *     转换为引擎格式  (Vertex3D + indices)
 *         ↓
 *     创建Mesh3D  (上传到GPU: VAO/VBO/EBO)
 *         ↓
 *     渲染
 *
 * ============================================================================
 * Assimp的数据结构
 * ============================================================================
 *
 *   aiScene（场景根节点）
 *     ├── mMeshes[]        所有网格数据
 *     ├── mMaterials[]     所有材质
 *     ├── mAnimations[]    所有动画（第25章用）
 *     └── mRootNode        场景节点树
 *           ├── mChildren[]    子节点
 *           └── mMeshes[]      引用的网格索引
 *
 *   aiMesh（单个网格）
 *     ├── mVertices[]      顶点位置
 *     ├── mNormals[]       法线
 *     ├── mTextureCoords[] UV坐标
 *     ├── mFaces[]         面（三角形索引）
 *     └── mBones[]         骨骼（第25章用）
 */

#pragma once

#include "engine3d/Mesh3D.h"
#include "math/Math.h"
#include <string>
#include <vector>
#include <iostream>

// Assimp头文件
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace MiniEngine {

using Math::Vec2;
using Math::Vec3;

/**
 * 加载的模型数据（可包含多个Mesh）
 */
struct Model3D {
    std::vector<Mesh3D> meshes;
    std::string name;
    std::string directory; // 模型文件所在目录（用于加载贴图）
    
    // 模型整体变换
    Vec3 position = Vec3(0, 0, 0);
    Vec3 rotation = Vec3(0, 0, 0);
    Vec3 scale = Vec3(1, 1, 1);
    
    // 统计
    int totalVertices = 0;
    int totalTriangles = 0;
    
    void Draw() const {
        for (auto& mesh : meshes) {
            mesh.Draw();
        }
    }
    
    void Destroy() {
        for (auto& mesh : meshes) {
            mesh.Destroy();
        }
        meshes.clear();
    }
    
    bool IsValid() const { return !meshes.empty(); }
};

/**
 * 模型加载器
 */
class ModelLoader {
public:
    /**
     * 从文件加载模型
     *
     * @param path 文件路径（支持OBJ/FBX/GLTF等）
     * @param normalize 是否归一化到单位大小
     * @return 加载的模型数据
     */
    static Model3D Load(const std::string& path, bool normalize = true) {
        Model3D model;
        
        Assimp::Importer importer;
        
        // 后处理标志：
        // aiProcess_Triangulate     — 将所有面转为三角形
        // aiProcess_GenNormals      — 如果没有法线就自动生成
        // aiProcess_FlipUVs         — 翻转UV的Y轴（OpenGL需要）
        // aiProcess_CalcTangentSpace — 计算切线空间（法线贴图用）
        unsigned int flags = aiProcess_Triangulate 
                           | aiProcess_GenNormals 
                           | aiProcess_FlipUVs
                           | aiProcess_CalcTangentSpace;
        
        const aiScene* scene = importer.ReadFile(path, flags);
        
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "[ModelLoader] Failed to load: " << path << std::endl;
            std::cerr << "[ModelLoader] Error: " << importer.GetErrorString() << std::endl;
            return model;
        }
        
        // 提取目录路径
        size_t lastSlash = path.find_last_of("/\\");
        model.directory = (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "";
        model.name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        
        // 递归处理节点
        ProcessNode(scene->mRootNode, scene, model);
        
        // 归一化模型大小
        if (normalize && model.totalVertices > 0) {
            // 已经在ProcessMesh中收集了所有顶点，这里不重新计算
            // normalize在ProcessNode后单独处理
        }
        
        std::cout << "[ModelLoader] Loaded: " << model.name 
                  << " | Meshes: " << model.meshes.size()
                  << " | Vertices: " << model.totalVertices
                  << " | Triangles: " << model.totalTriangles << std::endl;
        
        return model;
    }
    
    /**
     * 尝试多个路径加载模型
     */
    static Model3D LoadWithFallback(const std::string& relativePath, bool normalize = true) {
        std::string paths[] = {
            "data/models/" + relativePath,
            "../data/models/" + relativePath,
            "../../data/models/" + relativePath,
            relativePath
        };
        for (auto& p : paths) {
            Model3D m = Load(p, normalize);
            if (m.IsValid()) return m;
        }
        std::cerr << "[ModelLoader] All paths failed for: " << relativePath << std::endl;
        return Model3D();
    }

private:
    /**
     * 递归处理场景节点
     */
    static void ProcessNode(aiNode* node, const aiScene* scene, Model3D& model) {
        // 处理当前节点引用的所有Mesh
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            model.meshes.push_back(ProcessMesh(mesh, scene, model));
        }
        
        // 递归处理子节点
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene, model);
        }
    }
    
    /**
     * 将aiMesh转换为引擎的Mesh3D
     */
    static Mesh3D ProcessMesh(aiMesh* mesh, const aiScene* scene, Model3D& model) {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        // 提取顶点数据
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex3D v;
            
            // 位置
            v.position = Vec3(mesh->mVertices[i].x,
                             mesh->mVertices[i].y,
                             mesh->mVertices[i].z);
            
            // 法线
            if (mesh->mNormals) {
                v.normal = Vec3(mesh->mNormals[i].x,
                               mesh->mNormals[i].y,
                               mesh->mNormals[i].z);
            }
            
            // UV坐标（取第一组）
            if (mesh->mTextureCoords[0]) {
                v.texCoord = Vec2(mesh->mTextureCoords[0][i].x,
                                 mesh->mTextureCoords[0][i].y);
            }
            
            // 顶点颜色（如果有）
            if (mesh->mColors[0]) {
                v.color = Vec3(mesh->mColors[0][i].r,
                              mesh->mColors[0][i].g,
                              mesh->mColors[0][i].b);
            } else {
                // 用法线方向生成伪颜色（方便可视化）
                v.color = Vec3(
                    std::abs(v.normal.x) * 0.5f + 0.5f,
                    std::abs(v.normal.y) * 0.5f + 0.5f,
                    std::abs(v.normal.z) * 0.5f + 0.5f
                );
            }
            
            vertices.push_back(v);
        }
        
        // 提取索引数据
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }
        
        // 更新统计
        model.totalVertices += (int)vertices.size();
        model.totalTriangles += (int)indices.size() / 3;
        
        // 创建GPU Mesh
        Mesh3D result;
        result.Create(vertices, indices);
        return result;
    }
};

} // namespace MiniEngine
