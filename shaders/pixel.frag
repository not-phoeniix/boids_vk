#version 450

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inAmbient;

layout(location = 0) out vec4 outColor;

vec3 lambert_diffuse(vec3 lightColor, vec3 lightDir, float lightIntensity, vec3 surfaceColor, vec3 surfaceNormal) {
    float scalar = max(dot(-lightDir, surfaceNormal), 0.0);
    return surfaceColor * scalar * lightIntensity * lightColor;
}

void main() {
    vec3 totalColor = vec3(0.0);

    totalColor += inAmbient;

    totalColor += lambert_diffuse(
        vec3(0.7, 0.7, 1.0), 
        normalize(vec3(1.0, -3.0, -1.5)), 
        0.8, 
        inColor,
        inNormal
    );

    outColor = vec4(totalColor, 1.0);
}
