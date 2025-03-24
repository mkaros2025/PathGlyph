#version 420 core

in vec3 vertexColor; // 从顶点着色器传递的颜色
out vec4 FragColor; // 输出的片段颜色

uniform float time; // 动态变化的时间，用于动画效果

void main() {
    // 动态颜色变化，模拟路径的流动效果
    float pulse = abs(sin(time * 2.0));
    vec3 animatedColor = mix(vertexColor, vec3(1.0, 1.0, 1.0), pulse);
    FragColor = vec4(animatedColor, 1.0);
}