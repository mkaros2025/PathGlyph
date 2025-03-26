#version 420 core

layout(location = 0) in vec3 aPos;      // 顶点位置
layout(location = 1) in vec2 aTexCoord; // 纹理坐标
layout(location = 2) in vec3 aNormal;   // 法线
layout(location = 3) in vec3 aColor;    // 颜色
layout(location = 4) in int aTextureID; // 纹理ID
layout(location = 5) in uint aElementType; // 元素类型

out vec3 vertexColor;       // 传递给片段着色器的颜色
out vec2 texCoord;          // 传递给片段着色器的纹理坐标
out vec3 normal;            // 传递给片段着色器的法线
out flat int textureID;     // 传递给片段着色器的纹理ID
out flat uint elementType;  // 传递给片段着色器的元素类型

uniform mat4 projection;    // 投影矩阵
uniform mat4 view;          // 视图矩阵

void main() {
    // 计算最终位置
    gl_Position = projection * view * vec4(aPos, 1.0);
    
    // 传递属性到片段着色器
    vertexColor = aColor;
    texCoord = aTexCoord;
    normal = aNormal;
    textureID = aTextureID;
    elementType = aElementType;
}