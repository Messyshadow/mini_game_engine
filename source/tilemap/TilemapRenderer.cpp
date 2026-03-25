/**
 * TilemapRenderer.cpp - 瓦片地图渲染器实现
 */

#include "TilemapRenderer.h"
#include <algorithm>
#include <cmath>

namespace MiniEngine {

TilemapRenderer::TilemapRenderer()
    : showGrid(false)
    , showCollision(false)
{
}

void TilemapRenderer::Render(Renderer2D* renderer, const Tilemap* tilemap,
                              float cameraX, float cameraY,
                              int screenWidth, int screenHeight) {
    if (!tilemap || !renderer) return;
    
    // 渲染所有可见图层
    for (int i = 0; i < tilemap->GetLayerCount(); i++) {
        const TilemapLayer* layer = tilemap->GetLayer(i);
        if (layer && layer->visible) {
            RenderLayer(renderer, tilemap, i, cameraX, cameraY, screenWidth, screenHeight);
        }
    }
    
    // 调试渲染
    if (showGrid) {
        RenderGrid(renderer, tilemap, cameraX, cameraY, screenWidth, screenHeight);
    }
    
    if (showCollision) {
        RenderCollision(renderer, tilemap, cameraX, cameraY, screenWidth, screenHeight);
    }
}

void TilemapRenderer::RenderLayer(Renderer2D* renderer, const Tilemap* tilemap,
                                   int layerIndex,
                                   float cameraX, float cameraY,
                                   int screenWidth, int screenHeight) {
    const TilemapLayer* layer = tilemap->GetLayer(layerIndex);
    if (!layer) return;
    
    int tileW = tilemap->GetTileWidth();
    int tileH = tilemap->GetTileHeight();
    
    // 计算可见范围（瓦片坐标）
    int startX = std::max(0, (int)(cameraX / tileW));
    int startY = std::max(0, (int)(cameraY / tileH));
    int endX = std::min(layer->width - 1, (int)((cameraX + screenWidth) / tileW) + 1);
    int endY = std::min(layer->height - 1, (int)((cameraY + screenHeight) / tileH) + 1);
    
    // 渲染可见瓦片
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            int tileID = layer->GetTile(x, y);
            if (tileID == 0) continue;
            
            float screenX = x * tileW - cameraX;
            float screenY = y * tileH - cameraY;
            
            float ndcX = (screenX / screenWidth) * 2.0f - 1.0f;
            float ndcY = (screenY / screenHeight) * 2.0f - 1.0f;
            float ndcW = (float)tileW / screenWidth * 2.0f;
            float ndcH = (float)tileH / screenHeight * 2.0f;
            
            // 根据瓦片ID生成颜色
            Math::Vec4 color;
            float hue = (float)(tileID % 12) / 12.0f;
            float h = hue * 6.0f;
            float c = 0.8f;
            float x2 = c * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));
            
            if (h < 1) color = Math::Vec4(c, x2, 0, 1);
            else if (h < 2) color = Math::Vec4(x2, c, 0, 1);
            else if (h < 3) color = Math::Vec4(0, c, x2, 1);
            else if (h < 4) color = Math::Vec4(0, x2, c, 1);
            else if (h < 5) color = Math::Vec4(x2, 0, c, 1);
            else color = Math::Vec4(c, 0, x2, 1);
            
            color.w *= layer->opacity;
            
            Math::Vec2 p1(ndcX, ndcY);
            Math::Vec2 p2(ndcX + ndcW, ndcY);
            Math::Vec2 p3(ndcX + ndcW, ndcY + ndcH);
            Math::Vec2 p4(ndcX, ndcY + ndcH);
            
            renderer->DrawTriangle(p1, p2, p3, color);
            renderer->DrawTriangle(p1, p3, p4, color);
        }
    }
}

void TilemapRenderer::RenderGrid(Renderer2D* renderer, const Tilemap* tilemap,
                                  float cameraX, float cameraY,
                                  int screenWidth, int screenHeight) {
    int tileW = tilemap->GetTileWidth();
    int tileH = tilemap->GetTileHeight();
    
    int startX = std::max(0, (int)(cameraX / tileW));
    int startY = std::max(0, (int)(cameraY / tileH));
    int endX = std::min(tilemap->GetWidth(), (int)((cameraX + screenWidth) / tileW) + 2);
    int endY = std::min(tilemap->GetHeight(), (int)((cameraY + screenHeight) / tileH) + 2);
    
    for (int x = startX; x <= endX; x++) {
        float screenX = x * tileW - cameraX;
        float ndcX = (screenX / screenWidth) * 2.0f - 1.0f;
        renderer->DrawLine(Math::Vec2(ndcX, -1.0f), Math::Vec2(ndcX, 1.0f), gridColor, 1.0f);
    }
    
    for (int y = startY; y <= endY; y++) {
        float screenY = y * tileH - cameraY;
        float ndcY = (screenY / screenHeight) * 2.0f - 1.0f;
        renderer->DrawLine(Math::Vec2(-1.0f, ndcY), Math::Vec2(1.0f, ndcY), gridColor, 1.0f);
    }
}

void TilemapRenderer::RenderCollision(Renderer2D* renderer, const Tilemap* tilemap,
                                       float cameraX, float cameraY,
                                       int screenWidth, int screenHeight) {
    int tileW = tilemap->GetTileWidth();
    int tileH = tilemap->GetTileHeight();
    
    int startX = std::max(0, (int)(cameraX / tileW));
    int startY = std::max(0, (int)(cameraY / tileH));
    int endX = std::min(tilemap->GetWidth() - 1, (int)((cameraX + screenWidth) / tileW) + 1);
    int endY = std::min(tilemap->GetHeight() - 1, (int)((cameraY + screenHeight) / tileH) + 1);
    
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            if (tilemap->IsSolid(x, y)) {
                float screenX = x * tileW - cameraX;
                float screenY = y * tileH - cameraY;
                
                float ndcX = (screenX / screenWidth) * 2.0f - 1.0f;
                float ndcY = (screenY / screenHeight) * 2.0f - 1.0f;
                float ndcW = (float)tileW / screenWidth * 2.0f;
                float ndcH = (float)tileH / screenHeight * 2.0f;
                
                Math::Vec2 p1(ndcX, ndcY);
                Math::Vec2 p2(ndcX + ndcW, ndcY);
                Math::Vec2 p3(ndcX + ndcW, ndcY + ndcH);
                Math::Vec2 p4(ndcX, ndcY + ndcH);
                
                renderer->DrawTriangle(p1, p2, p3, collisionColor);
                renderer->DrawTriangle(p1, p3, p4, collisionColor);
            }
        }
    }
}

} // namespace MiniEngine
