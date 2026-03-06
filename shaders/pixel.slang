#include "common_types.slangi"
#include "lighting.slangi"

#define MAX_LIGHTS 256

struct PixelPushConstants {
    [[vk_offset(128)]]
    float3 color;
    float3 ambient;
    float3 camera_pos;
}

struct LightData {
    Light lights[MAX_LIGHTS];
    uint light_count;
}

[[vk_push_constant]]
PixelPushConstants ppc;

[[vk_binding(1, 0)]]
ConstantBuffer<LightData> light_data;

[shader("fragment")]
float4 main(PSIn in_data) : SV_Target {
    SurfaceData surface = {};
    surface.normal = in_data.normal;
    surface.color = ppc.color;
    surface.position = in_data.world_pos;
    surface.roughness = 0.2f;

    float3 total_color = float3(0.0f);
    total_color += ppc.ambient * surface.color;

    for (uint i = 0; i < light_data.light_count; i++) {
        total_color += Lighting::shade_retro(
            light_data.lights[i],
            surface,
            ppc.camera_pos
        );
    }

    return float4(total_color, 1.0f);
}
