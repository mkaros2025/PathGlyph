#version 420 core

// 从顶点着色器接收输入
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Color;

// 输出
out vec4 FragColor;

// 材质结构体
struct Material {
    vec4 diffuse;            // 漫反射颜色
    float shininess;         // 光泽度
    bool hasTexture;         // 是否有纹理
};

// 材质参数
uniform Material material;
uniform sampler2D diffuseMap;  // 漫反射纹理

// 光照参数
uniform vec3 lightPos = vec3(5.0, 5.0, 5.0);  // 光源位置
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);  // 光源颜色
uniform vec3 viewPos;        // 摄像机位置
uniform float ambientStrength = 0.2;  // 环境光强度
uniform float specularStrength = 0.5;  // 镜面反射强度

// 渲染选项
uniform bool useVertexColor = false;  // 是否使用顶点颜色

// 添加缺少的uniform变量
uniform float emissiveStrength = 0.0;  // 自发光强度
uniform float alpha = 1.0;             // 全局透明度

void main()
{   
    // 1. 确定基础颜色
    vec4 baseColor;
    
    if (material.hasTexture) {
        // 使用纹理
        baseColor = texture(diffuseMap, TexCoord);
    } else {
        // 使用材质颜色
        baseColor = material.diffuse;
    }
    
    // 忽略完全透明的片段
    if (baseColor.a < 0.1)
        discard;
    
    // 2. 环境光
    vec3 ambient = ambientStrength * lightColor;
    
    // 3. 漫反射
    vec3 norm = normalize(Normal);  // 再次归一化法线
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 4. 镜面反射 (Blinn-Phong)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // 5. 合并光照
    vec3 lighting = ambient + diffuse + specular;
    
    // 6. 添加自发光效果
    lighting += emissiveStrength * baseColor.rgb;
    
    // 最终颜色
    FragColor = vec4(lighting * baseColor.rgb, baseColor.a * alpha);
} 