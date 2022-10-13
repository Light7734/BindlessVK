#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec3 inColor;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;
layout (location = 5) out vec3 outFragPos;
layout (location = 6) out flat int outTexIndex;
layout (location = 7) out mat3 outTBN;

layout(std140, set = 0, binding = 0) uniform FrameData
{
    mat4 projection;
    mat4 view;
    vec4 lightPos;
    vec4 viewPos;
} U_FrameData;

struct ObjectData {
    mat4 model;
};


void main() {
    gl_Position = U_FrameData.projection * U_FrameData.view *  mat4(1.0) * vec4(inPosition, 1.0);

    outFragPos = (mat4(1.0) * vec4(inPosition, 1.0)).xyz;
    outNormal = inNormal;
    outUV = inUV;
    outColor = inColor;

    vec3 T = normalize(vec3(mat4(1.0) * vec4(inTangent, 0.0)));
    vec3 N = normalize(vec3(mat4(1.0) * vec4(inNormal, 0.0)));
    vec3 B = cross(N, T);
    outTBN = mat3(T, B, N);

    vec4 pos = U_FrameData.view * vec4(inPosition, 1.0);
    outNormal = mat3(U_FrameData.view) * inNormal;
    vec3 lPos = mat3(U_FrameData.view) * U_FrameData.lightPos.xyz;

    outLightVec = U_FrameData.lightPos.xyz - pos.xyz;
    outViewVec = U_FrameData.viewPos.xyz - pos.xyz;

    outTexIndex = gl_InstanceIndex;
}

