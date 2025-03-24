#include "shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace PathGlyph {

const std::string Shader::SHADER_PATH = []() -> std::string {
  // 检查环境变量 'PATHGLYPH_SHADER_PATH'
  const char* envPath = std::getenv("PATHGLYPH_SHADER_PATH");
  if (envPath && *envPath) {
      std::string path(envPath);
      if (path.back() != '/' && path.back() != '\\') {
          path += '/';
      }
      return path;
  }
  return "../../../../assets/shaders/";
}();

// 从文件加载着色器
Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    // 读取着色器代码
    std::string vertexCode = readFile(getShaderPath(vertexPath));
    std::string fragmentCode = readFile(getShaderPath(fragmentPath));
    
    // 编译着色器
    compile(vertexCode, fragmentCode);
}

// 从字符串加载着色器
Shader::Shader(const std::string& vertexCode, const std::string& fragmentCode, bool isCode) {
    if (!isCode) {
        // 当 isCode 为 false 时，解释为从文件路径加载
        std::string vertexSource = readFile(getShaderPath(vertexCode));
        std::string fragmentSource = readFile(getShaderPath(fragmentCode));
        compile(vertexSource, fragmentSource);
    } else {
        // 直接使用提供的字符串代码
        compile(vertexCode, fragmentCode);
    }
}

// 析构函数
Shader::~Shader() {
    glDeleteProgram(m_programID);
}

// 激活着色器程序
void Shader::use() const {
    glUseProgram(m_programID);
}

// 编译着色器
void Shader::compile(const std::string& vertexCode, const std::string& fragmentCode) {
    unsigned int vertex, fragment;
    
    // 顶点着色器
    vertex = glCreateShader(GL_VERTEX_SHADER);
    const char* vShaderCode = vertexCode.c_str();
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    
    // 片段着色器
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fShaderCode = fragmentCode.c_str();
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");
    
    // 着色器程序
    m_programID = glCreateProgram();
    glAttachShader(m_programID, vertex);
    glAttachShader(m_programID, fragment);
    glLinkProgram(m_programID);
    checkCompileErrors(m_programID, "PROGRAM");
    
    // 删除着色器，它们已经链接到程序中，不再需要
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

// 检查着色器编译/链接错误
void Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" 
                      << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" 
                      << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

// 读取文件内容
std::string Shader::readFile(const std::string& filename) {
    std::string fileContent;
    std::ifstream fileStream;
    
    // 确保ifstream对象可以抛出异常
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        // 打开文件
        fileStream.open(filename);
        std::stringstream stream;
        
        // 读取文件缓冲区内容到流中
        stream << fileStream.rdbuf();
        
        // 关闭文件
        fileStream.close();
        
        // 将流转换为字符串
        fileContent = stream.str();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << filename << std::endl;
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
    return fileContent;
}

// 获取着色器文件路径
std::string Shader::getShaderPath(const std::string& filename) {
    return SHADER_PATH + filename;
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

void Shader::setMat2(const std::string& name, const glm::mat2& value) const {
    glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

void Shader::setMat3(const std::string& name, const glm::mat3& value) const {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

} // namespace PathGlyph