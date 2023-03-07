#version 450 core
#pragma shader_stage(fragment)

#extension GL_EXT_nonuniform_qualifier:enable

layout(location = 0) in vec3 inUVW;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform samplerCube cubeTexSamplers[8];
layout(set = 1, binding = 1) uniform sampler2D texSamplers[];

void main()
{
    // @note We need this for SPV-Reflect to detect
    // that texSampler has runtime array dimension...
    texSamplers[int(inUVW.x)]; 

    outColor = texture(cubeTexSamplers[0], inUVW);
}

