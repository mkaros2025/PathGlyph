#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace PathGlyph {

class Shader {
public:
    Shader();
    ~Shader();
    
    // 禁用拷贝
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    
    // 使用程序
    void use() const;
    
    // 设置uniform变量
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat3(const std::string& name, const glm::mat3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;
    
    // 设置实例化矩阵数组（用于实例化渲染）
    void setMat4Array(const std::string& name, const glm::mat4* values, int count) const;
    
    unsigned int getID() const { return m_programID; }
    
private:
    // 编译着色器
    void compile(const std::string& vertexCode, const std::string& fragmentCode);
    
    bool checkCompileErrors(unsigned int shader, const std::string& type);
    
    int getUniformLocation(const std::string& name) const;
    
    unsigned int m_programID;
    mutable std::unordered_map<std::string, int> m_uniformLocationCache;
};

} // namespace PathGlyph