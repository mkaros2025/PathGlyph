#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "geometry/model.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace PathGlyph {

bool Model::loadModel(ModelType type) {
    modelType_ = type;
    
    // 查找模型路径
    auto it = MODEL_PATHS.find(type);
    if (it == MODEL_PATHS.end()) {
        std::cerr << "找不到模型类型对应的路径" << std::endl;
        return false;
    }
    
    std::string modelPath = it->second;
    
    // 使用 tinygltf 加载模型
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    
    bool ret = false;
    // 根据文件扩展名决定加载方式
    if (modelPath.ends_with(".gltf")) {
        ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, modelPath);
    } else if (modelPath.ends_with(".glb")) {
        ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, modelPath);
    } else {
        std::cerr << "不支持的文件格式: " << modelPath << std::endl;
        return false;
    }
    if (!warn.empty()) {
        std::cout << "GLTF 警告: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "GLTF 错误: " << err << std::endl;
    }
    if (!ret) {
        std::cerr << "加载模型失败: " << modelPath << std::endl;
        return false;
    }
    
    return processModel(gltfModel);
}

bool Model::processModel(const tinygltf::Model& model) {
    // 清空之前的网格数据
    nodeMeshes_.clear();
    
    // 处理场景
    if (model.scenes.empty() || model.defaultScene < 0) {
        std::cerr << "模型没有默认场景" << std::endl;
        return false;
    }
    
    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    
    // 遍历场景中的所有节点
    glm::mat4 identityMatrix(1.0f);
    for (int nodeIndex : scene.nodes) {
        if (nodeIndex >= 0 && nodeIndex < static_cast<int>(model.nodes.size())) {
            if (!processNode(model.nodes[nodeIndex], model, identityMatrix)) {
                return false;
            }
        }
    }
    
    return true;
}

bool Model::processNode(const tinygltf::Node& node, const tinygltf::Model& model, const glm::mat4& parentMatrix) {
    glm::mat4 nodeMatrix = calculateNodeMatrix(node, model);
    glm::mat4 transformMatrix = parentMatrix * nodeMatrix;
    
    // 处理节点上的网格
    if (node.mesh >= 0 && node.mesh < static_cast<int>(model.meshes.size())) {
        const tinygltf::Mesh& mesh = model.meshes[node.mesh];
        
        for (const auto& primitive : mesh.primitives) {
            auto uniqueMesh = processMesh(primitive, model);
            if (uniqueMesh) {
                NodeMeshInfo nodeInfo;
                nodeInfo.mesh = std::move(uniqueMesh);
                nodeInfo.transform = transformMatrix;
                nodeMeshes_.push_back(std::move(nodeInfo));
            }
        }
    }
    
    // 递归处理子节点
    for (int childIndex : node.children) {
        if (childIndex >= 0 && childIndex < static_cast<int>(model.nodes.size())) {
            if (!processNode(model.nodes[childIndex], model, transformMatrix)) {
                return false;
            }
        }
    }
    
    return true;
}

