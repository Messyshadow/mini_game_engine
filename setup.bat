@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo.
echo ==========================================
echo   Mini Game Engine - Setup Script
echo   Version 3.0
echo ==========================================
echo.

cd /d "%~dp0"
echo Working directory: %CD%
echo.

:: ============================================================================
:: Step 1: Create directories
:: ============================================================================
echo [1/8] Creating directory structure...

if not exist "template\depends\stb" mkdir "template\depends\stb"
if not exist "template\depends\miniaudio" mkdir "template\depends\miniaudio"
if not exist "template\data\shader" mkdir "template\data\shader"
if not exist "template\data\texture\characters" mkdir "template\data\texture\characters"
if not exist "template\data\texture\tiles" mkdir "template\data\texture\tiles"
if not exist "template\data\texture\ui" mkdir "template\data\texture\ui"
if not exist "template\data\texture\effects" mkdir "template\data\texture\effects"
if not exist "template\data\audio\sfx" mkdir "template\data\audio\sfx"
if not exist "template\data\audio\bgm" mkdir "template\data\audio\bgm"
if not exist "template\data\font" mkdir "template\data\font"

echo   Done.
echo.

:: ============================================================================
:: Step 2: Check tools
:: ============================================================================
echo [2/8] Checking required tools...

git --version >nul 2>&1
if %errorlevel% neq 0 (
    echo   [ERROR] Git not found!
    echo   Please install from: https://git-scm.com/download/win
    pause
    exit /b 1
)
echo   Git - OK

where powershell >nul 2>&1
if %errorlevel% neq 0 (
    echo   [ERROR] PowerShell not found!
    pause
    exit /b 1
)
echo   PowerShell - OK
echo.

:: ============================================================================
:: Step 3: Download GLFW
:: ============================================================================
echo [3/8] Setting up GLFW...

if exist "template\depends\glfw-3.3-3.4\src\init.c" (
    echo   Already exists, skipping...
) else (
    echo   Downloading GLFW 3.3.8...
    if exist "template\depends\glfw-3.3-3.4" rmdir /s /q "template\depends\glfw-3.3-3.4"
    git clone --branch 3.3.8 https://github.com/glfw/glfw.git "template\depends\glfw-3.3-3.4"
    if !errorlevel! neq 0 (
        echo   [ERROR] Failed to download GLFW!
        pause
        exit /b 1
    )
    echo   Done.
)
echo.

:: ============================================================================
:: Step 4: Download ImGui
:: ============================================================================
echo [4/8] Setting up ImGui...

if exist "template\depends\imgui\imgui.h" (
    echo   Already exists, skipping...
) else (
    echo   Downloading ImGui v1.90.1...
    if exist "template\depends\imgui" rmdir /s /q "template\depends\imgui"
    git clone --depth 1 --branch v1.90.1 https://github.com/ocornut/imgui.git "template\depends\imgui"
    if !errorlevel! neq 0 (
        echo   [ERROR] Failed to download ImGui!
        pause
        exit /b 1
    )
    echo   Done.
)
echo.

:: ============================================================================
:: Step 5: Download stb_image
:: ============================================================================
echo [5/8] Setting up stb_image...

if exist "template\depends\stb\stb_image.h" (
    echo   Already exists, skipping...
) else (
    echo   Downloading stb_image.h...
    powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/nothings/stb/master/stb_image.h' -OutFile 'template\depends\stb\stb_image.h'"
    if exist "template\depends\stb\stb_image.h" (
        echo   Done.
    ) else (
        echo   [WARNING] Failed to download stb_image.h
    )
)
echo.

:: ============================================================================
:: Step 6: Download miniaudio
:: ============================================================================
echo [6/8] Setting up miniaudio...

if exist "template\depends\miniaudio\miniaudio.h" (
    echo   Already exists, skipping...
) else (
    echo   Downloading miniaudio.h...
    powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h' -OutFile 'template\depends\miniaudio\miniaudio.h'"
    if exist "template\depends\miniaudio\miniaudio.h" (
        echo   Done.
    ) else (
        echo   [WARNING] Failed to download miniaudio.h
        echo   Download manually from: https://github.com/mackron/miniaudio
    )
)
echo.

:: ============================================================================
:: Step 7: Setup asset folders
:: ============================================================================
echo [7/8] Setting up asset folders...

