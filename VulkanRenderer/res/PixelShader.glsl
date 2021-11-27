#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D uTexSampler;

void main() 
{
    outColor = texture(uTexSampler, fragUV);
}
