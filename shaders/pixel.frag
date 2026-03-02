#version 450

layout(push_constant, std430) uniform PushConstants {
    layout(offset=128) vec3 color;
    vec3 ambient;
};
layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 1) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec4 outColor;

vec3 lambert_diffuse(vec3 lightColor, vec3 lightDir, float lightIntensity, vec3 surfaceColor, vec3 surfaceNormal) {
    float scalar = max(dot(-lightDir, surfaceNormal), 0.0);
    return surfaceColor * scalar * lightIntensity * lightColor;
}

void main() {
    vec3 totalColor = vec3(0.0, 0.0, 0.0);

    vec3 surfaceColor = texture(texSampler, inUV).rgb * color;

    totalColor += ambient;

    totalColor += lambert_diffuse(
        vec3(0.7, 0.7, 1.0), 
        normalize(vec3(1.0, -3.0, 1.5)), 
        0.8, 
        surfaceColor,
        inNormal
    );

    outColor = vec4(totalColor, 1.0);
}
