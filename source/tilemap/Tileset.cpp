/**
 * Tileset.cpp - 瓦片图集实现
 */

#include "Tileset.h"

namespace MiniEngine {

Tileset::Tileset()
    : m_tileWidth(32)
    , m_tileHeight(32)
    , m_columns(1)
    , m_rows(1)
    , m_margin(0)
    , m_spacing(0)
    , m_firstGID(1)
{
}

void Tileset::Initialize(std::shared_ptr<Texture> texture,
                          int tileWidth, int tileHeight,
                          int margin, int spacing) {
    m_texture = texture;
    m_tileWidth = tileWidth;
    m_tileHeight = tileHeight;
    m_margin = margin;
    m_spacing = spacing;
    
    // 计算列数和行数
    if (m_texture) {
        int texWidth = m_texture->GetWidth();
        int texHeight = m_texture->GetHeight();
        
        // 考虑边距和间距
        // columns = (texWidth - 2*margin + spacing) / (tileWidth + spacing)
        m_columns = (texWidth - 2 * m_margin + m_spacing) / (m_tileWidth + m_spacing);
        m_rows = (texHeight - 2 * m_margin + m_spacing) / (m_tileHeight + m_spacing);
        
        if (m_columns < 1) m_columns = 1;
        if (m_rows < 1) m_rows = 1;
    }
}

void Tileset::SetTileCount(int columns, int rows) {
    m_columns = columns;
    m_rows = rows;
}

bool Tileset::GetTileUV(int tileID, Math::Vec2& uvMin, Math::Vec2& uvMax) const {
    if (tileID < 0 || tileID >= m_columns * m_rows) {
        return false;
    }
    
    if (!m_texture) {
        return false;
    }
    
    /**
     * UV坐标计算
     * 
     * 瓦片在图集中的像素位置：
     *   pixelX = margin + col * (tileWidth + spacing)
     *   pixelY = margin + row * (tileHeight + spacing)
     * 
     * 转换为UV：
     *   u = pixelX / textureWidth
     *   v = pixelY / textureHeight
     * 
     * 注意：UV的V轴是从下往上的，但图片的Y轴是从上往下的
     * 需要翻转V坐标
     */
    
    int col = tileID % m_columns;
    int row = tileID / m_columns;
    
    float texWidth = (float)m_texture->GetWidth();
    float texHeight = (float)m_texture->GetHeight();
    
    // 瓦片的像素位置（左上角）
    float pixelX = m_margin + col * (m_tileWidth + m_spacing);
    float pixelY = m_margin + row * (m_tileHeight + m_spacing);
    
    // 转换为UV（图片坐标系，Y从上往下）
    float u0 = pixelX / texWidth;
    float v0 = pixelY / texHeight;
    float u1 = (pixelX + m_tileWidth) / texWidth;
    float v1 = (pixelY + m_tileHeight) / texHeight;
    
    // 翻转V坐标（UV坐标系Y从下往上）
    // 注意：OpenGL纹理的V=0在底部
    // 但我们加载图片时通常已经翻转了，所以这里可能不需要翻转
    // 具体取决于你的纹理加载方式
    
    // 如果使用stb_image默认加载（不翻转），则需要翻转UV
    // v0_flipped = 1 - v1
    // v1_flipped = 1 - v0
    
    uvMin.x = u0;
    uvMin.y = 1.0f - v1;  // 翻转V
    uvMax.x = u1;
    uvMax.y = 1.0f - v0;  // 翻转V
    
    return true;
}

bool Tileset::GetTileUVByGID(int globalID, Math::Vec2& uvMin, Math::Vec2& uvMax) const {
    if (!ContainsGID(globalID)) {
        return false;
    }
    
    int localID = globalID - m_firstGID;
    return GetTileUV(localID, uvMin, uvMax);
}

bool Tileset::ContainsGID(int globalID) const {
    if (globalID < m_firstGID) return false;
    if (globalID >= m_firstGID + GetTileCount()) return false;
    return true;
}

} // namespace MiniEngine
