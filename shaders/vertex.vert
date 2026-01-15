#version 450

layout(push_constant, std430) uniform PushConstants {
    mat4 view;
    mat4 proj;
};
layout(binding = 0) uniform UniformBufferObject {
    mat4 world;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 1) out vec3 outNormal;
layout(location = 3) out vec2 outUV;

void main() {
    gl_Position = proj * view * ubo.world * vec4(inPos, 1.0);
    outNormal = normalize(mat3(ubo.world) * inNormal);
    outUV = inUV;
}
