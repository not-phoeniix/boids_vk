#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 world;
    mat4 view;
    mat4 proj;
    // mat4 wit;

    vec3 color;
    vec3 ambient;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outAmbient;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.world * vec4(inPos, 1.0);
    outColor = ubo.color;
    outNormal = normalize(mat3(ubo.world) * inNormal);
    outAmbient = ubo.ambient;
}
