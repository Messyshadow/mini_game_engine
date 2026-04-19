cmake_minimum_required(VERSION 3.17)
project(MiniGameEngine)

set(CMAKE_CXX_STANDARD 17)

# MSVC编译选项
if (MSVC)
    add_compile_options(/bigobj)
    add_compile_options(/wd4819)  # 忽略保存为unicode警告
    add_compile_options(/utf-8)   # 使用UTF-8编码
endif ()

# 包含子模块CMake文件
include(CMakeLists.txt.glfw)
include(CMakeLists.txt.imgui)

# 拷贝depends目录（从template拷贝到build同级的depends）
file(COPY "./template/depends/glfw-3.3-3.4/" DESTINATION "../depends/glfw-3.3-3.4/")
file(COPY "./template/depends/imgui/" DESTINATION "../depends/imgui/")
file(COPY "./template/depends/stb/" DESTINATION "../depends/stb/")

# 拷贝data目录
file(COPY "./template/data/" DESTINATION "../data/")

# 头文件目录
include_directories("depends")
include_directories("depends/stb")
include_directories("depends/miniaudio")
include_directories("depends/assimp/include")
include_directories("source")

# 收集引擎源文件
file(GLOB_RECURSE engine_cpp source/*.cpp)
file(GLOB_RECURSE engine_h source/*.h)

# 示例源文件
file(GLOB example_01_cpp examples/01_window/*.cpp)
file(GLOB example_02_cpp examples/02_triangle/*.cpp)
file(GLOB example_03_cpp examples/03_transform/*.cpp)
file(GLOB example_04_cpp examples/04_sprite/*.cpp)
file(GLOB example_05_cpp examples/05_collision/*.cpp)
file(GLOB example_06_cpp examples/06_animation/*.cpp)
file(GLOB example_07_cpp examples/07_platformer_physics/*.cpp)
file(GLOB example_08_cpp examples/08_character_controller/*.cpp)
file(GLOB example_09_cpp examples/09_tilemap/*.cpp)
file(GLOB example_10_cpp examples/10_textured_tilemap/*.cpp)
file(GLOB example_11_cpp examples/11_sprite_animation/*.cpp)
file(GLOB example_12_cpp examples/12_camera_system/*.cpp)
file(GLOB example_13_cpp examples/13_audio_system/*.cpp)
file(GLOB example_14_cpp examples/14_combat_system/*.cpp)
file(GLOB example_15_cpp examples/15_enemy_ai/*.cpp)
file(GLOB example_16_cpp examples/16_room_system/*.cpp)
file(GLOB example_17_cpp examples/17_ui_system/*.cpp)
file(GLOB example_18_cpp examples/18_full_demo/*.cpp)
file(GLOB example_19_cpp examples/19_3d_basics/*.cpp)
file(GLOB example_20_cpp examples/20_model_loading/*.cpp)
file(GLOB example_21_cpp examples/21_lighting_system/*.cpp)
file(GLOB example_22_cpp examples/22_materials_textures/*.cpp)
file(GLOB example_23_cpp examples/23_3d_camera/*.cpp)
file(GLOB example_24_cpp examples/24_character_controller_3d/*.cpp)
file(GLOB example_25_cpp examples/25_skeletal_animation/*.cpp)
file(GLOB example_26_cpp examples/26_3d_combat/*.cpp)

# ============================================================================
# 示例01：基础窗口
# ============================================================================
add_executable(01_window 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_01_cpp}
)

# ============================================================================
# 示例02：三角形渲染
# ============================================================================
add_executable(02_triangle 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_02_cpp}
)

# ============================================================================
# 示例03：矩阵变换
# ============================================================================
add_executable(03_transform 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_03_cpp}
)

# ============================================================================
# 示例04：纹理和精灵
# ============================================================================
add_executable(04_sprite 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_04_cpp}
)

# ============================================================================
# 示例05：碰撞检测
# ============================================================================
add_executable(05_collision 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_05_cpp}
)

# ============================================================================
# 示例06：动画系统
# ============================================================================
add_executable(06_animation 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_06_cpp}
)

# ============================================================================
# 示例07：平台物理
# ============================================================================
add_executable(07_platformer_physics 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_07_cpp}
)

# ============================================================================
# 示例08：角色控制器
# ============================================================================
add_executable(08_character_controller 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_08_cpp}
)

# ============================================================================
# 示例09：瓦片地图
# ============================================================================
add_executable(09_tilemap 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_09_cpp}
)

# ============================================================================
# 示例10：纹理瓦片地图
# ============================================================================
add_executable(10_textured_tilemap 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_10_cpp}
)

# ============================================================================
# 示例11：精灵动画系统
# ============================================================================
add_executable(11_sprite_animation 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_11_cpp}
)

# ============================================================================
# 示例12：摄像机系统
# ============================================================================
add_executable(12_camera_system 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_12_cpp}
)

# ============================================================================
# 示例13：音频系统
# ============================================================================
add_executable(13_audio_system 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_13_cpp}
)

# ============================================================================
# 示例14：战斗系统
# ============================================================================
add_executable(14_combat_system 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_14_cpp}
)

# ============================================================================
# 示例15：敌人AI
# ============================================================================
add_executable(15_enemy_ai 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_15_cpp}
)

# ============================================================================
# 示例16：房间关卡系统
# ============================================================================
add_executable(16_room_system 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_16_cpp}
)

# ============================================================================
# 示例17：UI系统与进阶玩法
# ============================================================================
add_executable(17_ui_system 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_17_cpp}
)

# ============================================================================
# 示例18：完整银河恶魔城Demo
# ============================================================================
add_executable(18_full_demo 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_18_cpp}
)

# ============================================================================
# 示例19：3D渲染基础
# ============================================================================
add_executable(19_3d_basics 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_19_cpp}
)

# ============================================================================
# 示例20：3D模型加载（需要Assimp）
# ============================================================================
add_executable(20_model_loading 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_20_cpp}
)

# Assimp链接（只有第20章及之后需要）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib")
    target_link_libraries(20_model_loading 
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/zlibstatic.lib")
    # 拷贝DLL到输出目录
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/bin/assimp-vc142-mt.dll" 
         DESTINATION "${CMAKE_BINARY_DIR}/Debug")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/bin/assimp-vc142-mt.dll" 
         DESTINATION "${CMAKE_BINARY_DIR}/Release")
endif()

# ============================================================================
# 示例21：光照系统（需要Assimp）
# ============================================================================
add_executable(21_lighting_system 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_21_cpp}
)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib")
    target_link_libraries(21_lighting_system 
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/zlibstatic.lib")
endif()

# ============================================================================
# 示例22：材质与纹理
# ============================================================================
add_executable(22_materials_textures 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_22_cpp}
)

# ============================================================================
# 示例23：3D摄像机系统（需要Assimp加载模型）
# ============================================================================
add_executable(23_3d_camera 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_23_cpp}
)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib")
    target_link_libraries(23_3d_camera 
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/zlibstatic.lib")
endif()

# ============================================================================
# 示例24：3D角色控制器
# ============================================================================
add_executable(24_character_controller_3d 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_24_cpp}
)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib")
    target_link_libraries(24_character_controller_3d 
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/zlibstatic.lib")
endif()

# ============================================================================
# 示例25：骨骼动画
# ============================================================================
add_executable(25_skeletal_animation 
    ${glfw_sources} 
    ${imgui_src} 
    ${engine_cpp} 
    ${engine_h}
    ${example_25_cpp}
)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib")
    target_link_libraries(25_skeletal_animation
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/zlibstatic.lib")
endif()

# ============================================================================
# 示例26：3D战斗系统
# ============================================================================
add_executable(26_3d_combat
    ${glfw_sources}
    ${imgui_src}
    ${engine_cpp}
    ${engine_h}
    ${example_26_cpp}
)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib")
    target_link_libraries(26_3d_combat
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/assimp-vc142-mt.lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/assimp/lib/zlibstatic.lib")
endif()

# MSVC运行时库设置
if (MSVC)
    set_property(TARGET 01_window PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 02_triangle PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 03_transform PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 04_sprite PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 05_collision PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 06_animation PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 07_platformer_physics PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 08_character_controller PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 09_tilemap PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 10_textured_tilemap PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 11_sprite_animation PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 12_camera_system PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 13_audio_system PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 14_combat_system PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 15_enemy_ai PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 16_room_system PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 17_ui_system PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 18_full_demo PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 19_3d_basics PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 20_model_loading PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 21_lighting_system PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 22_materials_textures PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 23_3d_camera PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 24_character_controller_3d PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 25_skeletal_animation PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET 26_3d_combat PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT 26_3d_combat)
    
    set_target_properties(01_window PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(02_triangle PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(03_transform PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(04_sprite PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(05_collision PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(06_animation PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(07_platformer_physics PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(08_character_controller PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(09_tilemap PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(10_textured_tilemap PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(11_sprite_animation PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(12_camera_system PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(13_audio_system PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(14_combat_system PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(15_enemy_ai PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(16_room_system PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(17_ui_system PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(18_full_demo PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(19_3d_basics PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(20_model_loading PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(21_lighting_system PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(22_materials_textures PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(23_3d_camera PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(24_character_controller_3d PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(25_skeletal_animation PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(26_3d_combat PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()
