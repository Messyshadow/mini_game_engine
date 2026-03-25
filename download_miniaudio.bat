@echo off
chcp 65001 >nul
echo.
echo ==========================================
echo   Downloading miniaudio.h
echo ==========================================
echo.

cd /d "%~dp0"

if not exist "depends\miniaudio" mkdir "depends\miniaudio"

echo Downloading from GitHub...
powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h' -OutFile 'depends\miniaudio\miniaudio.h'"

echo.
if exist "depends\miniaudio\miniaudio.h" (
    echo [SUCCESS] miniaudio.h downloaded!
    echo Location: depends\miniaudio\miniaudio.h
) else (
    echo [FAILED] Download failed.
    echo.
    echo Please download manually:
    echo   URL: https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h
    echo   Save to: depends\miniaudio\miniaudio.h
)

echo.
pause