echo   Creating README files for asset folders...
echo Download from: https://kenney.nl/assets/pixel-platformer > "template\data\texture\characters\README.txt"
echo Download from: https://kenney.nl/assets/pixel-platformer > "template\data\texture\tiles\README.txt"
echo Download from: https://kenney.nl/assets/game-icons > "template\data\texture\ui\README.txt"
echo Download from: https://kenney.nl/assets/impact-sounds > "template\data\audio\sfx\README.txt"
echo Background music folder > "template\data\audio\bgm\README.txt"

echo   Done.
echo.

:: ============================================================================
:: Step 8: Create shader files
:: ============================================================================
echo [8/8] Creating shader files...

echo #version 330 core > "template\data\shader\sprite.vert"
echo layout (location = 0) in vec2 aPos; >> "template\data\shader\sprite.vert"
echo layout (location = 1) in vec2 aTexCoord; >> "template\data\shader\sprite.vert"
echo out vec2 vTexCoord; >> "template\data\shader\sprite.vert"
echo uniform mat4 uProjection; >> "template\data\shader\sprite.vert"
echo uniform mat4 uModel; >> "template\data\shader\sprite.vert"
echo void main() { >> "template\data\shader\sprite.vert"
echo     gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0); >> "template\data\shader\sprite.vert"
echo     vTexCoord = aTexCoord; >> "template\data\shader\sprite.vert"
echo } >> "template\data\shader\sprite.vert"

echo #version 330 core > "template\data\shader\sprite.frag"
echo in vec2 vTexCoord; >> "template\data\shader\sprite.frag"
echo out vec4 FragColor; >> "template\data\shader\sprite.frag"
echo uniform sampler2D uTexture; >> "template\data\shader\sprite.frag"
echo uniform vec4 uColor; >> "template\data\shader\sprite.frag"
echo void main() { >> "template\data\shader\sprite.frag"
echo     vec4 texColor = texture(uTexture, vTexCoord); >> "template\data\shader\sprite.frag"
echo     if (texColor.a ^< 0.01) discard; >> "template\data\shader\sprite.frag"
echo     FragColor = texColor * uColor; >> "template\data\shader\sprite.frag"
echo } >> "template\data\shader\sprite.frag"

echo   sprite.vert - OK
echo   sprite.frag - OK
echo.

:: ============================================================================
:: Verify installation
:: ============================================================================
echo ==========================================
echo   Verifying installation...
echo ==========================================
echo.

set ERRORS=0

if exist "template\depends\glfw-3.3-3.4\include\GLFW\glfw3.h" (
    echo   [OK] GLFW
) else (
    echo   [MISSING] GLFW
    set ERRORS=1
)

if exist "template\depends\imgui\imgui.h" (
    echo   [OK] ImGui
) else (
    echo   [MISSING] ImGui
    set ERRORS=1
)

if exist "template\depends\stb\stb_image.h" (
    echo   [OK] stb_image
) else (
    echo   [MISSING] stb_image
    set ERRORS=1
)

if exist "template\depends\miniaudio\miniaudio.h" (
    echo   [OK] miniaudio
) else (
    echo   [MISSING] miniaudio
    set ERRORS=1
)

if exist "template\data\shader\sprite.vert" (
    echo   [OK] Shaders
) else (
    echo   [MISSING] Shaders
    set ERRORS=1
)

echo.

:: ============================================================================
:: Done
:: ============================================================================
if %ERRORS%==0 (
    echo ==========================================
    echo   Setup Complete!
    echo ==========================================
) else (
    echo ==========================================
    echo   Setup completed with warnings.
    echo   Some components may need manual download.
    echo ==========================================
)

echo.
echo Directory structure:
echo.
echo   template\
echo     depends\
echo       glfw-3.3-3.4\    (Window/Input)
echo       imgui\           (Debug GUI)
echo       stb\             (Image loading)
echo       miniaudio\       (Audio playback)
echo     data\
echo       shader\          (GLSL shaders)
echo       texture\         (Images)
echo       audio\           (Sound files)
echo       font\            (TTF fonts)
echo.
echo ==========================================
echo   Next Steps
echo ==========================================
echo.
echo   cd mini_game_engine
echo   mkdir build
echo   cd build
echo   cmake -G "Visual Studio 16 2019" -A x64 ..
echo   cmake --build . --config Debug
echo.
pause
