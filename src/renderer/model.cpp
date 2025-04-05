// 首先定义实现宏
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#define STBI_FAILURE_USERMSG
// 禁用异常以避免问题
#define JSON_NOEXCEPTION
#define TINYGLTF_NOEXCEPTION
// 使用C++14特性
#define TINYGLTF_USE_CPP14

// 然后包含必要的头文件
#include "model.h"
#include <iostream>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace PathGlyph {

// 定义模型类型到文件路径的映射
const std::unordered_map<ModelType, std::string> Model::MODEL_PATHS = {
    {ModelType::GROUND, "../../../../assets/models/ground.glb"},
    {ModelType::PATH, "../../../../assets/models/path.glb"},
    {ModelType::OBSTACLE, "../../../../assets/models/obstacle.glb"},
    {ModelType::START, "../../../../assets/models/start.glb"},
    {ModelType::GOAL, "../../../../assets/models/goal.glb"},
    {ModelType::AGENT, "../../../../assets/models/agent.glb"}
};

//-----------------------------------------------------------------------------
// Mesh 实现
//-----------------------------------------------------------------------------

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, 
          const Material& material)
    : indexCount_(indices.size()), material_(material) {
    
    // 创建OpenGL缓冲区对象
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    // 绑定顶点数组对象
    glBindVertexArray(VAO);
    
    // 绑定和设置顶点缓冲区
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    // 绑定和设置索引缓冲区
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // 设置顶点属性指针
    // 位置
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    // 法线
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // 纹理坐标
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    // 颜色
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    
    // 解绑
    glBindVertexArray(0);
}

Mesh::~Mesh() {
    // 清理OpenGL资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    
    // 清理纹理
    if (material_.hasTexture()) {
        glDeleteTextures(1, &material_.diffuseMap);
    }
}

//-----------------------------------------------------------------------------
// Model 实现
//-----------------------------------------------------------------------------

Model::Model() : modelType_(ModelType::GROUND) {
    // 初始化包围盒为无效值
    boundingBox_.min = glm::vec3(std::numeric_limits<float>::max());
    boundingBox_.max = glm::vec3(std::numeric_limits<float>::lowest());
}

Model::~Model() {
    // Mesh的析构函数会负责清理资源
}

bool Model::loadModel(ModelType type) {
    modelType_ = type;
    
    auto it = MODEL_PATHS.find(type);
    if (it == MODEL_PATHS.end()) {
        std::cerr << "未找到类型对应的模型路径: " << static_cast<int>(type) << std::endl;
        return false;
    }
    
    std::string modelPath = it->second; // 创建副本，可以修改
    
    // 输出当前工作目录
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::cout << "当前工作目录: " << cwd << std::endl;
    }
    
    // 检查文件是否存在
    std::cout << "尝试加载模型: " << modelPath << std::endl;
    if (!std::filesystem::exists(modelPath)) {
        std::cout << "文件不存在，尝试查找绝对路径..." << std::endl;
        // 尝试其他路径
        std::string alt_path = "assets/models/" + std::filesystem::path(modelPath).filename().string();
        std::cout << "尝试替代路径1: " << alt_path << std::endl;
        if (!std::filesystem::exists(alt_path)) {
            std::string alt_path2 = "../assets/models/" + std::filesystem::path(modelPath).filename().string();
            std::cout << "尝试替代路径2: " << alt_path2 << std::endl;
            if (!std::filesystem::exists(alt_path2)) {
                std::string alt_path3 = "../../assets/models/" + std::filesystem::path(modelPath).filename().string();
                std::cout << "尝试替代路径3: " << alt_path3 << std::endl;
                if (!std::filesystem::exists(alt_path3)) {
                    std::cerr << "无法找到模型文件: " << modelPath << std::endl;
                    return false;
                } else {
                    modelPath = alt_path3;
                }
            } else {
                modelPath = alt_path2;
            }
        } else {
            modelPath = alt_path;
        }
        std::cout << "将使用路径: " << modelPath << std::endl;
    }
    
    // 使用tinygltf加载模型
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    bool ret = false;
    
    // 根据文件扩展名确定加载方法
    std::string ext = std::filesystem::path(modelPath).extension().string();
    std::cout << "文件扩展名: " << ext << std::endl;
    
    if (ext == ".glb") {
        std::cout << "加载二进制GLTF文件..." << std::endl;
        ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, modelPath);
    } else if (ext == ".gltf") {
        std::cout << "加载ASCII GLTF文件..." << std::endl;
        ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, modelPath);
    } else {
        std::cerr << "不支持的文件格式: " << ext << std::endl;
        return false;
    }
    
    // 检查加载结果
    if (!warn.empty()) {
        std::cout << "GLTF警告: " << warn << std::endl;
    }
    
    if (!err.empty()) {
        std::cerr << "GLTF错误: " << err << std::endl;
        return false;
    }
    
    if (!ret) {
        std::cerr << "加载模型失败: " << modelPath << std::endl;
        return false;
    }
    
    std::cout << "GLTF模型加载成功，检查结构..." << std::endl;
    // 打印模型信息
    std::cout << "场景数: " << gltfModel.scenes.size() 
              << ", 网格数: " << gltfModel.meshes.size() 
              << ", 材质数: " << gltfModel.materials.size() 
              << ", 节点数: " << gltfModel.nodes.size() << std::endl;
    
    // 清除现有的网格
    meshes_.clear();
    
    // 处理模型
    if (!processModel(gltfModel)) {
        std::cerr << "处理模型失败" << std::endl;
        return false;
    }
    
    // 计算包围盒
    calculateBoundingBox();
    
    std::cout << "模型 " << static_cast<int>(type) << " 加载完成，共创建了 " << meshes_.size() << " 个网格" << std::endl;
    return true;
}

