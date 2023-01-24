#version 450 core
#pragma shader_stage(fragment)

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inViewVec;
layout(location = 4) in vec3 inLightVec;
layout(location = 5) in vec3 inFragPos;
layout(location = 6) in flat int inTexIndex;
layout(location = 7) in mat3 inTBN;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D[32] texSamplers;
layout(set = 1, binding = 1) uniform samplerCube[8] cubeTexSamplers;

void main() {
    vec4 color = texture(texSamplers[inTexIndex], inUV) * vec4(inColor, 1.0);

    vec3 N = texture(texSamplers[inTexIndex-2], inUV).rgb;
    N = N * 2.0 - 1.0;
    N = normalize(inTBN * N);

    vec3 L = normalize(inLightVec - inFragPos);
    vec3 V = normalize(inViewVec - inFragPos);
    vec3 R = reflect(-L, N);

    vec3 ambient = color.rgb * 0.1;

    vec3 diffuse = max(dot(L, N), 0.0) * vec3(1.0, 1.0, 1.0);

    vec3 specular = pow(max(dot(V, R), 0.0), 32.0) * vec3(0.75);
    
    outColor = vec4((ambient + diffuse + specular) * color.rgb, 1.0);
}
