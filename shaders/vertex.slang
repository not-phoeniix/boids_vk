#include "common_types.slangi"

struct CameraPushConstants {
    float4x4 view;
    float4x4 proj;
}

struct ObjectData {
    float4x4 world;
}

[[vk_push_constant]]
CameraPushConstants cpc;

[[vk_binding(0, 0)]]
ConstantBuffer<ObjectData> object_data;

[shader("vertex")]
PSIn main(Vertex vert) {
    float4x4 wvp = mul(cpc.proj, mul(cpc.view, object_data.world));

    PSIn output;
    output.position = mul(wvp, float4(vert.position, 1.0f));
    output.world_pos = mul(float3x3(object_data.world), vert.position);
    output.normal = normalize(mul(float3x3(object_data.world), vert.normal));
    output.uv = vert.uv;

    return output;
}
