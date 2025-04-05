#include "shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace PathGlyph {

// 从文件加载着色器
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    // 初始化为0
    m_programID = 0;
    
    // 检查文件是否存在
    if (!fs::exists(vertexPath)) {
        throw std::runtime_error("顶点着色器文件不存在");
    }
    
    if (!fs::exists(fragmentPath)) {
        throw std::runtime_error("片段着色器文件不存在");
    }
    
    // 读取着色器代码
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    
    // 确保ifstream对象可以抛出异常
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        // 打开文件
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        
        // 读取文件缓冲区内容到流中
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        
        // 关闭文件
        vShaderFile.close();
        fShaderFile.close();
        
        // 将流转换为字符串
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        
        // 确保着色器代码非空
        if(vertexCode.empty() || fragmentCode.empty()) {
            std::cerr << "错误：着色器代码为空" << std::endl;
            throw std::runtime_error("空着色器代码");
        }
        
        // 编译着色器
        compile(vertexCode, fragmentCode);
        
        // 验证着色器程序ID
        if(m_programID == 0) {
            std::cerr << "错误：着色器程序创建失败" << std::endl;
            throw std::runtime_error("着色器程序创建失败");
        }
        
        std::cout << "着色器程序创建成功，ID: " << m_programID << std::endl;
        
    } catch (std::exception& e) {
        std::cerr << "着色器创建错误: " << e.what() << std::endl;
        std::cerr << "顶点着色器路径: " << vertexPath << std::endl;
        std::cerr << "片段着色器路径: " << fragmentPath << std::endl;
        
        // 确保m_programID是安全的
        if(m_programID != 0) {
            glDeleteProgram(m_programID);
            m_programID = 0;
        }
        
        throw; // 重新抛出异常
    }
}

// 析构函数
Shader::~Shader() {
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
        m_programID = 0;
    }
}

// 激活着色器程序
void Shader::use() const {
    // 检查this指针
    if (this == nullptr) {
        std::cerr << "严重错误: Shader对象是nullptr!" << std::endl;
        return;
    }
    
    // 添加检查
    if (m_programID == 0) {
        std::cerr << "错误: 着色器程序ID无效" << std::endl;
        return;
    }
    
    glUseProgram(m_programID);
}

// 编译着色器
void Shader::compile(const std::string& vertexCode, const std::string& fragmentCode) {
    unsigned int vertex = 0, fragment = 0;
    bool success = true;
    
    try {
        // 顶点着色器
        vertex = glCreateShader(GL_VERTEX_SHADER);
        if (vertex == 0) {
            std::cerr << "ERROR: Failed to create vertex shader" << std::endl;
            success = false;
            throw std::runtime_error("Failed to create vertex shader");
        }
        
        const char* vShaderCode = vertexCode.c_str();
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        if (!checkCompileErrors(vertex, "VERTEX")) {
            success = false;
            throw std::runtime_error("Vertex shader compilation failed");
        }
        
        // 片段着色器
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        if (fragment == 0) {
            std::cerr << "ERROR: Failed to create fragment shader" << std::endl;
            success = false;
            throw std::runtime_error("Failed to create fragment shader");
        }
        
        const char* fShaderCode = fragmentCode.c_str();
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        if (!checkCompileErrors(fragment, "FRAGMENT")) {
            success = false;
            throw std::runtime_error("Fragment shader compilation failed");
        }
        
        // 着色器程序
        m_programID = glCreateProgram();
        if (m_programID == 0) {
            std::cerr << "ERROR: Failed to create shader program" << std::endl;
            success = false;
            throw std::runtime_error("Failed to create shader program");
        }
        
        glAttachShader(m_programID, vertex);
        glAttachShader(m_programID, fragment);
        glLinkProgram(m_programID);
        if (!checkCompileErrors(m_programID, "PROGRAM")) {
            success = false;
            throw std::runtime_error("Shader program linking failed");
        }
    } 
    catch (const std::exception& e) {
        // 清理资源
        if (vertex != 0)
            glDeleteShader(vertex);
        if (fragment != 0)
            glDeleteShader(fragment);
        if (m_programID != 0) {
            glDeleteProgram(m_programID);
            m_programID = 0;
        }
        
        // 重新抛出异常
        throw;
    }
    
    // 删除着色器，它们已经链接到程序中，不再需要
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    if (!success) {
        m_programID = 0; // 确保ID为0，表示无效程序
    }
}

// 检查着色器编译/链接错误
bool Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" 
                      << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" 
                      << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            return false;
        }
    }
    return true;
}

// 获取uniform变量位置（带缓存）
int Shader::getUniformLocation(const std::string& name) const {
    // 检查缓存
    auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }
    
    // 查找uniform位置并缓存
    int location = glGetUniformLocation(m_programID, name.c_str());
    m_uniformLocationCache[name] = location;
    
    if (location == -1) {
        std::cerr << "WARNING: uniform '" << name << "' doesn't exist!" << std::endl;
    }
    
    return location;
}

// uniform设置函数的实现
void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

void Shader::setMat4Array(const std::string& name, const glm::mat4* values, int count) const {
    glUniformMatrix4fv(getUniformLocation(name), count, GL_FALSE, &values[0][0][0]);
}

} // namespace PathGlyph