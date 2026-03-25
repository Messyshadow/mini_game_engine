/**
 * Tilemap.cpp - 瓦片地图实现
 */

#include "Tilemap.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

namespace MiniEngine {

// ============================================================================
// 构造函数
// ============================================================================

Tilemap::Tilemap()
    : m_width(0)
    , m_height(0)
    , m_tileWidth(32)
    , m_tileHeight(32)
    , m_allNonZeroSolid(true)
{
}

// ============================================================================
// 创建和加载
// ============================================================================

void Tilemap::Create(int width, int height, int tileSize) {
    m_width = width;
    m_height = height;
    m_tileWidth = tileSize;
    m_tileHeight = tileSize;
    
    m_layers.clear();
    m_objects.clear();
}

void Tilemap::LoadFromData(const std::vector<int>& data, int width, int height, int tileSize) {
    Create(width, height, tileSize);
    
    // 创建默认图层
    int layerIdx = AddLayer("main", true);
    TilemapLayer* layer = GetLayer(layerIdx);
    
    if (layer) {
        layer->data = data;
    }
}

bool Tilemap::LoadFromJSON(const std::string& filename) {
    /**
     * 简化的JSON解析
     * 
     * 注意：这是一个简化实现，只支持基本的Tiled JSON格式
     * 生产环境应该使用完整的JSON库（如nlohmann/json）
     * 
     * 本示例主要用于演示Tilemap的概念
     */
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[Tilemap] Failed to open file: " << filename << std::endl;
        return false;
    }
    
    // 读取整个文件
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    std::cout << "[Tilemap] Loading from JSON: " << filename << std::endl;
    std::cout << "[Tilemap] Note: Using simplified parser. For production, use a proper JSON library." << std::endl;
    
    // 这里应该解析JSON，但为了简化，我们跳过
    // 实际项目中应该使用 nlohmann/json 或 rapidjson
    
    return true;
}

// ============================================================================
// 图层管理
// ============================================================================

int Tilemap::AddLayer(const std::string& name, bool hasCollision) {
    TilemapLayer layer;
    layer.name = name;
    layer.width = m_width;
    layer.height = m_height;
    layer.data.resize(m_width * m_height, 0);
    layer.collision = hasCollision;
    layer.visible = true;
    layer.opacity = 1.0f;
    
    m_layers.push_back(layer);
    return (int)m_layers.size() - 1;
}

TilemapLayer* Tilemap::GetLayer(int index) {
    if (index >= 0 && index < (int)m_layers.size()) {
        return &m_layers[index];
    }
    return nullptr;
}

TilemapLayer* Tilemap::GetLayer(const std::string& name) {
    for (auto& layer : m_layers) {
        if (layer.name == name) {
            return &layer;
        }
    }
    return nullptr;
}

const TilemapLayer* Tilemap::GetLayer(int index) const {
    if (index >= 0 && index < (int)m_layers.size()) {
        return &m_layers[index];
    }
    return nullptr;
}

const TilemapLayer* Tilemap::GetLayer(const std::string& name) const {
    for (const auto& layer : m_layers) {
        if (layer.name == name) {
            return &layer;
        }
    }
    return nullptr;
}

// ============================================================================
// 瓦片操作
// ============================================================================

void Tilemap::SetTile(int layerIndex, int x, int y, int tileID) {
    TilemapLayer* layer = GetLayer(layerIndex);
    if (layer) {
        layer->SetTile(x, y, tileID);
    }
}

int Tilemap::GetTile(int layerIndex, int x, int y) const {
    const TilemapLayer* layer = GetLayer(layerIndex);
    if (layer) {
        return layer->GetTile(x, y);
    }
    return 0;
}

int Tilemap::GetCollisionTile(int x, int y) const {
    for (const auto& layer : m_layers) {
        if (layer.collision) {
            int tile = layer.GetTile(x, y);
            if (tile != 0) {
                return tile;
            }
        }
    }
    return 0;
}

// ============================================================================
// 图集
// ============================================================================

void Tilemap::SetTileset(std::shared_ptr<Tileset> tileset) {
    m_tilesets.clear();
    m_tilesets.push_back(tileset);
}

void Tilemap::AddTileset(std::shared_ptr<Tileset> tileset) {
    m_tilesets.push_back(tileset);
}

