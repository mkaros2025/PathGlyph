#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../common/Types.h"
#include <tiny_gltf.h>

namespace PathGlyph {

// 顶点数据结构
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 color;
};

// 简化的材质结构
struct Material {
    glm::vec4 diffuse = glm::vec4(1.0f);
    float shininess = 32.0f;
    GLuint diffuseMap = 0;
    
    bool hasTexture() const { return diffuseMap > 0; }
};

// 网格类 - 只存储数据，不执行渲染
class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, 
         const Material& material = Material());
    ~Mesh();
    
    // 数据访问方法
    GLuint getVAO() const { return VAO; }
    unsigned int getIndexCount() const { return indexCount_; }
    const Material& getMaterial() const { return material_; }
    
private:
    // OpenGL资源
    GLuint VAO, VBO, EBO;
    unsigned int indexCount_;
    Material material_;
};

// 模型类 - 只负责加载和管理数据
class Model {
public:
    struct BoundingBox {
        glm::vec3 min;
        glm::vec3 max;
    };

    Model();
    ~Model();
    
    // 简化后的加载函数，直接根据类型加载模型
    bool loadModel(ModelType type);
    
    // 数据访问方法
    const std::vector<std::unique_ptr<Mesh>>& getMeshes() const { return meshes_; }
    ModelType getModelType() const { return modelType_; }
    const BoundingBox& getBoundingBox() const { return boundingBox_; }
    
    // 设置属性
    void setModelType(ModelType type) { modelType_ = type; }
    
private:
    ModelType modelType_ = ModelType::GROUND;
    std::vector<std::unique_ptr<Mesh>> meshes_;
    BoundingBox boundingBox_;
    
    // tinygltf相关
    static const std::unordered_map<ModelType, std::string> MODEL_PATHS;
    
    // 辅助函数
    void calculateBoundingBox();
    
    // 处理模型数据
    bool processModel(const tinygltf::Model& model);
    std::unique_ptr<Mesh> processMesh(const tinygltf::Primitive& primitive, const tinygltf::Model& model);
    Material extractMaterial(const tinygltf::Material& material, const tinygltf::Model& model);
    GLuint loadTexture(const tinygltf::Image& image);
    
    // 从Buffer提取数据
    template<typename T>
    std::vector<T> getDataFromAccessor(const tinygltf::Model& model, int accessorIndex);
};

} // namespace PathGlyph 