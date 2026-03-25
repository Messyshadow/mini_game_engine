/**
 * Texture.h - 纹理类
 * 
 * ============================================================================
 * 纹理是什么？
 * ============================================================================
 * 
 * 纹理（Texture）是存储在GPU显存中的图像数据。
 * 
 * 在游戏中，纹理用于：
 * - 精灵（Sprite）：2D游戏中的角色、道具、背景
 * - 贴图：3D模型表面的材质
 * - UI元素：按钮、图标、背景
 * 
 * ============================================================================
 * 纹理坐标（UV坐标）
 * ============================================================================
 * 
 * UV坐标用于指定图片的哪个部分映射到哪个顶点。
 * 
 *   U轴（水平）：0 = 左边，1 = 右边
 *   V轴（垂直）：0 = 底部，1 = 顶部（OpenGL约定）
 * 
 *      (0,1) ──────────── (1,1)
 *        │                  │
 *        │     纹理图片      │
 *        │                  │
 *      (0,0) ──────────── (1,0)
 * 
 * 注意：很多图片格式（PNG、JPG）的Y轴是从上到下的，
 *      所以加载时需要翻转！
 * 
 * ============================================================================
 * 纹理过滤（Filtering）
 * ============================================================================
 * 
 * 当纹理被放大或缩小显示时，需要决定如何采样：
 * 
 * 1. GL_NEAREST（最近邻）：
 *    - 选择最近的像素，不做插值
 *    - 效果：像素风格，有锯齿
 *    - 适合：像素艺术游戏
 * 
 * 2. GL_LINEAR（线性插值）：
 *    - 对周围像素进行加权平均
 *    - 效果：平滑，但可能模糊
 *    - 适合：写实风格游戏
 * 
 * ============================================================================
 * 纹理环绕（Wrapping）
 * ============================================================================
 * 
 * 当UV坐标超出 [0,1] 范围时的处理方式：
 * 
 * 1. GL_REPEAT：重复纹理（默认）
 * 2. GL_CLAMP_TO_EDGE：拉伸边缘像素
 * 3. GL_MIRRORED_REPEAT：镜像重复
 */

#ifndef MINI_ENGINE_TEXTURE_H
#define MINI_ENGINE_TEXTURE_H

#include <string>
#include "math/Math.h"

namespace MiniEngine {

/**
 * 纹理过滤模式
 */
enum class TextureFilter {
    Nearest,  // 最近邻采样（像素风格）
    Linear    // 线性插值（平滑）
};

/**
 * 纹理环绕模式
 */
enum class TextureWrap {
    Repeat,         // 重复
    ClampToEdge,    // 边缘拉伸
    MirroredRepeat  // 镜像重复
};

/**
 * 纹理类
 * 
 * 管理OpenGL纹理对象的加载、绑定和销毁
 */
class Texture {
public:
    // ========================================================================
    // 构造和析构
    // ========================================================================
    
    /**
     * 默认构造函数
     * 创建一个空的纹理对象，需要后续调用Load()加载
     */
    Texture();
    
    /**
     * 析构函数
     * 自动释放GPU上的纹理资源
     */
    ~Texture();
    
    // 禁止拷贝（GPU资源不能简单拷贝）
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    // 允许移动
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    
    // ========================================================================
    // 加载纹理
    // ========================================================================
    
    /**
     * 从文件加载纹理
     * 
     * 支持的格式：PNG, JPG, BMP, TGA, GIF 等（由stb_image支持）
     * 
     * @param filepath 图片文件路径
     * @param flipY 是否垂直翻转（通常需要设为true）
     * @return 加载成功返回true
     * 
     * 示例：
     *   Texture playerTex;
     *   if (!playerTex.Load("data/texture/player.png")) {
     *       // 处理加载失败
     *   }
     */
    bool Load(const std::string& filepath, bool flipY = true);
    
    /**
     * 从内存数据创建纹理
     * 
     * @param data 像素数据（RGBA格式，每像素4字节）
     * @param width 宽度（像素）
     * @param height 高度（像素）
     * @param channels 通道数（3=RGB, 4=RGBA）
     * @return 创建成功返回true
     */
    bool LoadFromMemory(const unsigned char* data, int width, int height, int channels);
    
    /**
     * 创建一个纯色纹理
     * 
     * @param color 颜色
     * @param width 宽度（默认1x1像素）
     * @param height 高度
     * @return 创建成功返回true
     */
    bool CreateSolidColor(const Math::Vec4& color, int width = 1, int height = 1);
    
    // ========================================================================
    // 使用纹理
    // ========================================================================
    
    /**
     * 绑定纹理到指定的纹理单元
     * 
     * OpenGL支持多个纹理同时使用（多重纹理），
     * 每个纹理单元可以绑定一个纹理。
     * 
     * @param slot 纹理单元编号（0-15，通常使用0）
     * 
     * 示例：
     *   texture.Bind(0);  // 绑定到纹理单元0
     *   shader.SetInt("uTexture", 0);  // 告诉着色器使用单元0
     */
    void Bind(unsigned int slot = 0) const;
    
    /**
     * 解绑纹理
     */
    void Unbind() const;
    
    // ========================================================================
    // 设置纹理参数
    // ========================================================================
    
    /**
     * 设置纹理过滤模式
     * 
     * @param minFilter 缩小时的过滤（纹理比屏幕像素小）
     * @param magFilter 放大时的过滤（纹理比屏幕像素大）
     */
    void SetFilter(TextureFilter minFilter, TextureFilter magFilter);
    
    /**
     * 设置纹理环绕模式
     * 
     * @param wrapS 水平方向（U轴）环绕
     * @param wrapT 垂直方向（V轴）环绕
     */
    void SetWrap(TextureWrap wrapS, TextureWrap wrapT);
    
    // ========================================================================
    // 获取属性
    // ========================================================================
    
    /** 获取纹理ID（OpenGL句柄） */
    unsigned int GetID() const { return m_textureID; }
    
    /** 获取宽度（像素） */
    int GetWidth() const { return m_width; }
    
    /** 获取高度（像素） */
    int GetHeight() const { return m_height; }
    
    /** 获取尺寸 */
    Math::Vec2 GetSize() const { return Math::Vec2((float)m_width, (float)m_height); }
    
    /** 获取通道数（3=RGB, 4=RGBA） */
    int GetChannels() const { return m_channels; }
    
    /** 检查纹理是否有效 */
    bool IsValid() const { return m_textureID != 0; }
    
    /** 获取文件路径（如果是从文件加载的） */
    const std::string& GetFilePath() const { return m_filepath; }
    
private:
    // ========================================================================
    // 私有成员
    // ========================================================================
    
    unsigned int m_textureID = 0;   // OpenGL纹理ID
    int m_width = 0;                // 宽度
    int m_height = 0;               // 高度
    int m_channels = 0;             // 通道数
    std::string m_filepath;         // 文件路径
    
    /**
     * 释放纹理资源
     */
    void Release();
    
    /**
     * 将枚举转换为OpenGL常量
     */
    static unsigned int FilterToGL(TextureFilter filter);
    static unsigned int WrapToGL(TextureWrap wrap);
};

} // namespace MiniEngine

#endif // MINI_ENGINE_TEXTURE_H
