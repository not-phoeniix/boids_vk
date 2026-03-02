#include "common_types.slangi"
#include "lighting.slangi"

struct PixelPushConstants {
    [[vk_offset(128)]]
    float3 color;
    float3 ambient;
    float3 camera_pos;
}

[[vk_push_constant]]
PixelPushConstants ppc;

[[vk_binding(0, 1)]]
Sampler2D texture;

[shader("fragment")]
float4 main(PSIn in_data) : SV_Target {
    Light light = {
        LightType::DIRECTIONAL,                 // type
        normalize(float3(1.0f, -3.0f, 1.5f)),   // direction
        0.8f,                                   // intensity
        float3(1.0f, 0.0f, 0.3f),               // color
    };

    SurfaceData surface = {
        in_data.normal,                             // normal
        texture.Sample(in_data.uv).rgb * ppc.color, // surface color
        in_data.world_pos,                          // surface frag pos
        0.9f                                        // roughness
    };

    float3 total_color = float3(0.0f);

    total_color += Lighting::shade_retro(light, surface, ppc.ambient, ppc.camera_pos);

    return float4(total_color, 1.0f);
}
