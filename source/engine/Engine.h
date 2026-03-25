/**
 * Engine.h - 游戏引擎核心类
 */

#ifndef MINI_ENGINE_ENGINE_H
#define MINI_ENGINE_ENGINE_H

#include <string>
#include <functional>
#include <memory>

struct GLFWwindow;

namespace MiniEngine {

struct EngineConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    std::string windowTitle = "Mini Game Engine";
    bool vsync = true;
    bool resizable = true;
    bool fullscreen = false;
    int samples = 4;
};

class Engine {
public:
    static Engine& Instance();
    
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    
    bool Initialize(const EngineConfig& config = EngineConfig());
    void Run();
    void Quit();
    void Shutdown();
    
    void SetUpdateCallback(std::function<void(float)> callback);
    void SetRenderCallback(std::function<void()> callback);
    void SetImGuiCallback(std::function<void()> callback);
    
    bool IsRunning() const { return m_running; }
    int GetWindowWidth() const { return m_windowWidth; }
    int GetWindowHeight() const { return m_windowHeight; }
    float GetDeltaTime() const { return m_deltaTime; }
    float GetTime() const { return m_totalTime; }
    float GetFPS() const { return m_fps; }
    GLFWwindow* GetWindow() const { return m_window; }
    
private:
    Engine();
    ~Engine();
    
    bool InitWindow(const EngineConfig& config);
    bool InitOpenGL();
    bool InitImGui();
    void ProcessInput();
    void UpdateTime();
    void BeginFrame();
    void EndFrame();
    
    GLFWwindow* m_window = nullptr;
    int m_windowWidth = 1280;
    int m_windowHeight = 720;
    bool m_running = false;
    
    float m_deltaTime = 0.0f;
    float m_totalTime = 0.0f;
    float m_lastFrameTime = 0.0f;
    float m_fps = 0.0f;
    int m_frameCount = 0;
    float m_fpsUpdateTime = 0.0f;
    
    std::function<void(float)> m_updateCallback;
    std::function<void()> m_renderCallback;
    std::function<void()> m_imguiCallback;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_ENGINE_H
