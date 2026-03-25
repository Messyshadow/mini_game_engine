# 第10章：纹理瓦片地图渲染

## 📚 本章概述

本章学习如何使用纹理图集（Tileset）来渲染瓦片地图，替代之前的彩色方块渲染方式。这是2D游戏中最常用的地图渲染技术。

---

## 🎯 学习目标

1. 理解纹理图集（Texture Atlas）的概念
2. 掌握UV坐标的计算方法
3. 实现从tileset.png加载并渲染瓦片
4. 理解视锥剔除优化

---

## 🔑 核心概念

### 1. 什么是Tileset（瓦片图集）？

Tileset是一张包含多个小图（瓦片）的大图：

```
tileset.png (256x128像素)

┌────┬────┬────┬────┬────┬────┬────┬────┐
│ 1  │ 2  │ 3  │ 4  │ 5  │ 6  │ 7  │ 8  │  行0: 天空/背景
├────┼────┼────┼────┼────┼────┼────┼────┤
│ 9  │ 10 │ 11 │ 12 │ 13 │ 14 │ 15 │ 16 │  行1: 草地/泥土
├────┼────┼────┼────┼────┼────┼────┼────┤
│ 17 │ 18 │ 19 │ 20 │ 21 │ 22 │ 23 │ 24 │  行2: 石头/砖块
├────┼────┼────┼────┼────┼────┼────┼────┤
│ 25 │ 26 │ 27 │ 28 │ 29 │ 30 │ 31 │ 32 │  行3: 装饰物
└────┴────┴────┴────┴────┴────┴────┴────┘
      每个格子32x32像素，共8列×4行
```

### 2. 为什么使用Tileset？

| 优点 | 说明 |
|------|------|
| **减少Draw Call** | 一张大图 vs 多张小图 |
| **减少纹理切换** | GPU不需要频繁绑定不同纹理 |
| **更好的批处理** | 相同纹理可以一起渲染 |
| **易于管理** | 所有瓦片在一个文件中 |

### 3. UV坐标系统

UV坐标是纹理坐标，范围从0.0到1.0：

```
UV坐标系（OpenGL）：

(0,1)────────────(1,1)
  │                │
  │    纹理图片    │
  │                │
(0,0)────────────(1,0)

注意：OpenGL的UV原点在左下角
      但图片文件的原点通常在左上角
      所以Y轴需要翻转！
```

### 4. UV计算公式

假设tileset是256x128像素，8列×4行，每个瓦片32x32：

```cpp
// tileID从1开始（0表示空）
int index = tileID - 1;

// 计算在tileset中的位置
int col = index % 8;  // 列号
int row = index / 8;  // 行号

// 计算UV（每个瓦片占的UV大小）
float tileU = 1.0f / 8;   // = 0.125
float tileV = 1.0f / 4;   // = 0.25

// 最终UV坐标（注意Y轴翻转）
uvMin.x = col * tileU;
uvMin.y = 1.0f - (row + 1) * tileV;  // 翻转
uvMax.x = (col + 1) * tileU;
uvMax.y = 1.0f - row * tileV;         // 翻转
```

**示例：计算tileID=9的UV**

```
index = 9 - 1 = 8
col = 8 % 8 = 0
row = 8 / 8 = 1

tileU = 0.125
tileV = 0.25

uvMin = (0 * 0.125, 1 - 2 * 0.25) = (0.0, 0.5)
uvMax = (1 * 0.125, 1 - 1 * 0.25) = (0.125, 0.75)
```

---

## 💻 关键代码

### UV计算函数

```cpp
void CalculateTileUV(int tileID, int cols, int rows, 
                     Vec2& uvMin, Vec2& uvMax) {
    if (tileID <= 0) {
        uvMin = uvMax = Vec2(0, 0);
        return;
    }
    
    int index = tileID - 1;
    int col = index % cols;
    int row = index / cols;
    
    float tileU = 1.0f / cols;
    float tileV = 1.0f / rows;
    
    // Y轴翻转
    uvMin.x = col * tileU;
    uvMin.y = 1.0f - (row + 1) * tileV;
    uvMax.x = (col + 1) * tileU;
    uvMax.y = 1.0f - row * tileV;
}
```

### 纹理瓦片渲染

