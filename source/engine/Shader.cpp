/**
 * Shader.cpp - 着色器管理类实现
 * 
 * 支持从文件或源代码加载着色器
 * 文件扩展名约定：
 *   - .vs / .vert  顶点着色器
 *   - .fs / .frag  片段着色器
 */

#include "Shader.h"
#include <glad/gl.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

namespace MiniEngine {

// 着色器搜索路径（按顺序查找）
static const std::vector<std::string> s_shaderSearchPaths = {
    "",                     // 当前目录
    "data/shader/",         // 标准位置
    "../data/shader/",      // 从build目录运行时
    "../../data/shader/",   // 从build/Debug目录运行时
};

Shader::Shader() : m_programID(0) {}

Shader::~Shader() {
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
    }
}

Shader::Shader(Shader&& other) noexcept 
    : m_programID(other.m_programID)
    , m_uniformLocationCache(std::move(other.m_uniformLocationCache)) 
{
    other.m_programID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_programID != 0) {
            glDeleteProgram(m_programID);
        }
        m_programID = other.m_programID;
        m_uniformLocationCache = std::move(other.m_uniformLocationCache);
        other.m_programID = 0;
    }
    return *this;
}

bool Shader::LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource) {
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
        m_programID = 0;
        m_uniformLocationCache.clear();
    }
    
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) return false;
    
    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }
    
    bool success = LinkProgram(vertexShader, fragmentShader);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    if (success) {
        std::cout << "[Shader] Program linked successfully (ID: " << m_programID << ")" << std::endl;
    }
    
    return success;
}

bool Shader::LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
    std::cout << "[Shader] Loading: " << vertexPath << ", " << fragmentPath << std::endl;
    
    std::string vertexSource = ReadFile(vertexPath);
    std::string fragmentSource = ReadFile(fragmentPath);
    
    if (vertexSource.empty()) {
        std::cerr << "[Shader] ERROR: Failed to load vertex shader: " << vertexPath << std::endl;
        std::cerr << "         Searched paths:" << std::endl;
        for (const auto& path : s_shaderSearchPaths) {
            std::cerr << "           - " << path << vertexPath << std::endl;
        }
        return false;
    }
    
    if (fragmentSource.empty()) {
        std::cerr << "[Shader] ERROR: Failed to load fragment shader: " << fragmentPath << std::endl;
        return false;
    }
    
    return LoadFromSource(vertexSource, fragmentSource);
}

void Shader::Bind() const { glUseProgram(m_programID); }
void Shader::Unbind() const { glUseProgram(0); }

void Shader::SetBool(const std::string& name, bool value) {
    glUniform1i(GetUniformLocation(name), static_cast<int>(value));
}

void Shader::SetInt(const std::string& name, int value) {
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetFloat(const std::string& name, float value) {
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetVec2(const std::string& name, const Math::Vec2& value) {
    glUniform2f(GetUniformLocation(name), value.x, value.y);
}

void Shader::SetVec2(const std::string& name, float x, float y) {
    glUniform2f(GetUniformLocation(name), x, y);
}

void Shader::SetVec3(const std::string& name, const Math::Vec3& value) {
    glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
}

void Shader::SetVec3(const std::string& name, float x, float y, float z) {
    glUniform3f(GetUniformLocation(name), x, y, z);
}

void Shader::SetVec4(const std::string& name, const Math::Vec4& value) {
    glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::SetVec4(const std::string& name, float x, float y, float z, float w) {
    glUniform4f(GetUniformLocation(name), x, y, z, w);
}

void Shader::SetMat3(const std::string& name, const float* value) {
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, value);
}

void Shader::SetMat4(const std::string& name, const float* value) {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, value);
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::string shaderType = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        std::cerr << "ERROR: " << shaderType << " shader compilation failed:\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool Shader::LinkProgram(unsigned int vertexShader, unsigned int fragmentShader) {
    m_programID = glCreateProgram();
    glAttachShader(m_programID, vertexShader);
    glAttachShader(m_programID, fragmentShader);
    glLinkProgram(m_programID);
    
    int success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_programID, 512, nullptr, infoLog);
        std::cerr << "ERROR: Shader program linking failed:\n" << infoLog << std::endl;
        glDeleteProgram(m_programID);
        m_programID = 0;
        return false;
    }
    
    return true;
}

int Shader::GetUniformLocation(const std::string& name) {
    auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end()) return it->second;
    
    int location = glGetUniformLocation(m_programID, name.c_str());
    m_uniformLocationCache[name] = location;
    return location;
}

std::string Shader::ReadFile(const std::string& path) {
    // 尝试多个搜索路径
    for (const auto& searchPath : s_shaderSearchPaths) {
        std::string fullPath = searchPath + path;
        std::ifstream file(fullPath);
        
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::cout << "[Shader] Loaded: " << fullPath << std::endl;
            return buffer.str();
        }
    }
    
    // 所有路径都找不到
    return "";
}

} // namespace MiniEngine
