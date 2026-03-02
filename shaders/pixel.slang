#include "common_types.slangi"
#include "lighting.slangi"

struct PixelPushConstants {
    [[vk_offset(128)]]
    float3 color;
    float3 ambient;
}

[[vk_push_constant]]
PixelPushConstants ppc;

[[vk_binding(0, 1)]]
Sampler2D texture;

[shader("fragment")]
float4 main(PSIn in_data) : SV_Target {
    Light light = {
        float3(0.7f, 0.7f, 1.0f),
        0.8f,
        normalize(float3(1.0f, -3.0f, 1.5f))
    };

    float3 total_color = {0.0f, 0.0f, 0.0f};

    float3 surface_color = texture.Sample(in_data.uv).rgb * ppc.color;

    total_color += ppc.ambient;
    total_color += Lighting::lambert_diffuse(light, surface_color, in_data.normal);

    return float4(total_color, 1.0f);
}