std::unique_ptr<Mesh> Model::processMesh(const tinygltf::Primitive& primitive, const tinygltf::Model& model) {
    auto mesh = std::make_unique<Mesh>();
    
    // 提取顶点数据
    std::vector<Vertex> vertices;
    
    // 处理顶点位置
    if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
        int posAccessorIndex = primitive.attributes.at("POSITION");
        auto positions = getDataFromAccessor<glm::vec3>(model, posAccessorIndex);
        
        // 初始化顶点数组
        vertices.resize(positions.size());
        
        for (size_t i = 0; i < positions.size(); i++) {
            vertices[i].position = positions[i];
        }
        
        // 处理法线
        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
            int normalAccessorIndex = primitive.attributes.at("NORMAL");
            auto normals = getDataFromAccessor<glm::vec3>(model, normalAccessorIndex);
            
            if (normals.size() == vertices.size()) {
                for (size_t i = 0; i < normals.size(); i++) {
                    vertices[i].normal = normals[i];
                }
            }
        }
        
        // 处理纹理坐标
        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
            int texCoordAccessorIndex = primitive.attributes.at("TEXCOORD_0");
            auto texCoords = getDataFromAccessor<glm::vec2>(model, texCoordAccessorIndex);
            
            if (texCoords.size() == vertices.size()) {
                for (size_t i = 0; i < texCoords.size(); i++) {
                    vertices[i].texCoords = texCoords[i];
                }
            }
        }
        
        // 处理顶点颜色
        if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
            int colorAccessorIndex = primitive.attributes.at("COLOR_0");
            auto colors = getDataFromAccessor<glm::vec3>(model, colorAccessorIndex);
            
            if (colors.size() == vertices.size()) {
                for (size_t i = 0; i < colors.size(); i++) {
                    vertices[i].color = colors[i];
                }
            }
        } else {
            // 如果没有颜色数据，设置默认颜色
            for (auto& vertex : vertices) {
                vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
            }
        }
    }
    
    // 处理索引数据
    std::vector<unsigned int> indices;
    if (primitive.indices >= 0) {
        indices = getDataFromAccessor<unsigned int>(model, primitive.indices);
    } else {
        // 如果没有索引数据，创建默认索引（0, 1, 2, 3, ...）
        indices.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); i++) {
            indices[i] = static_cast<unsigned int>(i);
        }
    }
    
    // 设置顶点和索引数据
    mesh->setVertexData(vertices);
    mesh->setIndexData(indices);
    
    // 处理材质
    if (primitive.material >= 0 && primitive.material < static_cast<int>(model.materials.size())) {
        std::shared_ptr<Material> material = extractMaterial(model.materials[primitive.material], model);
        
        Mesh::Primitive meshPrimitive;
        meshPrimitive.indexOffset = 0; // 假设只有一个基元
        meshPrimitive.indexCount = indices.size();
        meshPrimitive.material = material;
        
        mesh->addPrimitive(meshPrimitive);
    }
    
    return mesh;
}

template<typename T>
std::vector<T> Model::getDataFromAccessor(const tinygltf::Model& model, int accessorIndex) {
    std::vector<T> data;
    
    if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size())) {
        std::cerr << "无效的访问器索引" << std::endl;
        return data;
    }
    
    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    
    const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
    size_t stride = accessor.ByteStride(bufferView);
    size_t count = accessor.count;
    
    data.resize(count);
    
    for (size_t i = 0; i < count; i++) {
        const unsigned char* elementPtr = dataPtr + i * stride;
        
        if constexpr (std::is_same_v<T, glm::vec2>) {
            // 读取2D向量
            switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    data[i] = glm::vec2(
                        *reinterpret_cast<const float*>(elementPtr),
                        *reinterpret_cast<const float*>(elementPtr + sizeof(float))
                    );
                    break;
                default:
                    std::cerr << "不支持的组件类型用于 glm::vec2" << std::endl;
                    break;
            }
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            // 读取3D向量
            switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    data[i] = glm::vec3(
                        *reinterpret_cast<const float*>(elementPtr),
                        *reinterpret_cast<const float*>(elementPtr + sizeof(float)),
                        *reinterpret_cast<const float*>(elementPtr + 2 * sizeof(float))
                    );
                    break;
                default:
                    std::cerr << "不支持的组件类型用于 glm::vec3" << std::endl;
                    break;
            }
        } else if constexpr (std::is_same_v<T, unsigned int>) {
            // 读取索引数据
            switch (accessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    data[i] = *reinterpret_cast<const unsigned int*>(elementPtr);
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    data[i] = static_cast<unsigned int>(*reinterpret_cast<const unsigned short*>(elementPtr));
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    data[i] = static_cast<unsigned int>(*elementPtr);
                    break;
                default:
                    std::cerr << "不支持的组件类型用于索引" << std::endl;
                    break;
            }
        } else {
            std::cerr << "不支持的数据类型" << std::endl;
        }
    }
    
    return data;
}

