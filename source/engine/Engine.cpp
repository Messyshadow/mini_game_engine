/**
 * Engine.cpp - 游戏引擎核心实现
 */

#include "Engine.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

namespace MiniEngine {

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

static void ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

Engine& Engine::Instance() {
    static Engine instance;
    return instance;
}

Engine::Engine() = default;
Engine::~Engine() { Shutdown(); }

bool Engine::Initialize(const EngineConfig& config) {
    std::cout << "=== Mini Game Engine ===" << std::endl;
    
    if (!InitWindow(config)) return false;
    if (!InitOpenGL()) return false;
    if (!InitImGui()) return false;
    
    m_running = true;
    std::cout << "Engine initialized successfully!" << std::endl;
    return true;
}

bool Engine::InitWindow(const EngineConfig& config) {
    glfwSetErrorCallback(ErrorCallback);
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, config.samples);
    
    GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    m_window = glfwCreateWindow(config.windowWidth, config.windowHeight, 
                                 config.windowTitle.c_str(), monitor, nullptr);
    
    if (!m_window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(config.vsync ? 1 : 0);
    glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);
    
    m_windowWidth = config.windowWidth;
    m_windowHeight = config.windowHeight;
    
    std::cout << "Window created: " << config.windowWidth << "x" << config.windowHeight << std::endl;
    return true;
}

bool Engine::InitOpenGL() {
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return false;
    }
    
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    
    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    return true;
}

bool Engine::InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    return true;
}

void Engine::Run() {
    m_lastFrameTime = static_cast<float>(glfwGetTime());
    
    while (!glfwWindowShouldClose(m_window) && m_running) {
        UpdateTime();
        ProcessInput();
        BeginFrame();
        
        if (m_updateCallback) m_updateCallback(m_deltaTime);
        if (m_renderCallback) m_renderCallback();
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        if (m_imguiCallback) m_imguiCallback();
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        EndFrame();
    }
}

void Engine::UpdateTime() {
    float currentTime = static_cast<float>(glfwGetTime());
    m_deltaTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;
    m_totalTime = currentTime;
    
    if (m_deltaTime > 0.25f) m_deltaTime = 0.25f;
    
    m_frameCount++;
    m_fpsUpdateTime += m_deltaTime;
    if (m_fpsUpdateTime >= 1.0f) {
        m_fps = static_cast<float>(m_frameCount) / m_fpsUpdateTime;
        m_frameCount = 0;
        m_fpsUpdateTime = 0.0f;
    }
}

void Engine::ProcessInput() {
    glfwPollEvents();
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) Quit();
}

void Engine::BeginFrame() {
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Engine::EndFrame() {
    glfwSwapBuffers(m_window);
}

void Engine::Quit() { m_running = false; }

void Engine::Shutdown() {
    // 防止重复调用
    static bool s_shutdown = false;
    if (s_shutdown) return;
    s_shutdown = true;
    
    // 先清除回调，防止悬空引用
    m_updateCallback = nullptr;
    m_renderCallback = nullptr;
    m_imguiCallback = nullptr;
    
    // 按正确顺序释放资源
    // 1. 先关闭ImGui（它依赖GLFW和OpenGL）
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    
    // 2. 再销毁窗口和GLFW
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
    
    std::cout << "Engine shutdown complete." << std::endl;
}

void Engine::SetUpdateCallback(std::function<void(float)> callback) { m_updateCallback = callback; }
void Engine::SetRenderCallback(std::function<void()> callback) { m_renderCallback = callback; }
void Engine::SetImGuiCallback(std::function<void()> callback) { m_imguiCallback = callback; }

} // namespace MiniEngine
