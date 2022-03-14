#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

layout(binding = 0) uniform uniMVP {
    mat4 model;
    mat4 view;
    mat4 proj;
} U_MVP;

void main() {
    gl_Position =  U_MVP.proj * U_MVP.view * U_MVP.model * vec4(inPosition, 1.0);
    outColor = inColor;
    outUV = inUV;
}

