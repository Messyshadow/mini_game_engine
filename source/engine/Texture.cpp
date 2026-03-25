/**
 * Texture.cpp - 纹理类实现
 * 
 * 使用 stb_image 库加载图片
 * stb_image 是一个单头文件库，非常适合游戏开发
 */

// ============================================================================
// stb_image 配置
// ============================================================================

// 定义这个宏来创建实现代码
// 只能在一个 .cpp 文件中定义！
#define STB_IMAGE_IMPLEMENTATION

// 包含 stb_image
// 这个头文件会根据上面的宏定义来决定是只声明还是包含实现
#include "stb_image.h"

#include "Texture.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

namespace MiniEngine {

// ============================================================================
// 构造和析构
// ============================================================================

Texture::Texture() 
    : m_textureID(0)
    , m_width(0)
    , m_height(0)
    , m_channels(0) 
{
}

Texture::~Texture() {
    Release();
}

Texture::Texture(Texture&& other) noexcept
    : m_textureID(other.m_textureID)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_channels(other.m_channels)
    , m_filepath(std::move(other.m_filepath))
{
    // 转移所有权后，将原对象置空
    other.m_textureID = 0;
    other.m_width = 0;
    other.m_height = 0;
    other.m_channels = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        // 先释放当前资源
        Release();
        
        // 转移资源
        m_textureID = other.m_textureID;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        m_filepath = std::move(other.m_filepath);
        
        // 置空原对象
        other.m_textureID = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_channels = 0;
    }
    return *this;
}

// ============================================================================
// 加载纹理
// ============================================================================

bool Texture::Load(const std::string& filepath, bool flipY) {
    /**
     * 加载纹理的完整流程：
     * 
     * 1. 使用 stb_image 从文件加载图片数据
     * 2. 创建 OpenGL 纹理对象
     * 3. 上传图片数据到 GPU
     * 4. 设置纹理参数（过滤、环绕）
     * 5. 释放 CPU 端的图片数据
     */
    
    // 先释放旧的纹理（如果有）
    Release();
    
    // 保存文件路径
    m_filepath = filepath;
    
    // ------------------------------------------------------------------------
    // 步骤1：使用 stb_image 加载图片
    // ------------------------------------------------------------------------
    
    // 设置是否垂直翻转
    // OpenGL的纹理坐标原点在左下角，但大多数图片格式原点在左上角
    // 所以通常需要翻转
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    
    // 加载图片
    // 参数：文件路径, 宽度输出, 高度输出, 通道数输出, 期望的通道数(0=自动)
    unsigned char* data = stbi_load(
        filepath.c_str(),   // 文件路径
        &m_width,           // 输出：图片宽度
        &m_height,          // 输出：图片高度
        &m_channels,        // 输出：图片通道数
        0                   // 0 = 保持原始通道数
    );
    
    // 检查是否加载成功
    if (!data) {
        std::cerr << "[Texture] Failed to load: " << filepath << std::endl;
        std::cerr << "  Reason: " << stbi_failure_reason() << std::endl;
        return false;
    }
    
    std::cout << "[Texture] Loaded: " << filepath 
              << " (" << m_width << "x" << m_height 
              << ", " << m_channels << " channels)" << std::endl;
    
    // ------------------------------------------------------------------------
    // 步骤2-4：创建 OpenGL 纹理并上传数据
    // ------------------------------------------------------------------------
    
    bool success = LoadFromMemory(data, m_width, m_height, m_channels);
    
    // ------------------------------------------------------------------------
    // 步骤5：释放 CPU 端数据
    // ------------------------------------------------------------------------
    
    // stb_image 分配的内存需要用 stbi_image_free 释放
    stbi_image_free(data);
    
    return success;
}

