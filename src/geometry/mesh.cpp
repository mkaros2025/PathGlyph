#include "geometry/mesh.h"
#include "graphics/shader.h"
#include <iostream>

namespace PathGlyph {

Mesh::Mesh() : VAO(0), VBO(0), EBO(0) {
    // 初始化 VAO, VBO, EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
}

Mesh::~Mesh() {
    // 清理 OpenGL 资源
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void Mesh::addPrimitive(const Primitive& primitive) {
    primitives.push_back(primitive);
}

void Mesh::setVertexData(const std::vector<Vertex>& vertices) {
    glBindVertexArray(VAO);
    
    // 绑定并填充顶点缓冲
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // 设置顶点属性指针
    // 位置属性
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // 法线属性
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    
    // 纹理坐标属性
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    
    // 顶点颜色属性
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    
    // 解绑 VAO，避免后续操作影响当前设置
    glBindVertexArray(0);
}

void Mesh::setIndexData(const std::vector<unsigned int>& indices) {
    glBindVertexArray(VAO);
    
    // 绑定并填充索引缓冲
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // 解绑 VAO，避免后续操作影响当前设置
    glBindVertexArray(0);
}

void Mesh::render(Shader* shader) const {
    // 绑定当前网格的 VAO
    glBindVertexArray(VAO);
    
    // 遍历所有图元
    for (const auto& primitive : primitives) {
        // 设置材质属性
        if (primitive.material) {
            // 设置材质的基础属性
            shader->setVec4("material.diffuse", primitive.material->diffuse);
            shader->setFloat("material.shininess", primitive.material->shininess);
            shader->setBool("material.hasTexture", primitive.material->hasTexture());
            
            // 如果有漫反射纹理，绑定它
            if (primitive.material->hasTexture()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, primitive.material->diffuseMap);
                shader->setInt("diffuseMap", 0);
            }
            
            // 如果有自发光纹理，绑定它
            if (primitive.material->emissiveMap > 0) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, primitive.material->emissiveMap);
                shader->setInt("emissiveMap", 1);
                shader->setFloat("material.emissiveStrength", primitive.material->emissiveStrength);
            }
        }
        
        // 绘制图元
        glDrawElements(
            GL_TRIANGLES,                  // 图元类型
            static_cast<GLsizei>(primitive.indexCount), // 索引数量
            GL_UNSIGNED_INT,               // 索引类型
            reinterpret_cast<void*>(primitive.indexOffset * sizeof(unsigned int)) // 索引偏移
        );
    }
    
    // 解绑 VAO
    glBindVertexArray(0);
    
    // 重置纹理绑定
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::renderInstanced(Shader* shader, uint32_t instanceCount) const {
    // 绑定当前网格的 VAO
    glBindVertexArray(VAO);
    
    // 遍历所有图元
    for (const auto& primitive : primitives) {
        // 设置材质属性
        if (primitive.material) {
            // 设置材质的基础属性
            shader->setVec4("material.diffuse", primitive.material->diffuse);
            shader->setFloat("material.shininess", primitive.material->shininess);
            shader->setBool("material.hasTexture", primitive.material->hasTexture());
            
            // 如果有漫反射纹理，绑定它
            if (primitive.material->hasTexture()) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, primitive.material->diffuseMap);
                shader->setInt("diffuseMap", 0);
            }
            
            // 如果有自发光纹理，绑定它
            if (primitive.material->emissiveMap > 0) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, primitive.material->emissiveMap);
                shader->setInt("emissiveMap", 1);
                shader->setFloat("material.emissiveStrength", primitive.material->emissiveStrength);
            }
        }
        
        // 使用实例化渲染
        glDrawElementsInstanced(
            GL_TRIANGLES,                  // 图元类型
            static_cast<GLsizei>(primitive.indexCount), // 索引数量
            GL_UNSIGNED_INT,               // 索引类型
            reinterpret_cast<void*>(primitive.indexOffset * sizeof(unsigned int)), // 索引偏移
            instanceCount                  // 实例数量
        );
    }
    
    // 解绑 VAO
    glBindVertexArray(0);
    
    // 重置纹理绑定
    glActiveTexture(GL_TEXTURE0);
}

} // namespace PathGlyph
