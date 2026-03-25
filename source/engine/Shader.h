/**
 * Shader.h - 着色器管理类
 */

#ifndef MINI_ENGINE_SHADER_H
#define MINI_ENGINE_SHADER_H

#include <string>
#include <unordered_map>
#include "math/Math.h"

namespace MiniEngine {

class Shader {
public:
    Shader();
    ~Shader();
    
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;
    
    bool LoadFromSource(const std::string& vertexSource, const std::string& fragmentSource);
    bool LoadFromFile(const std::string& vertexPath, const std::string& fragmentPath);
    
    void Bind() const;
    void Unbind() const;
    
    bool IsValid() const { return m_programID != 0; }
    unsigned int GetProgramID() const { return m_programID; }
    
    // 设置Uniform变量
    void SetBool(const std::string& name, bool value);
    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetVec2(const std::string& name, const Math::Vec2& value);
    void SetVec2(const std::string& name, float x, float y);
    void SetVec3(const std::string& name, const Math::Vec3& value);
    void SetVec3(const std::string& name, float x, float y, float z);
    void SetVec4(const std::string& name, const Math::Vec4& value);
    void SetVec4(const std::string& name, float x, float y, float z, float w);
    void SetMat3(const std::string& name, const float* value);
    void SetMat4(const std::string& name, const float* value);
    
private:
    unsigned int CompileShader(unsigned int type, const std::string& source);
    bool LinkProgram(unsigned int vertexShader, unsigned int fragmentShader);
    int GetUniformLocation(const std::string& name);
    std::string ReadFile(const std::string& path);
    
    unsigned int m_programID = 0;
    std::unordered_map<std::string, int> m_uniformLocationCache;
};

} // namespace MiniEngine

#endif // MINI_ENGINE_SHADER_H
