#version 420 core

layout(location = 0) in vec3 aPos; // 顶点位置
layout(location = 1) in vec3 aColor; // 顶点颜色
layout(location = 2) in vec2 aTexCoord; // 纹理坐标

out vec3 vertexColor; // 传递给片段着色器的颜色
out vec2 texCoord; // 传递给片段着色器的纹理坐标

uniform mat4 projection; // 投影矩阵
uniform mat4 view; // 视图矩阵
uniform mat4 model; // 模型矩阵

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
    texCoord = aTexCoord;
}