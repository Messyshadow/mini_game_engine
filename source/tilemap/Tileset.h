/**
 * Tileset.h - 瓦片图集
 * 
 * ============================================================================
 * 概述
 * ============================================================================
 * 
 * 瓦片图集管理一张包含多个瓦片的纹理图片。
 * 它负责：
 * 1. 存储瓦片的尺寸和数量
 * 2. 计算指定瓦片ID的UV坐标
 * 
 * ============================================================================
 * 使用示例
 * ============================================================================
 * 
 *   Tileset tileset;
 *   tileset.Initialize(texture, 32, 32);  // 32x32像素的瓦片
 *   
 *   Vec2 uvMin, uvMax;
 *   tileset.GetTileUV(5, uvMin, uvMax);  // 获取瓦片5的UV
 */

#ifndef MINI_ENGINE_TILESET_H
#define MINI_ENGINE_TILESET_H

#include "engine/Texture.h"
#include "math/Math.h"
#include <memory>
#include <string>

namespace MiniEngine {

/**
 * 瓦片图集类
 */
class Tileset {
public:
    // ========================================================================
    // 构造和析构
    // ========================================================================
    
    Tileset();
    ~Tileset() = default;
    
    // ========================================================================
    // 初始化
    // ========================================================================
    
    /**
     * 初始化瓦片图集
     * 
     * @param texture 纹理
     * @param tileWidth 单个瓦片宽度（像素）
     * @param tileHeight 单个瓦片高度（像素）
     * @param margin 图集边缘留白（像素）
     * @param spacing 瓦片之间间距（像素）
     */
    void Initialize(std::shared_ptr<Texture> texture, 
                    int tileWidth, int tileHeight,
                    int margin = 0, int spacing = 0);
    
    /**
     * 设置瓦片数量（如果不能从纹理尺寸计算）
     */
    void SetTileCount(int columns, int rows);
    
    // ========================================================================
    // 属性访问
    // ========================================================================
    
    /** 获取纹理 */
    std::shared_ptr<Texture> GetTexture() const { return m_texture; }
    
    /** 获取瓦片宽度 */
    int GetTileWidth() const { return m_tileWidth; }
    
    /** 获取瓦片高度 */
    int GetTileHeight() const { return m_tileHeight; }
    
    /** 获取列数 */
    int GetColumns() const { return m_columns; }
    
    /** 获取行数 */
    int GetRows() const { return m_rows; }
    
    /** 获取瓦片总数 */
    int GetTileCount() const { return m_columns * m_rows; }
    
    /** 获取Tiled的firstgid */
    int GetFirstGID() const { return m_firstGID; }
    
    /** 设置Tiled的firstgid */
    void SetFirstGID(int gid) { m_firstGID = gid; }
    
    /** 获取图集名称 */
    const std::string& GetName() const { return m_name; }
    
    /** 设置图集名称 */
    void SetName(const std::string& name) { m_name = name; }
    
    // ========================================================================
    // UV坐标计算
    // ========================================================================
    
    /**
     * 获取瓦片的UV坐标
     * 
     * @param tileID 瓦片索引（从0开始）
     * @param uvMin 输出：UV左下角
     * @param uvMax 输出：UV右上角
     * @return 如果tileID有效返回true
     * 
     * 注意：此方法处理了UV的Y轴翻转
     */
    bool GetTileUV(int tileID, Math::Vec2& uvMin, Math::Vec2& uvMax) const;
    
    /**
     * 根据Tiled的全局ID获取UV坐标
     * 
     * Tiled中每个瓦片有一个全局ID (GID)
     * GID = firstgid + 本地tileID
     * 
     * @param globalID Tiled的全局瓦片ID
     */
    bool GetTileUVByGID(int globalID, Math::Vec2& uvMin, Math::Vec2& uvMax) const;
    
    /**
     * 检查GID是否属于此图集
     */
    bool ContainsGID(int globalID) const;
    
private:
    std::shared_ptr<Texture> m_texture;
    std::string m_name;
    
    int m_tileWidth = 32;
    int m_tileHeight = 32;
    int m_columns = 1;
    int m_rows = 1;
    int m_margin = 0;
    int m_spacing = 0;
    int m_firstGID = 1;  // Tiled的firstgid，默认为1
};

} // namespace MiniEngine

#endif // MINI_ENGINE_TILESET_H