bool Model::processModel(const tinygltf::Model& model) {
    std::cout << "开始处理模型..." << std::endl;
    
    if (model.meshes.empty()) {
        std::cerr << "模型没有网格！" << std::endl;
        return false;
    }
    
    // 处理场景中的所有网格
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const auto& mesh = model.meshes[i];
        std::cout << "处理网格 " << i << ": " << mesh.name << ", 图元数: " << mesh.primitives.size() << std::endl;
        
        for (size_t j = 0; j < mesh.primitives.size(); j++) {
            const auto& primitive = mesh.primitives[j];
            std::cout << "  处理图元 " << j << ", 属性数: " << primitive.attributes.size() << std::endl;
            
            auto newMesh = processMesh(primitive, model);
            if (newMesh) {
                meshes_.push_back(std::move(newMesh));
                std::cout << "  图元处理成功，添加到网格列表" << std::endl;
            } else {
                std::cout << "  图元处理失败，跳过" << std::endl;
            }
        }
    }
    
    std::cout << "模型处理完成，创建了 " << meshes_.size() << " 个网格" << std::endl;
    return !meshes_.empty();
}

std::unique_ptr<Mesh> Model::processMesh(const tinygltf::Primitive& primitive, const tinygltf::Model& model) {
    // 检查必要属性是否存在
    if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
        std::cerr << "网格缺少位置属性" << std::endl;
        return nullptr;
    }
    
    std::cout << "开始处理网格数据..." << std::endl;
    
    // 输出所有属性
    std::cout << "网格属性列表:" << std::endl;
    for (const auto& attr : primitive.attributes) {
        std::cout << "  " << attr.first << ": 索引 " << attr.second << std::endl;
    }
    
    // 提取顶点位置
    std::vector<glm::vec3> positions;
    int posAccessor = primitive.attributes.at("POSITION");
    positions = getDataFromAccessor<glm::vec3>(model, posAccessor);
    std::cout << "顶点位置数: " << positions.size() << std::endl;
    
    // 输出前几个顶点位置用于调试
    std::cout << "顶点位置示例:" << std::endl;
    for (size_t i = 0; i < std::min(positions.size(), (size_t)5); i++) {
        std::cout << "  顶点" << i << ": (" << positions[i].x << ", " << positions[i].y << ", " << positions[i].z << ")" << std::endl;
    }
    
    // 提取法线（如果有）
    std::vector<glm::vec3> normals;
    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
        int normalAccessor = primitive.attributes.at("NORMAL");
        normals = getDataFromAccessor<glm::vec3>(model, normalAccessor);
        std::cout << "法线数: " << normals.size() << std::endl;
    } else {
        // 如果没有法线，创建默认法线
        normals.resize(positions.size(), glm::vec3(0.0f, 1.0f, 0.0f));
        std::cout << "未找到法线数据，使用默认法线" << std::endl;
    }
    
    // 提取纹理坐标（如果有）
    std::vector<glm::vec2> texCoords;
    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
        int texAccessor = primitive.attributes.at("TEXCOORD_0");
        texCoords = getDataFromAccessor<glm::vec2>(model, texAccessor);
        std::cout << "纹理坐标数: " << texCoords.size() << std::endl;
    } else {
        // 如果没有纹理坐标，使用默认值
        texCoords.resize(positions.size(), glm::vec2(0.0f));
        std::cout << "未找到纹理坐标数据，使用默认纹理坐标" << std::endl;
    }
    
    // 提取颜色（如果有）
    std::vector<glm::vec3> colors;
    if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
        int colorAccessor = primitive.attributes.at("COLOR_0");
        colors = getDataFromAccessor<glm::vec3>(model, colorAccessor);
        std::cout << "顶点颜色数: " << colors.size() << std::endl;
    } else {
        // 如果没有颜色，使用默认白色
        colors.resize(positions.size(), glm::vec3(1.0f));
        std::cout << "未找到顶点颜色数据，使用默认白色" << std::endl;
    }
    
    // 提取索引（如果有）
    std::vector<unsigned int> indices;
    if (primitive.indices >= 0) {
        indices = getDataFromAccessor<unsigned int>(model, primitive.indices);
        std::cout << "索引数: " << indices.size() << std::endl;
        
        // 输出前几个索引用于调试
        std::cout << "索引示例:" << std::endl;
        for (size_t i = 0; i < std::min(indices.size(), (size_t)15); i += 3) {
            if (i + 2 < indices.size()) {
                std::cout << "  三角形" << i/3 << ": " << indices[i] << ", " << indices[i+1] << ", " << indices[i+2] << std::endl;
            }
        }
    } else {
        // 如果没有索引，创建默认索引（按顶点顺序）
        indices.resize(positions.size());
        for (size_t i = 0; i < positions.size(); i++) {
            indices[i] = static_cast<unsigned int>(i);
        }
        std::cout << "未找到索引数据，使用默认索引" << std::endl;
    }
    
    // 创建顶点结构体数组
    std::vector<Vertex> vertices(positions.size());
    for (size_t i = 0; i < positions.size(); i++) {
        vertices[i].position = positions[i];
        vertices[i].normal = i < normals.size() ? normals[i] : glm::vec3(0.0f, 1.0f, 0.0f);
        vertices[i].texCoords = i < texCoords.size() ? texCoords[i] : glm::vec2(0.0f);
        vertices[i].color = i < colors.size() ? colors[i] : glm::vec3(1.0f);
    }
    
    // 处理材质
    Material material;
    if (primitive.material >= 0) {
        std::cout << "处理材质，索引: " << primitive.material << std::endl;
        material = extractMaterial(model.materials[primitive.material], model);
    } else {
        std::cout << "未找到材质，使用默认材质" << std::endl;
    }
    
    // 创建网格
    std::cout << "创建网格对象，顶点数: " << vertices.size() << ", 索引数: " << indices.size() << std::endl;
    return std::make_unique<Mesh>(vertices, indices, material);
}

