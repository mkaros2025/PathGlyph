#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "common/types.h"
#include "geometry/mesh.h"
#include <tiny_gltf.h>

namespace PathGlyph {

class Model {
public:
    // 添加默认构造函数
    Model() = default;
    
    // 禁用拷贝
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // 公共接口 - 模型加载和管理
    bool loadModel(ModelType type);
    bool processModel(const tinygltf::Model& model);
    bool processNode(const tinygltf::Node& node, const tinygltf::Model& model, const glm::mat4& parentMatrix);
    std::unique_ptr<Mesh> processMesh(const tinygltf::Primitive& primitive, const tinygltf::Model& model);

    // 公共接口 - 数据访问
    ModelType getModelType() const { return modelType_; }
    const std::vector<NodeMeshInfo>& getNodeMeshes() const { return nodeMeshes_; }
    template<typename T>
    std::vector<T> getDataFromAccessor(const tinygltf::Model& model, int accessorIndex);

    // 公共接口 - 属性设置
    void setModelType(ModelType type) { modelType_ = type; }

    // 从 gltf 中提取材质信息并存储到 Material 中
    std::shared_ptr<Material> extractMaterial(const tinygltf::Material& material, const tinygltf::Model& model);
    // 从将纹理信息加载到 gpu
    GLuint loadTexture(const tinygltf::Image& image);

    // 计算的是当前节点的局部变换矩阵，表示当前节点相对于模型中其他节点的位置信息、大小信息和方向信息
    glm::mat4 calculateNodeMatrix(const tinygltf::Node& node, const tinygltf::Model& model);
    
private:
    ModelType modelType_ = ModelType::GROUND;
    std::vector<NodeMeshInfo> nodeMeshes_;
};

} // namespace PathGlyph
