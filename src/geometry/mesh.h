#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "common/types.h"

namespace PathGlyph {

// 顶点数据结构
struct Vertex {
  glm::vec3 position;    // 位置坐标
  glm::vec3 normal;      // 法线向量
  // 用于将 2D 纹理图像（Texture）映射到 3D 模型表面
  glm::vec2 texCoords;   // 纹理坐标
  glm::vec3 color;       // 颜色
};

struct Material {
  glm::vec4 diffuse = glm::vec4(1.0f); // 漫反射颜色
  float shininess = 32.0f; // 光泽度，控制高光大小
  float emissiveStrength = 0.0f; // 自发光强度
  GLuint emissiveMap = 0; // 自发光贴图
  GLuint diffuseMap = 0;   // 漫反射纹理的 OpenGL 句柄
  
  bool hasTexture() const { return diffuseMap > 0; }
  
  // 析构函数，确保资源被正确释放
  ~Material() {
      if (diffuseMap > 0) {
          glDeleteTextures(1, &diffuseMap);
      }
      if (emissiveMap > 0) {
          glDeleteTextures(1, &emissiveMap);
      }
  }
};

class Mesh {
public:
  struct Primitive {
      // 表示这个图元的索引数据在 EBO 中的起始位置
      size_t indexOffset;
      // 表示这个图元需要多少个索引来绘制
      size_t indexCount;
      std::shared_ptr<Material> material;
  };

  Mesh();
  ~Mesh();

  void addPrimitive(const Primitive& primitive);
  void setVertexData(const std::vector<Vertex>& vertices);
  void setIndexData(const std::vector<unsigned int>& indices);
  
  // 渲染方法
  void render(class Shader* shader) const;
  
  // 实例化渲染方法
  void renderInstanced(class Shader* shader, uint32_t instanceCount) const;

  // 声明Model为友元类
  friend class Model;

private:
  GLuint VAO;
  GLuint VBO;
  GLuint EBO;  
  std::vector<Primitive> primitives;
};

struct NodeMeshInfo {
  std::shared_ptr<Mesh> mesh;
  glm::mat4 transform;
};

}