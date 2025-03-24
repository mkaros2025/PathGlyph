#pragma once
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <cstdlib> // 用于std::getenv

namespace PathGlyph {

class Shader {
public:
    // 指定着色器文件基础路径
    static const std::string SHADER_PATH;
    
    // 构造函数 - 从文件加载顶点和片段着色器
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    
    /* 暂时注释掉几何着色器支持
    // 构造函数 - 包含几何着色器
    Shader(const std::string& vertexPath, const std::string& geometryPath, 
           const std::string& fragmentPath);
    */
    
    // 构造函数 - 从字符串加载
    Shader(const std::string& vertexCode, const std::string& fragmentCode, bool isCode);
    
    // 析构函数 - 清理资源
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
    void setMat2(const std::string& name, const glm::mat2& value) const;
    void setMat3(const std::string& name, const glm::mat3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;
    
    // 获取程序ID
    unsigned int getID() const { return m_programID; }
    
private:
    // 简化为不带几何着色器的版本
    void compile(const std::string& vertexCode, const std::string& fragmentCode);
    
    // 检查着色器编译或链接错误
    void checkCompileErrors(unsigned int shader, const std::string& type);
    
    // 读取文件内容
    std::string readFile(const std::string& filename);
    
    // 获取文件的完整路径
    std::string getShaderPath(const std::string& filename);
    
    // 缓存uniform位置
    int getUniformLocation(const std::string& name) const;
    
    unsigned int m_programID;
    mutable std::unordered_map<std::string, int> m_uniformLocationCache;
};

} // namespace PathGlyph