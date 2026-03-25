/**
 * Tilemap.h - 瓦片地图
 * 
 * ============================================================================
 * 概述
 * ============================================================================
 * 
 * Tilemap管理完整的瓦片地图，包括：
 * - 多个图层
 * - 瓦片碰撞检测
 * - 地图渲染
 * 
 * ============================================================================
 * 使用示例
 * ============================================================================
 * 
 *   Tilemap map;
 *   map.Create(20, 15, 32);  // 20x15的地图，每个瓦片32像素
 *   map.SetTileset(tileset);
 *   
 *   // 添加图层
 *   int groundLayer = map.AddLayer("ground", true);  // 有碰撞
 *   int bgLayer = map.AddLayer("background", false); // 无碰撞
 *   
 *   // 设置瓦片
 *   map.SetTile(groundLayer, 5, 0, 1);  // 设置地面
 */

#ifndef MINI_ENGINE_TILEMAP_H
#define MINI_ENGINE_TILEMAP_H

#include "Tileset.h"
#include "math/Math.h"
#include "physics/AABB.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace MiniEngine {

// ============================================================================
// 瓦片图层
// ============================================================================

/**
 * 单个瓦片图层
 */
struct TilemapLayer {
    std::string name;               // 图层名称
    std::vector<int> data;          // 瓦片数据（一维数组）
    int width = 0;                  // 图层宽度（瓦片数）
    int height = 0;                 // 图层高度
    bool visible = true;            // 是否可见
    bool collision = false;         // 是否参与碰撞
    float opacity = 1.0f;           // 透明度
    Math::Vec2 offset;              // 图层偏移
    
    /**
     * 获取指定位置的瓦片ID
     * @return 瓦片ID，如果越界返回0
     */
    int GetTile(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            return 0;
        }
        return data[y * width + x];
    }
    
    /**
     * 设置指定位置的瓦片
     */
    void SetTile(int x, int y, int tileID) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            data[y * width + x] = tileID;
        }
    }
};

// ============================================================================
// 地图对象
// ============================================================================

/**
 * 地图对象（用于放置敌人、道具等）
 */
struct MapObject {
    std::string name;               // 对象名称
    std::string type;               // 对象类型
    float x = 0;                    // X坐标（像素）
    float y = 0;                    // Y坐标
    float width = 0;                // 宽度
    float height = 0;               // 高度
    std::unordered_map<std::string, std::string> properties;  // 自定义属性
};

// ============================================================================
// 瓦片地图类
// ============================================================================

/**
 * 瓦片地图
 */
class Tilemap {
public:
    // ========================================================================
    // 构造和析构
    // ========================================================================
    
    Tilemap();
    ~Tilemap() = default;
    
    // ========================================================================
    // 创建和加载
    // ========================================================================
    
    /**
     * 创建空地图
     * 
     * @param width 宽度（瓦片数）
     * @param height 高度（瓦片数）
     * @param tileSize 瓦片尺寸（像素）
     */
    void Create(int width, int height, int tileSize);
    
    /**
     * 从JSON文件加载（Tiled格式）
     * 
     * @param filename JSON文件路径
     * @return 成功返回true
     */
    bool LoadFromJSON(const std::string& filename);
    
    /**
     * 从简单的数组数据创建
     * 
     * @param data 瓦片数据
     * @param width 宽度
     * @param height 高度
     * @param tileSize 瓦片尺寸
     */
    void LoadFromData(const std::vector<int>& data, int width, int height, int tileSize);
    
    // ========================================================================
    // 图层管理
    // ========================================================================
    
    /**
     * 添加图层
     * 
     * @param name 图层名称
     * @param hasCollision 是否有碰撞
     * @return 图层索引
     */
    int AddLayer(const std::string& name, bool hasCollision = false);
    
    /**
     * 获取图层
     */
    TilemapLayer* GetLayer(int index);
    TilemapLayer* GetLayer(const std::string& name);
    const TilemapLayer* GetLayer(int index) const;
    const TilemapLayer* GetLayer(const std::string& name) const;
    
    /**
     * 获取图层数量
     */
    int GetLayerCount() const { return (int)m_layers.size(); }
    
