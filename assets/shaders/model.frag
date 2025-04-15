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
// uniform vec3 lightPos = vec3(5.0, 5.0, 5.0);  // 移除点光源位置
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);  // 光源颜色 (环境光和方向光共用)
uniform vec3 viewPos;        // 摄像机位置
uniform float ambientStrength = 0.1;  // 环境光强度 (从 0.0 恢复到 0.1)
uniform float specularStrength = 0.15;  // 镜面反射强度 (保持 0.15)

// --- 移除方向光，添加点光源位置 uniform --- 
// const vec3 lightDirection = normalize(vec3(0.5f, -1.0f, 0.5f)); // 移除
uniform vec3 lightPos; // 点光源位置 (从 C++ 设置)

// 渲染选项
uniform bool useVertexColor = false;  // 是否使用顶点颜色
uniform bool useModelColor = false;   // 是否使用模型自带颜色
uniform vec4 baseColor = vec4(1.0);   // 外部传入的基础颜色

// 添加缺少的uniform变量
uniform float emissiveStrength = 0.0;  // 自发光强度
uniform float alpha = 1.0;             // 全局透明度
uniform bool useTexture = true;        // 是否使用纹理

void main()
{   
    // 1. 确定基础颜色
    vec4 objectColor;
    
    if (useTexture && material.hasTexture) {
        // 使用纹理
        objectColor = texture(diffuseMap, TexCoord);
    } else if (useVertexColor) {
        // 使用顶点颜色
        objectColor = vec4(Color, 1.0);
    } else if (useModelColor) {
        // 使用模型材质颜色
        objectColor = material.diffuse;
    } else {
        // 使用外部传入的基础颜色
        objectColor = baseColor;
    }
    
    // 忽略完全透明的片段
    if (objectColor.a < 0.1)
        discard;
    
    // 2. 环境光 (ambientStrength 为 0.0，所以 ambient 为 0)
    vec3 ambient = ambientStrength * lightColor;
    
    // 3. 漫反射
    vec3 norm = normalize(Normal);  // 再次归一化法线
    // 恢复点光源方向计算
    vec3 lightDir = normalize(lightPos - FragPos);
    // vec3 lightDir = -lightDirection; // 移除方向光计算
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // 4. 镜面反射 (Blinn-Phong) (计算不变，使用新的 lightDir)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // 5. 合并光照 (ambient 现在是 0)
    vec3 lighting = ambient + diffuse + specular;
    
    // 6. 添加自发光效果
    lighting += emissiveStrength * objectColor.rgb;
    
    // 最终颜色
    FragColor = vec4(lighting * objectColor.rgb, objectColor.a * alpha);
} 