bool Texture::LoadFromMemory(const unsigned char* data, int width, int height, int channels) {
    /**
     * 从内存创建纹理的步骤：
     * 
     * 1. glGenTextures  - 生成纹理ID
     * 2. glBindTexture  - 绑定纹理
     * 3. glTexImage2D   - 上传数据到GPU
     * 4. glTexParameter - 设置参数
     * 5. glGenerateMipmap - 生成多级纹理（可选）
     */
    
    // 先释放旧的
    Release();
    
    m_width = width;
    m_height = height;
    m_channels = channels;
    
    // 根据通道数确定OpenGL格式
    // GL_RGB  = 3通道（红绿蓝）
    // GL_RGBA = 4通道（红绿蓝+透明度）
    GLenum internalFormat;  // GPU内部存储格式
    GLenum dataFormat;      // 输入数据格式
    
    if (channels == 4) {
        internalFormat = GL_RGBA8;  // 8位RGBA
        dataFormat = GL_RGBA;
    } else if (channels == 3) {
        internalFormat = GL_RGB8;   // 8位RGB
        dataFormat = GL_RGB;
    } else if (channels == 1) {
        internalFormat = GL_R8;     // 8位单通道（灰度）
        dataFormat = GL_RED;
    } else {
        std::cerr << "[Texture] Unsupported channel count: " << channels << std::endl;
        return false;
    }
    
    // ------------------------------------------------------------------------
    // 创建纹理对象
    // ------------------------------------------------------------------------
    
    // 生成一个纹理ID
    glGenTextures(1, &m_textureID);
    
    // 绑定到 GL_TEXTURE_2D 目标
    // 后续的纹理操作都会作用于这个纹理
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    
    // ------------------------------------------------------------------------
    // 上传数据到GPU
    // ------------------------------------------------------------------------
    
    /**
     * glTexImage2D 参数说明：
     * 
     * 1. target       - 纹理目标，2D纹理用 GL_TEXTURE_2D
     * 2. level        - Mipmap级别，0是基础级别
     * 3. internalFormat - GPU内部存储格式
     * 4. width        - 宽度
     * 5. height       - 高度
     * 6. border       - 必须是0（历史遗留参数）
     * 7. format       - 输入数据的格式
     * 8. type         - 输入数据的类型
     * 9. data         - 像素数据指针
     */
    glTexImage2D(
        GL_TEXTURE_2D,      // 目标
        0,                  // Mipmap级别
        internalFormat,     // 内部格式
        width,              // 宽度
        height,             // 高度
        0,                  // 边框（必须为0）
        dataFormat,         // 数据格式
        GL_UNSIGNED_BYTE,   // 数据类型（每通道8位）
        data                // 像素数据
    );
    
    // ------------------------------------------------------------------------
    // 设置纹理参数
    // ------------------------------------------------------------------------
    
    // 设置过滤模式
    // GL_LINEAR 适合大多数情况
    // 如果是像素艺术游戏，改用 GL_NEAREST
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // 设置环绕模式
    // GL_CLAMP_TO_EDGE 防止边缘出现接缝
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // 解绑纹理（可选，但是好习惯）
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return true;
}

bool Texture::CreateSolidColor(const Math::Vec4& color, int width, int height) {
    /**
     * 创建纯色纹理
     * 常用于：
     * - 作为默认纹理（当图片加载失败时）
     * - 调试用的占位纹理
     * - 纯色填充
     */
    
    // 计算像素数
    int pixelCount = width * height;
    
    // 分配像素数据（RGBA，每像素4字节）
    std::vector<unsigned char> data(pixelCount * 4);
    
    // 将颜色转换为0-255范围
    unsigned char r = static_cast<unsigned char>(color.x * 255);
    unsigned char g = static_cast<unsigned char>(color.y * 255);
    unsigned char b = static_cast<unsigned char>(color.z * 255);
    unsigned char a = static_cast<unsigned char>(color.w * 255);
    
    // 填充所有像素
    for (int i = 0; i < pixelCount; i++) {
        data[i * 4 + 0] = r;
        data[i * 4 + 1] = g;
        data[i * 4 + 2] = b;
        data[i * 4 + 3] = a;
    }
    
    return LoadFromMemory(data.data(), width, height, 4);
}

// ============================================================================
// 使用纹理
// ============================================================================

void Texture::Bind(unsigned int slot) const {
    /**
     * 绑定纹理到指定的纹理单元
     * 
     * OpenGL有多个纹理单元（通常至少16个），
     * 可以同时绑定多个纹理。
     * 
     * 在着色器中，通过 sampler2D 的值来选择纹理单元：
     *   uniform sampler2D uTexture;  // 在CPU端设置为0
     *   texture(uTexture, texCoord); // 从单元0采样
     */
    
    // 激活指定的纹理单元
    // GL_TEXTURE0, GL_TEXTURE1, ... GL_TEXTURE15
    glActiveTexture(GL_TEXTURE0 + slot);
    
    // 将纹理绑定到当前激活的纹理单元
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void Texture::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// 设置参数
// ============================================================================

void Texture::SetFilter(TextureFilter minFilter, TextureFilter magFilter) {
    if (m_textureID == 0) return;
    
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FilterToGL(minFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FilterToGL(magFilter));
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::SetWrap(TextureWrap wrapS, TextureWrap wrapT) {
    if (m_textureID == 0) return;
    
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WrapToGL(wrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WrapToGL(wrapT));
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// 私有方法
// ============================================================================

void Texture::Release() {
    // 检查OpenGL上下文是否还有效
    if (glfwGetCurrentContext() == nullptr) {
        m_textureID = 0;
        return;
    }
    
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }
    
    m_width = 0;
    m_height = 0;
    m_channels = 0;
    m_filepath.clear();
}

unsigned int Texture::FilterToGL(TextureFilter filter) {
    switch (filter) {
        case TextureFilter::Nearest: return GL_NEAREST;
        case TextureFilter::Linear:  return GL_LINEAR;
        default: return GL_LINEAR;
    }
}

unsigned int Texture::WrapToGL(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        default: return GL_CLAMP_TO_EDGE;
    }
}

} // namespace MiniEngine