Material Model::extractMaterial(const tinygltf::Material& material, const tinygltf::Model& model) {
    Material result;
    
    // 设置基本颜色
    if (material.values.find("baseColorFactor") != material.values.end()) {
        auto& colorArray = material.values.at("baseColorFactor");
        if (colorArray.number_array.size() >= 4) {
            result.diffuse = glm::vec4(
                colorArray.number_array[0],
                colorArray.number_array[1],
                colorArray.number_array[2],
                colorArray.number_array[3]
            );
        }
    }
    
    // 设置光泽度
    if (material.values.find("roughnessFactor") != material.values.end()) {
        auto& roughness = material.values.at("roughnessFactor");
        // 将粗糙度转换为光泽度（粗糙度越低，光泽度越高）
        result.shininess = (1.0f - static_cast<float>(roughness.number_value)) * 100.0f;
    }
    
    // 加载漫反射纹理
    if (material.values.find("baseColorTexture") != material.values.end()) {
        auto& texInfo = material.values.at("baseColorTexture");
        int texIndex = static_cast<int>(texInfo.TextureIndex());
        int srcTexture = model.textures[texIndex].source;
        
        if (srcTexture >= 0) {
            result.diffuseMap = loadTexture(model.images[srcTexture]);
        }
    }
    
    return result;
}

GLuint Model::loadTexture(const tinygltf::Image& image) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    
    GLenum format;
    if (image.component == 1)
        format = GL_RED;
    else if (image.component == 3)
        format = GL_RGB;
    else if (image.component == 4)
        format = GL_RGBA;
    else
        format = GL_RGB;
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.image.data());
    
    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // 生成mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return textureID;
}