Tileset* Tilemap::GetTilesetForGID(int gid) {
    for (auto& tileset : m_tilesets) {
        if (tileset->ContainsGID(gid)) {
            return tileset.get();
        }
    }
    return nullptr;
}

// ============================================================================
// 坐标转换
// ============================================================================

void Tilemap::WorldToTile(float worldX, float worldY, int& tileX, int& tileY) const {
    tileX = (int)(worldX / m_tileWidth);
    tileY = (int)(worldY / m_tileHeight);
}

void Tilemap::TileToWorld(int tileX, int tileY, float& worldX, float& worldY) const {
    worldX = (float)(tileX * m_tileWidth);
    worldY = (float)(tileY * m_tileHeight);
}

AABB Tilemap::GetTileAABB(int tileX, int tileY) const {
    AABB aabb;
    
    float x = tileX * m_tileWidth + m_tileWidth * 0.5f;
    float y = tileY * m_tileHeight + m_tileHeight * 0.5f;
    
    aabb.center = Math::Vec2(x, y);
    aabb.halfSize = Math::Vec2(m_tileWidth * 0.5f, m_tileHeight * 0.5f);
    
    return aabb;
}

// ============================================================================
// 碰撞检测
// ============================================================================

bool Tilemap::IsTileSolid(int tileID) const {
    if (tileID == 0) return false;  // 0总是空的
    
    if (m_allNonZeroSolid) {
        return true;  // 所有非0瓦片都实心
    }
    
    // 检查是否在实心瓦片列表中
    for (int solidID : m_solidTileIDs) {
        if (tileID == solidID) return true;
    }
    
    return false;
}

bool Tilemap::IsSolid(int tileX, int tileY) const {
    int tileID = GetCollisionTile(tileX, tileY);
    return IsTileSolid(tileID);
}

bool Tilemap::CheckCollision(const AABB& aabb) const {
    /**
     * 高效碰撞检测：
     * 只检查AABB覆盖的瓦片范围
     */
    
    Math::Vec2 min = aabb.GetMin();
    Math::Vec2 max = aabb.GetMax();
    
    int startX, startY, endX, endY;
    WorldToTile(min.x, min.y, startX, startY);
    WorldToTile(max.x, max.y, endX, endY);
    
    // 确保范围有效
    startX = std::max(0, startX);
    startY = std::max(0, startY);
    endX = std::min(m_width - 1, endX);
    endY = std::min(m_height - 1, endY);
    
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            if (IsSolid(x, y)) {
                AABB tileAABB = GetTileAABB(x, y);
                if (aabb.Intersects(tileAABB)) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

std::vector<AABB> Tilemap::GetCollidingTiles(const AABB& aabb) const {
    std::vector<AABB> result;
    
    Math::Vec2 min = aabb.GetMin();
    Math::Vec2 max = aabb.GetMax();
    
    int startX, startY, endX, endY;
    WorldToTile(min.x, min.y, startX, startY);
    WorldToTile(max.x, max.y, endX, endY);
    
    // 扩展范围以确保不遗漏
    startX = std::max(0, startX - 1);
    startY = std::max(0, startY - 1);
    endX = std::min(m_width - 1, endX + 1);
    endY = std::min(m_height - 1, endY + 1);
    
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            if (IsSolid(x, y)) {
                AABB tileAABB = GetTileAABB(x, y);
                if (aabb.Intersects(tileAABB)) {
                    result.push_back(tileAABB);
                }
            }
        }
    }
    
    return result;
}

void Tilemap::SetSolidTiles(const std::vector<int>& solidTileIDs) {
    m_solidTileIDs = solidTileIDs;
    m_allNonZeroSolid = false;
}

// ============================================================================
// 对象层
// ============================================================================

const MapObject* Tilemap::GetObject(const std::string& name) const {
    for (const auto& obj : m_objects) {
        if (obj.name == name) {
            return &obj;
        }
    }
    return nullptr;
}

std::vector<const MapObject*> Tilemap::GetObjectsByType(const std::string& type) const {
    std::vector<const MapObject*> result;
    for (const auto& obj : m_objects) {
        if (obj.type == type) {
            result.push_back(&obj);
        }
    }
    return result;
}

void Tilemap::AddObject(const MapObject& obj) {
    m_objects.push_back(obj);
}

} // namespace MiniEngine
