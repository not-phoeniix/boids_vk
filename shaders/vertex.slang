#include "common_types.slangi"

struct CameraPushConstants {
    float4x4 view;
    float4x4 proj;
}

struct InstanceData {
    float4x4 world;
}

[[vk_push_constant]]
CameraPushConstants cpc;

[[vk_binding(0, 0)]]
StructuredBuffer<InstanceData> instance_datas;

[shader("vertex")]
PSIn main(Vertex vert, uint iid: SV_VulkanInstanceID) {
    float4x4 world_mat = instance_datas[iid].world;
    float4x4 wvp = mul(cpc.proj, mul(cpc.view, world_mat));

    PSIn output;
    output.position = mul(wvp, float4(vert.position, 1.0f));
    output.world_pos = mul(world_mat, float4(vert.position, 1.0f)).xyz;
    output.normal = normalize(mul(float3x3(world_mat), vert.normal));
    output.uv = vert.uv;

    return output;
}
