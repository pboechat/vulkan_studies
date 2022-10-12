#version 450

layout(set = 0, binding = 0) uniform uSceneConstants {
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUv;

layout(location = 0) out vec3 fNormal;
layout(location = 1) out vec2 fUv;
layout(location = 2) out vec3 fLightDir;

void main()
{
    vec4 worldPosition = view * vec4(vPosition, 1.0);
    gl_Position = projection * worldPosition;
    fNormal = (inverse(view) * vec4(vNormal, 0.0)).xyz;
    fUv = vUv;
    fLightDir = -normalize(worldPosition.xyz);
}