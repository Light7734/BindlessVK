#version 450

layout(location = 0) in vec3 inUVW;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D[32] texSamplers;
layout(set = 1, binding = 1) uniform samplerCube[8]  cubeTexSamplers;

void main()
{
    // outColor = vec4(1.0, 0.0, 0.0, 1.0);
    outColor = texture(cubeTexSamplers[0], inUVW);
}

