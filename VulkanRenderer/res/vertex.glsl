#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in uint inObjectIndex;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

layout(push_constant) uniform constants
{
    mat4 projection;
    mat4 view;
} U_PushConstants;

struct ObjectData {
    mat4 model;
};

layout(std140, set = 0, binding = 0) readonly buffer ObjectBuffer{
    ObjectData objects[];
} B_ObjectBuffer;

void main() {
    gl_Position =   U_PushConstants.projection * U_PushConstants.view *  B_ObjectBuffer.objects[inObjectIndex].model * vec4(inPosition, 1.0);
    outUV = inUV;
    outColor = inColor;
}

