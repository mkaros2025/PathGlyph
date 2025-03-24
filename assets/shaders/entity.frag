#version 420 core       

in vec3 vertexColor;       // 输入：顶点颜色（插值后）
out vec4 FragColor;        // 输出：片段颜色

uniform vec3 highlightColor; // 高亮颜色
uniform bool isHighlighted;  // 是否启用高亮

void main() {
    vec3 finalColor = isHighlighted ? highlightColor : vertexColor;
    FragColor = vec4(finalColor, 1.0); // 输出颜色（不透明）
}
