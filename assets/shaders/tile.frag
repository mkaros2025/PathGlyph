#version 420 core

in vec3 vertexColor;        // 从顶点着色器传递的颜色
in vec2 texCoord;           // 从顶点着色器传递的纹理坐标
in vec3 normal;             // 从顶点着色器传递的法线
in flat int textureID;      // 从顶点着色器传递的纹理ID
in flat uint elementType;   // 从顶点着色器传递的元素类型

out vec4 FragColor;         // 输出的片段颜色

uniform sampler2D textures[8]; // 纹理数组
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);  // 光源颜色
uniform vec3 lightPos = vec3(0.0, 0.0, 100.0);  // 光源位置
uniform vec3 viewPos = vec3(0.0, 0.0, 100.0);   // 观察者位置

// 元素类型常量
const uint TYPE_TILE = 0;
const uint TYPE_PATH = 1;
const uint TYPE_OBSTACLE = 2;
const uint TYPE_AGENT = 3;

// 基本光照计算
vec3 calculateBasicLighting(vec3 color) {
    // 环境光
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // 漫反射
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - gl_FragCoord.xyz);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 高光
    float specularStrength = 0.1;
    vec3 viewDir = normalize(viewPos - gl_FragCoord.xyz);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    return (ambient + diffuse + specular) * color;
}

void main() {
    vec4 result;
    
    // 根据元素类型进行不同的渲染
    switch(elementType) {
        case TYPE_TILE:
            // 图块使用纹理和基本光照
            if (textureID > 0 && textureID < 8) {
                vec4 texColor = texture(textures[textureID], texCoord);
                vec3 litColor = calculateBasicLighting(texColor.rgb * vertexColor);
                result = vec4(litColor, texColor.a);
            } else {
                // 无纹理，使用顶点颜色
                result = vec4(calculateBasicLighting(vertexColor), 1.0);
            }
            break;
            
        case TYPE_PATH:
            // 路径使用顶点颜色，增加一些发光效果
            result = vec4(vertexColor * 1.2, 0.7); // 半透明
            break;
            
        case TYPE_OBSTACLE:
            // 障碍物使用顶点颜色和基本光照
            result = vec4(calculateBasicLighting(vertexColor), 1.0);
            break;
            
        case TYPE_AGENT:
            // 代理使用顶点颜色，增加发光效果
            result = vec4(vertexColor * 1.3, 0.9); // 稍微透明
            break;
            
        default:
            // 默认使用顶点颜色
            result = vec4(vertexColor, 1.0);
    }
    
    FragColor = result;
}