    // ========================================================================
    // 瓦片操作
    // ========================================================================
    
    /**
     * 设置瓦片
     */
    void SetTile(int layerIndex, int x, int y, int tileID);
    
    /**
     * 获取瓦片
     */
    int GetTile(int layerIndex, int x, int y) const;
    
    /**
     * 获取碰撞层的瓦片
     * 
     * 遍历所有碰撞层，返回第一个非0的瓦片ID
     */
    int GetCollisionTile(int x, int y) const;
    
    // ========================================================================
    // 图集
    // ========================================================================
    
    /**
     * 设置瓦片图集
     */
    void SetTileset(std::shared_ptr<Tileset> tileset);
    
    /**
     * 添加瓦片图集（支持多图集）
     */
    void AddTileset(std::shared_ptr<Tileset> tileset);
    
    /**
     * 根据瓦片ID获取对应的图集
     */
    Tileset* GetTilesetForGID(int gid);
    
    // ========================================================================
    // 属性
    // ========================================================================
    
    /** 获取地图宽度（瓦片数） */
    int GetWidth() const { return m_width; }
    
    /** 获取地图高度（瓦片数） */
    int GetHeight() const { return m_height; }
    
    /** 获取瓦片宽度（像素） */
    int GetTileWidth() const { return m_tileWidth; }
    
    /** 获取瓦片高度（像素） */
    int GetTileHeight() const { return m_tileHeight; }
    
    /** 获取地图像素宽度 */
    int GetPixelWidth() const { return m_width * m_tileWidth; }
    
    /** 获取地图像素高度 */
    int GetPixelHeight() const { return m_height * m_tileHeight; }
    
    // ========================================================================
    // 坐标转换
    // ========================================================================
    
    /**
     * 世界坐标转瓦片坐标
     */
    void WorldToTile(float worldX, float worldY, int& tileX, int& tileY) const;
    
    /**
     * 瓦片坐标转世界坐标（左下角）
     */
    void TileToWorld(int tileX, int tileY, float& worldX, float& worldY) const;
    
    /**
     * 获取瓦片的AABB（用于碰撞）
     */
    AABB GetTileAABB(int tileX, int tileY) const;
    
    // ========================================================================
    // 碰撞检测
    // ========================================================================
    
    /**
     * 检查位置是否是实心瓦片
     */
    bool IsSolid(int tileX, int tileY) const;
    
    /**
     * 检查AABB是否与任何碰撞瓦片相交
     */
    bool CheckCollision(const AABB& aabb) const;
    
    /**
     * 获取与AABB相交的所有碰撞瓦片
     */
    std::vector<AABB> GetCollidingTiles(const AABB& aabb) const;
    
    /**
     * 设置哪些瓦片ID是实心的
     * 默认情况下，非0的瓦片都是实心
     */
    void SetSolidTiles(const std::vector<int>& solidTileIDs);
    
    /**
     * 设置是否所有非0瓦片都实心
     */
    void SetAllNonZeroSolid(bool solid) { m_allNonZeroSolid = solid; }
    
    // ========================================================================
    // 对象层
    // ========================================================================
    
    /**
     * 获取所有地图对象
     */
    const std::vector<MapObject>& GetObjects() const { return m_objects; }
    
    /**
     * 按名称获取对象
     */
    const MapObject* GetObject(const std::string& name) const;
    
    /**
     * 按类型获取所有对象
     */
    std::vector<const MapObject*> GetObjectsByType(const std::string& type) const;
    
    /**
     * 添加对象
     */
    void AddObject(const MapObject& obj);
    
private:
    int m_width = 0;
    int m_height = 0;
    int m_tileWidth = 32;
    int m_tileHeight = 32;
    
    std::vector<TilemapLayer> m_layers;
    std::vector<std::shared_ptr<Tileset>> m_tilesets;
    std::vector<MapObject> m_objects;
    
    // 碰撞设置
    std::vector<int> m_solidTileIDs;
    bool m_allNonZeroSolid = true;
    
    /**
     * 检查瓦片ID是否实心
     */
    bool IsTileSolid(int tileID) const;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_TILEMAP_H
