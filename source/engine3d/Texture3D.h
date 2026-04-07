/**
 * Texture3D.h - 3D渲染用纹理加载
 *
 * 封装OpenGL纹理创建和绑定，支持多纹理单元。
 *
 * 纹理单元(Texture Unit)：
 *   OpenGL可以同时绑定多个纹理到不同的"插槽"
 *   GL_TEXTURE0 = 漫反射贴图
 *   GL_TEXTURE1 = 法线贴图
 *   GL_TEXTURE2 = 粗糙度贴图
 *   Shader中用sampler2D对应的int值来选择纹理单元
 */

#pragma once

#include <glad/gl.h>
#include <string>
#include <iostream>

// stb_image由引擎全局定义STB_IMAGE_IMPLEMENTATION
// 这里只需要声明
#ifndef STB_IMAGE_INCLUDE_GUARD
#define STB_IMAGE_INCLUDE_GUARD
extern "C" {
    unsigned char* stbi_load(const char*, int*, int*, int*, int);
    void stbi_image_free(void*);
    void stbi_set_flip_vertically_on_load(int);
}
#endif

namespace MiniEngine {

class Texture3D {
public:
    Texture3D() = default;
    ~Texture3D() { Destroy(); }
    
    Texture3D(const Texture3D&) = delete;
    Texture3D& operator=(const Texture3D&) = delete;
    Texture3D(Texture3D&& o) noexcept : m_id(o.m_id), m_w(o.m_w), m_h(o.m_h) { o.m_id=0; }
    Texture3D& operator=(Texture3D&& o) noexcept { if(this!=&o){Destroy();m_id=o.m_id;m_w=o.m_w;m_h=o.m_h;o.m_id=0;} return *this; }
    
    /**
     * 从文件加载纹理
     */
    bool Load(const std::string& path, bool flipY = true) {
        if (flipY) stbi_set_flip_vertically_on_load(1);
        else stbi_set_flip_vertically_on_load(0);
        
        int channels;
        unsigned char* data = stbi_load(path.c_str(), &m_w, &m_h, &channels, 0);
        if (!data) {
            // 尝试备用路径
            std::string altPaths[] = {"../"+path, "../../"+path};
            for (auto& p : altPaths) {
                data = stbi_load(p.c_str(), &m_w, &m_h, &channels, 0);
                if (data) break;
            }
        }
        if (!data) {
            std::cerr << "[Texture3D] Failed to load: " << path << std::endl;
            return false;
        }
        
        GLenum format = GL_RGB;
        if (channels == 1) format = GL_RED;
        else if (channels == 3) format = GL_RGB;
        else if (channels == 4) format = GL_RGBA;
        
        Destroy();
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_2D, m_id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, m_w, m_h, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // 纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
        return true;
    }
    
    /**
     * 绑定到指定纹理单元
     * @param unit 纹理单元编号（0=GL_TEXTURE0, 1=GL_TEXTURE1, ...）
     */
    void Bind(int unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }
    
    void Destroy() {
        if (m_id) { glDeleteTextures(1, &m_id); m_id = 0; }
    }
    
    bool IsValid() const { return m_id != 0; }
    int GetWidth() const { return m_w; }
    int GetHeight() const { return m_h; }
    unsigned int GetID() const { return m_id; }
    
private:
    unsigned int m_id = 0;
    int m_w = 0, m_h = 0;
};

/**
 * 材质（一组贴图+参数）
 */
struct Material3D {
    Texture3D diffuseMap;
    Texture3D normalMap;
    Texture3D roughnessMap;
    
    float shininess = 32.0f;
    float tiling = 1.0f;     // 纹理平铺次数
    
    bool hasDiffuse = false;
    bool hasNormal = false;
    bool hasRoughness = false;
    
    std::string name;
    
    /**
     * 从目录加载材质（期望目录下有diffuse.png/normal.png/roughness.png）
     */
    bool LoadFromDirectory(const std::string& dir, const std::string& matName = "") {
        name = matName.empty() ? dir : matName;
        
        std::string paths[] = {dir, "data/texture/materials/"+dir, "../data/texture/materials/"+dir, "../../data/texture/materials/"+dir};
        
        for (auto& base : paths) {
            if (diffuseMap.Load(base + "/diffuse.png")) {
                hasDiffuse = true;
                hasNormal = normalMap.Load(base + "/normal.png");
                hasRoughness = roughnessMap.Load(base + "/roughness.png");
                std::cout << "[Material] Loaded: " << name 
                          << " (D:" << (hasDiffuse?"Y":"N")
                          << " N:" << (hasNormal?"Y":"N")
                          << " R:" << (hasRoughness?"Y":"N") << ")" << std::endl;
                return true;
            }
        }
        std::cerr << "[Material] Failed to load: " << dir << std::endl;
        return false;
    }
    
    /**
     * 绑定贴图到Shader的纹理单元
     */
    void Bind() const {
        if (hasDiffuse) diffuseMap.Bind(0);    // GL_TEXTURE0
        if (hasNormal) normalMap.Bind(1);       // GL_TEXTURE1
        if (hasRoughness) roughnessMap.Bind(2); // GL_TEXTURE2
    }
    
    void Destroy() {
        diffuseMap.Destroy();
        normalMap.Destroy();
        roughnessMap.Destroy();
    }
};

} // namespace MiniEngine
