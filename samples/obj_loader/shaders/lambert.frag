#version 450

layout(location = 0) in vec3 fNormal;
layout(location = 1) in vec2 fUv;
layout(location = 2) in vec3 fLightDir;

layout(location = 0) out vec4 oColor;

void main()
{
    float NdotL = dot(fNormal, fLightDir);
    float attenuation = max(NdotL, 0);
    oColor = vec4(vec3(attenuation), 1);
}