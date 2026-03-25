/**
 * TilemapRenderer.h - 瓦片地图渲染器
 * 
 * 专门负责渲染瓦片地图
 */

#ifndef MINI_ENGINE_TILEMAP_RENDERER_H
#define MINI_ENGINE_TILEMAP_RENDERER_H

#include "Tilemap.h"
#include "engine/Renderer2D.h"
#include "math/Math.h"

namespace MiniEngine {

/**
 * 瓦片地图渲染器
 */
class TilemapRenderer {
public:
    TilemapRenderer();
    ~TilemapRenderer() = default;
    
    /**
     * 渲染瓦片地图
     * 
     * @param renderer 渲染器
     * @param tilemap 要渲染的地图
     * @param cameraX 相机X位置
     * @param cameraY 相机Y位置
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     */
    void Render(Renderer2D* renderer, const Tilemap* tilemap,
                float cameraX, float cameraY,
                int screenWidth, int screenHeight);
    
    /**
     * 渲染单个图层
     */
    void RenderLayer(Renderer2D* renderer, const Tilemap* tilemap,
                     int layerIndex,
                     float cameraX, float cameraY,
                     int screenWidth, int screenHeight);
    
    /**
     * 渲染网格（调试用）
     */
    void RenderGrid(Renderer2D* renderer, const Tilemap* tilemap,
                    float cameraX, float cameraY,
                    int screenWidth, int screenHeight);
    
    /**
     * 渲染碰撞框（调试用）
     */
    void RenderCollision(Renderer2D* renderer, const Tilemap* tilemap,
                         float cameraX, float cameraY,
                         int screenWidth, int screenHeight);
    
    // 调试选项
    bool showGrid = false;
    bool showCollision = false;
    Math::Vec4 gridColor = Math::Vec4(1.0f, 1.0f, 1.0f, 0.2f);
    Math::Vec4 collisionColor = Math::Vec4(1.0f, 0.0f, 0.0f, 0.3f);
};

} // namespace MiniEngine

#endif // MINI_ENGINE_TILEMAP_RENDERER_H
