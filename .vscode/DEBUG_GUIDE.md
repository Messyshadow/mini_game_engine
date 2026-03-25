# VSCode 调试配置指南

---

## 📋 前置要求

### 1. 安装VSCode扩展

在VSCode中安装以下扩展（按 `Ctrl+Shift+X` 打开扩展面板）：

| 扩展名 | 说明 | 必需 |
|--------|------|------|
| **C/C++** | Microsoft官方C++扩展 | ✅ 必需 |
| **CMake Tools** | CMake集成 | ✅ 推荐 |
| **CMake** | CMake语法高亮 | 可选 |

### 2. 安装编译器

**方案A：Visual Studio（推荐Windows用户）**
- 下载 [Visual Studio 2019/2022 Community](https://visualstudio.microsoft.com/)
- 安装时选择 "使用C++的桌面开发" 工作负载

**方案B：MinGW-w64（轻量级）**
- 下载 [MSYS2](https://www.msys2.org/)
- 运行 `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb`
- 添加到PATH：`C:\msys64\mingw64\bin`

---

## 🚀 快速开始

### 步骤1：下载依赖

```batch
cd 项目根目录
setup.bat
```

### 步骤2：用VSCode打开项目

```batch
cd mini_game_engine
code .
```

### 步骤3：配置CMake

按 `Ctrl+Shift+P`，输入并选择：
- **Visual Studio用户**：选择 `Tasks: Run Task` → `CMake: configure`
- **MinGW用户**：选择 `Tasks: Run Task` → `CMake: configure (MinGW)`

### 步骤4：构建项目

按 `Ctrl+Shift+B`（或 `Tasks: Run Task` → `CMake: build`）

### 步骤5：开始调试

1. 按 `F5` 或点击左侧调试图标
2. 在顶部下拉菜单选择要调试的示例
3. 点击绿色播放按钮开始调试

---

## 🎮 调试配置列表

| 配置名 | 可执行文件 | 说明 |
|--------|-----------|------|
| 01 - Window & Vector Math | 01_window.exe | 基础窗口和向量数学 |
| 02 - Triangle Rendering | 02_triangle.exe | 三角形渲染 |
| 03 - Matrix Transform | 03_transform.exe | 矩阵变换 |
| 04 - Texture & Sprite | 04_sprite.exe | 纹理和精灵 |
| 05 - Collision Detection | 05_collision.exe | 碰撞检测 |
| 06 - Sprite Animation | 06_animation.exe | 精灵动画 |
| 07 - Platformer Physics | 07_platformer_physics.exe | 平台跳跃物理 |
| 08 - Character Controller | 08_character_controller.exe | 角色控制器 |
| 09 - Tilemap System | 09_tilemap.exe | 瓦片地图系统 |

---

## ⌨️ 调试快捷键

| 快捷键 | 功能 |
|--------|------|
| `F5` | 开始调试 / 继续执行 |
| `Shift+F5` | 停止调试 |
| `Ctrl+Shift+F5` | 重新开始调试 |
| `F9` | 切换断点 |
| `F10` | 单步跳过（Step Over） |
| `F11` | 单步进入（Step Into） |
| `Shift+F11` | 单步跳出（Step Out） |
| `Ctrl+Shift+B` | 构建项目 |

---

## 🔧 调试技巧

### 1. 设置断点

在代码行号左侧点击，出现红点即为断点：

```cpp
void Update(float deltaTime) {
    // 在这里点击行号左侧设置断点
    animTime += deltaTime;  // ← 红点
}
```

### 2. 条件断点

右键断点 → "编辑断点" → 添加条件：

```cpp
// 当 animTime > 5.0 时才暂停
animTime > 5.0f
```

### 3. 监视变量

在调试时：
1. 打开 "监视" 面板（左侧调试视图）
2. 点击 "+" 添加表达式
3. 输入变量名如 `vectorA.x` 或 `g_player.position`

### 4. 查看调用堆栈

调试暂停时，在 "调用堆栈" 面板查看函数调用链：

```
main()
└── engine.Run()
    └── Update(deltaTime)
        └── ResolveCollisions()  ← 当前位置
```

### 5. 即时窗口

在调试暂停时，打开 "调试控制台"，输入表达式查看值：

```
vectorA.Length()
> 3.605551
```

---

## ❓ 常见问题

### Q1: 找不到编译器

**症状**：CMake配置失败，提示找不到编译器

**解决**：
1. 确认已安装Visual Studio或MinGW
2. 重新打开VSCode让PATH生效
3. 使用正确的CMake配置任务

### Q2: 断点不生效（灰色空心圆）

**症状**：断点显示为灰色空心圆，调试时不暂停

**原因**：可能构建的是Release版本，或源码与二进制不匹配

**解决**：
1. 确保构建Debug版本：`CMake: build`（不是build Release）
2. 清理后重新构建：`CMake: rebuild`

### Q3: 启动调试失败

**症状**：提示找不到exe文件

**解决**：
1. 先执行构建：`Ctrl+Shift+B`
2. 检查 `build/Debug/` 目录下是否有exe文件
3. 如果没有，检查构建输出是否有错误

### Q4: 智能感知（IntelliSense）不工作

**症状**：没有代码补全，头文件显示红色波浪线

**解决**：
1. 安装C/C++扩展
2. 检查 `c_cpp_properties.json` 中的 `includePath`
3. 按 `Ctrl+Shift+P` → "C/C++: Reset IntelliSense Database"

### Q5: MinGW调试器找不到

**症状**：使用GDB配置时提示找不到gdb

**解决**：
1. 确认MinGW已安装GDB：`pacman -S mingw-w64-x86_64-gdb`
2. 检查PATH是否包含 `C:\msys64\mingw64\bin`
3. 修改 `launch.json` 中的 `miDebuggerPath` 为完整路径

---

## 📁 配置文件说明

```
.vscode/
├── launch.json           ← 调试配置（选择调试哪个程序）
├── tasks.json            ← 任务配置（构建命令）
├── c_cpp_properties.json ← C++智能感知配置
└── settings.json         ← 工作区设置
```

---

## 🎯 推荐调试流程

1. **初学者**：从 `01 - Window` 开始，单步执行理解主循环
2. **理解渲染**：调试 `02 - Triangle`，观察顶点数据
3. **理解变换**：调试 `03 - Transform`，监视矩阵变化
4. **理解物理**：调试 `07 - Platformer Physics`，观察速度和碰撞

---

## 💡 小贴士

1. **快速定位**：`Ctrl+P` 输入文件名快速打开
2. **查找符号**：`Ctrl+T` 搜索函数/类名
3. **跳转定义**：`F12` 或 `Ctrl+Click`
4. **查看引用**：`Shift+F12`
5. **返回上一位置**：`Alt+←`