```cpp
void RenderTexturedTilemap(float cameraX, float cameraY, 
                           int screenW, int screenH) {
    g_spriteRenderer->Begin();
    
    for (int layerIdx = 0; layerIdx < tilemap->GetLayerCount(); layerIdx++) {
        const TilemapLayer* layer = tilemap->GetLayer(layerIdx);
        if (!layer || !layer->visible) continue;
        
        // 视锥剔除：只渲染可见范围
        int startX = std::max(0, (int)(cameraX / tileW) - 1);
        int startY = std::max(0, (int)(cameraY / tileH) - 1);
        int endX = std::min(width-1, (int)((cameraX + screenW) / tileW) + 1);
        int endY = std::min(height-1, (int)((cameraY + screenH) / tileH) + 1);
        
        for (int y = startY; y <= endY; y++) {
            for (int x = startX; x <= endX; x++) {
                int tileID = layer->GetTile(x, y);
                if (tileID == 0) continue;
                
                // 计算屏幕位置
                float screenX = x * tileW - cameraX;
                float screenY = y * tileH - cameraY;
                
                // 计算UV
                Vec2 uvMin, uvMax;
                CalculateTileUV(tileID, cols, rows, uvMin, uvMax);
                
                // 创建精灵并渲染
                Sprite tile;
                tile.SetTexture(tilesetTexture);
                tile.SetPosition(Vec2(screenX + tileW/2, screenY + tileH/2));
                tile.SetSize(Vec2(tileW, tileH));
                tile.SetUV(uvMin, uvMax);
                
                g_spriteRenderer->DrawSprite(tile);
            }
        }
    }
    
    g_spriteRenderer->End();
}
```

---

## 📁 文件依赖

### 必需文件

```
mini_game_engine/
└── data/
    └── texture/
        └── tileset.png    ← 必需！
```

### tileset.png规格

| 属性 | 值 |
|------|-----|
| **尺寸** | 256 × 128 像素 |
| **布局** | 8列 × 4行 |
| **每个瓦片** | 32 × 32 像素 |
| **格式** | PNG（支持透明） |
| **瓦片总数** | 32个 |

---

## 🎮 操作说明

| 按键 | 功能 |
|------|------|
| A/D | 左右移动 |
| 空格 | 跳跃 |
| 方向键 | 手动移动摄像机 |
| **T** | **切换纹理/彩色模式** |
| G | 显示/隐藏网格 |
| C | 显示/隐藏碰撞层 |
| 1/2/3 | 切换图层可见性 |
| E | 编辑模式 |
| R | 重置玩家位置 |

### 编辑模式

| 操作 | 功能 |
|------|------|
| 鼠标左键 | 放置瓦片 |
| 鼠标右键 | 删除瓦片 |
| ImGui滑块 | 选择瓦片ID和图层 |

---

## 🔧 调试技巧

### 1. 纹理加载失败

如果看到警告信息：
```
WARNING: tileset.png not found!
```

**解决方案：**
- 确保`tileset.png`放在`data/texture/`目录
- 检查工作目录设置
- 程序会自动回退到彩色模式

### 2. UV坐标错误

如果瓦片显示错位：
- 检查tileset的列数/行数设置
- 确认tileID是从1开始还是0开始
- 检查Y轴是否正确翻转

### 3. 性能问题

如果帧率低：
- 检查视锥剔除是否正确实现
- 确保只渲染可见范围的瓦片
- 考虑使用批处理优化

---

## 📊 性能优化

### 视锥剔除（Frustum Culling）

只渲染摄像机可见范围内的瓦片：

```cpp
// 计算可见范围
int startX = (int)(cameraX / tileW) - 1;
int endX = (int)((cameraX + screenW) / tileW) + 1;
// ... 只遍历 startX 到 endX
```

**效果：** 40×20的地图，屏幕只显示约40×22个瓦片时：
- 无剔除：渲染800个瓦片
- 有剔除：渲染约880个瓦片（边缘多1格）
- 节省约50%的渲染量

### 批处理（Batching）

将多个相同纹理的瓦片合并为一次Draw Call：
- 当前实现：每个瓦片一次DrawSprite
- 优化方向：构建顶点缓冲，一次性提交

---

## 🎨 自定义Tileset

### 创建自己的tileset.png

1. **使用图片编辑软件**（PS、GIMP、Aseprite）
2. **创建256×128画布**
3. **划分32×32网格**
4. **绘制瓦片**
5. **导出为PNG**

### 推荐布局

```
行0: 天空、云、背景装饰
行1: 草地、泥土、沙子
行2: 石头、砖块、平台
行3: 装饰物、道具、特殊瓦片
```

---

## 📝 思考题

1. **为什么UV的Y轴需要翻转？**
   - 提示：比较OpenGL坐标系和图片文件坐标系

2. **如何支持不同大小的瓦片？**
   - 提示：修改tileU和tileV的计算

3. **如何实现瓦片动画？**
   - 提示：随时间改变tileID

4. **如何支持多个tileset？**
   - 提示：根据tileID范围选择不同的纹理

---

## 🔗 相关章节

- **第9章**：瓦片地图基础（数据结构）
- **第11章**：摄像机系统（下一章）
- **第4章**：纹理与精灵（UV基础）

---

## ✅ 本章检查清单

- [ ] 理解Tileset的概念
- [ ] 掌握UV坐标计算
- [ ] 成功加载tileset.png
- [ ] 实现纹理瓦片渲染
- [ ] 理解视锥剔除优化
- [ ] 能够切换纹理/彩色模式
- [ ] 能够使用编辑模式修改地图