void Model::calculateBoundingBox() {
    // 重置包围盒
    boundingBox_.min = glm::vec3(std::numeric_limits<float>::max());
    boundingBox_.max = glm::vec3(std::numeric_limits<float>::lowest());
    
    // 遍历所有网格并收集顶点位置
    for (const auto& mesh : meshes_) {
        // 这里需要访问Mesh的顶点，但我们已经移除了对顶点的存储
        // 因此这个逻辑需要修改，例如在Mesh中添加一个获取包围盒的方法
        // 简单起见，我们假设已经知道包围盒
        
        // 注意：实际实现时需要从VAO中提取顶点或在Mesh构造时计算包围盒
        // 下面的代码仅为演示，实际应该由Mesh提供包围盒信息
        
        // 假设每个网格的包围盒为(-1,-1,-1)到(1,1,1)
        boundingBox_.min = glm::min(boundingBox_.min, glm::vec3(-1.0f));
        boundingBox_.max = glm::max(boundingBox_.max, glm::vec3(1.0f));
    }
    
    // 如果没有网格，设置默认包围盒
    if (meshes_.empty()) {
        boundingBox_.min = glm::vec3(-0.5f);
        boundingBox_.max = glm::vec3(0.5f);
    }
}

// 从Buffer提取数据的模板函数实现
template<typename T>
std::vector<T> Model::getDataFromAccessor(const tinygltf::Model& model, int accessorIndex) {
    std::vector<T> result;
    
    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    
    // 计算元素数量和每个元素的字节大小
    size_t elementCount = accessor.count;
    int elementSize = tinygltf::GetComponentSizeInBytes(accessor.componentType) * 
                      tinygltf::GetNumComponentsInType(accessor.type);
    
    // 提取数据
    result.resize(elementCount);
    const unsigned char* data = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
    
    // 确定正确的步幅
    size_t byteStride = bufferView.byteStride;
    if (byteStride == 0) {
        // 如果byteStride为0，使用紧凑的布局
        byteStride = elementSize;
    }
    
    // 输出调试信息
    std::cout << "数据提取 - 类型: " << accessor.type 
              << ", 元素数: " << elementCount 
              << ", 步幅: " << byteStride 
              << ", 元素大小: " << elementSize << std::endl;
    
    // 根据不同的分量类型和数据类型进行处理
    if (std::is_same<T, glm::vec2>::value) {
        for (size_t i = 0; i < elementCount; i++) {
            glm::vec2& elem = *reinterpret_cast<glm::vec2*>(&result[i]);
            const float* floatData = reinterpret_cast<const float*>(data + i * byteStride);
            elem.x = floatData[0];
            elem.y = floatData[1];
        }
    } else if (std::is_same<T, glm::vec3>::value) {
        for (size_t i = 0; i < elementCount; i++) {
            glm::vec3& elem = *reinterpret_cast<glm::vec3*>(&result[i]);
            const float* floatData = reinterpret_cast<const float*>(data + i * byteStride);
            elem.x = floatData[0];
            elem.y = floatData[1];
            elem.z = floatData[2];
        }
    } else if (std::is_same<T, unsigned int>::value) {
        // 处理不同类型的索引
        switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                for (size_t i = 0; i < elementCount; i++) {
                    unsigned char value = *(data + i * byteStride);
                    *reinterpret_cast<unsigned int*>(&result[i]) = static_cast<unsigned int>(value);
                }
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                for (size_t i = 0; i < elementCount; i++) {
                    unsigned short value = *reinterpret_cast<const unsigned short*>(data + i * byteStride);
                    *reinterpret_cast<unsigned int*>(&result[i]) = static_cast<unsigned int>(value);
                }
                break;
            }
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                for (size_t i = 0; i < elementCount; i++) {
                    *reinterpret_cast<unsigned int*>(&result[i]) = *reinterpret_cast<const unsigned int*>(data + i * byteStride);
                }
                break;
            }
        }
    }
    
    return result;
}

// 明确实例化模板函数以避免链接错误
template std::vector<glm::vec2> Model::getDataFromAccessor<glm::vec2>(const tinygltf::Model&, int);
template std::vector<glm::vec3> Model::getDataFromAccessor<glm::vec3>(const tinygltf::Model&, int);
template std::vector<unsigned int> Model::getDataFromAccessor<unsigned int>(const tinygltf::Model&, int);

} // namespace PathGlyph 