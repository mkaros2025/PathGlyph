#version 420 core

in vec3 vertexColor; // 从顶点着色器传递的颜色
in vec2 texCoord; // 从顶点着色器传递的纹理坐标
out vec4 FragColor; // 输出的片段颜色

uniform sampler2D texture1; // 纹理
uniform vec3 lightColor; // 光源颜色
uniform vec3 lightPos; // 光源位置
uniform vec3 viewPos; // 观察者位置

void main() {
    // 纹理颜色
    vec4 texColor = texture(texture1, texCoord);

    // 简单的光照计算
    vec3 norm = normalize(vec3(0.0, 0.0, 1.0)); // 假设法线朝向 Z 轴
    vec3 lightDir = normalize(lightPos - vec3(texCoord, 0.0));
    float diff = max(dot(norm, lightDir), 0.0);

    // 结合光照和纹理颜色
    vec3 result = (lightColor * diff + vertexColor) * texColor.rgb;
    FragColor = vec4(result, texColor.a); // 保留纹理的透明度
}