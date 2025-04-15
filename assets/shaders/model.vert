#version 420 core

// 顶点属性
layout (location = 0) in vec3 aPos;       // 位置
layout (location = 1) in vec3 aNormal;    // 法线
layout (location = 2) in vec2 aTexCoord;  // 纹理坐标
layout (location = 3) in vec3 aColor;     // 顶点颜色

// 输出到片段着色器
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 Color;

// 变换矩阵
uniform mat4 model;          // 模型变换
uniform mat4 view;           // 视图变换
uniform mat4 projection;     // 投影变换
uniform mat4 nodeTransform;  // 节点自身的变换
uniform bool isInstanced;    // 是否使用实例化渲染
uniform float modelScale = 1.0;  // 模型统一缩放因子

// 实例化变换矩阵数组
uniform mat4 instanceTransforms[500];  // 最多支持500个实例

void main()
{
    // 调试输出 - 确保顶点颜色正确传递
    Color = aColor;
    
    // 根据是否实例化选择模型矩阵
    // 使用gl_InstanceID内置变量代替uniform instanceID
    mat4 instanceMatrix = isInstanced ? instanceTransforms[gl_InstanceID] : model;
    
    mat4 modelMatrix = instanceMatrix * nodeTransform; // 暂时注释掉局部变换的乘法
    
    // 计算世界空间位置
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    
    // 计算法线变换 (使用法线矩阵，即模型矩阵的转置的逆矩阵)
    // 法线计算也要用修改后的 modelMatrix
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    Normal = normalize(normalMatrix * aNormal);
    
    // 纹理坐标
    TexCoord = aTexCoord;
    
    // 位置变换 (视图空间+投影)
    gl_Position = projection * view * vec4(FragPos, 1.0);
} 