std::shared_ptr<Material> Model::extractMaterial(const tinygltf::Material& material, const tinygltf::Model& model) {
    auto mat = std::make_shared<Material>();
    
    // 处理基础颜色
    if (material.pbrMetallicRoughness.baseColorFactor.size() == 4) {
        mat->diffuse = glm::vec4(
            material.pbrMetallicRoughness.baseColorFactor[0],
            material.pbrMetallicRoughness.baseColorFactor[1],
            material.pbrMetallicRoughness.baseColorFactor[2],
            material.pbrMetallicRoughness.baseColorFactor[3]
        );
    }
    
    // 处理漫反射纹理
    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
        int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
        int sourceIndex = model.textures[textureIndex].source;
        
        if (sourceIndex >= 0 && sourceIndex < static_cast<int>(model.images.size())) {
            mat->diffuseMap = loadTexture(model.images[sourceIndex]);
        }
    }
    
    // 处理自发光
    if (material.emissiveTexture.index >= 0) {
        int textureIndex = material.emissiveTexture.index;
        int sourceIndex = model.textures[textureIndex].source;
        
        if (sourceIndex >= 0 && sourceIndex < static_cast<int>(model.images.size())) {
            mat->emissiveMap = loadTexture(model.images[sourceIndex]);
        }
    }
    
    // 设置自发光强度
    if (material.emissiveFactor.size() == 3) {
        // 使用发射因子的平均值作为强度
        mat->emissiveStrength = (
            material.emissiveFactor[0] +
            material.emissiveFactor[1] +
            material.emissiveFactor[2]
        ) / 3.0f;
    }
    
    // 可以根据粗糙度等因素设置光泽度
    if (material.pbrMetallicRoughness.roughnessFactor >= 0) {
        // 将粗糙度映射到光泽度（粗糙度越低，光泽度越高）
        mat->shininess = 128.0f * (1.0f - material.pbrMetallicRoughness.roughnessFactor);
    }
    
    return mat;
}

GLuint Model::loadTexture(const tinygltf::Image& image) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    GLenum format;
    if (image.component == 1) {
        format = GL_RED;
    } else if (image.component == 3) {
        format = GL_RGB;
    } else if (image.component == 4) {
        format = GL_RGBA;
    } else {
        std::cerr << "不支持的纹理组件数量: " << image.component << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
    
    // 上传纹理数据
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        image.width,
        image.height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        image.image.data()
    );
    
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return textureID;
}

glm::mat4 Model::calculateNodeMatrix(const tinygltf::Node& node, const tinygltf::Model& model) {
    // 检查模型类型
    if (modelType_ != ModelType::AGENT) {
        return glm::mat4(1.0f);
    }
    
    glm::mat4 nodeMatrix(1.0f); // 初始化为单位矩阵

    if (node.matrix.size() == 16) {
        glm::dmat4 tempMatrix = glm::make_mat4(node.matrix.data());
        nodeMatrix = glm::mat4(tempMatrix); // 将 double 矩阵转换为 float 矩阵
    } else {
        glm::mat4 translationMatrix(1.0f);
        glm::mat4 rotationMatrix(1.0f);
        glm::mat4 scaleMatrix(1.0f);

        if (node.translation.size() == 3) {
            translationMatrix = glm::translate(
                glm::mat4(1.0f),
                // 将 double 转换为 float 用于 glm::vec3 构造函数
                glm::vec3(static_cast<float>(node.translation[0]),
                          static_cast<float>(node.translation[1]),
                          static_cast<float>(node.translation[2]))
            );
        }

        if (node.rotation.size() == 4) {
            // GLM 四元数构造函数顺序为 (w, x, y, z)
            glm::quat rotationQuat(
                static_cast<float>(node.rotation[3]), // w
                static_cast<float>(node.rotation[0]), // x
                static_cast<float>(node.rotation[1]), // y
                static_cast<float>(node.rotation[2])  // z
            );
            // 将四元数转换为旋转矩阵
            rotationMatrix = glm::mat4_cast(rotationQuat);
        }

        if (node.scale.size() == 3) {
            scaleMatrix = glm::scale(
                glm::mat4(1.0f),
                // 将 double 转换为 float 用于 glm::vec3 构造函数
                glm::vec3(static_cast<float>(node.scale[0]),
                          static_cast<float>(node.scale[1]),
                          static_cast<float>(node.scale[2]))
            );
        }
        nodeMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    }

    return nodeMatrix;
}

} // namespace PathGlyph
