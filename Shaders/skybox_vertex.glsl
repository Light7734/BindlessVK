#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec3 inColor;

layout(location = 0) out vec3 outUVW;

layout(std140, set = 0, binding = 0) uniform FrameData {
    mat4 projection;
    mat4 view;
    vec4 viewPos;
} U_FrameData;


layout(std140, set = 0, binding = 1) uniform SceneData {
    vec4 lightPos;
} U_SceneData;


void main()
{
    outUVW = inPosition;
    vec4 pos = U_FrameData.projection * mat4(mat3(U_FrameData.view)) * vec4(inPosition.xyz, 1.0);
    gl_Position = pos.xyww;
}
