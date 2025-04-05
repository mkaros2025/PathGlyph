#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace PathGlyph {

class Shader {
public:
    // 构造函数 - 从文件加载顶点和片段着色器
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    
    // 析构函数 - 清理资源
    ~Shader();
    
    // 禁用拷贝
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    
    // 使用程序
    void use() const;
    
    // 设置uniform变量 - 只保留最常用的类型
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;
    
    // 设置实例化矩阵数组（用于实例化渲染）
    void setMat4Array(const std::string& name, const glm::mat4* values, int count) const;
    
    // 获取程序ID
    unsigned int getID() const { return m_programID; }
    
private:
    // 编译着色器
    void compile(const std::string& vertexCode, const std::string& fragmentCode);
    
    // 检查编译错误，返回编译是否成功
    bool checkCompileErrors(unsigned int shader, const std::string& type);
    
    // 获取uniform位置
    int getUniformLocation(const std::string& name) const;
    
    // 成员变量
    unsigned int m_programID;
    mutable std::unordered_map<std::string, int> m_uniformLocationCache;
};

} // namespace PathGlyph