// Linear blend skinning: every vertex is the weighted sum of where each of its
// influences' matrices would put it.

#include "SkinningCommon.hlsli"

struct InfluenceMatrix
{
    row_major float4x4 m;
};

StructuredBuffer<InfluenceMatrix> gPalette : register(t2);

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    const uint index = id.x;
    if (index >= gVertexCount)
        return;

    const RestVertex rest = gRest[index];
    const uint weightBase = index * gInfluenceCount;

    float3 position = float3(0.0f, 0.0f, 0.0f);
    float3 normal = float3(0.0f, 0.0f, 0.0f);
    float total = 0.0f;

    for (uint i = 0; i < gInfluenceCount; ++i)
    {
        const float weight = gWeights[weightBase + i];
        if (weight <= 0.0f)
            continue;

        const float4x4 m = gPalette[i].m;

        position += weight * mul(float4(rest.position, 1.0f), m).xyz;
        normal += weight * mul(float4(rest.normal, 0.0f), m).xyz;
        total += weight;
    }

    // An unweighted vertex would collapse onto the origin, so leave it at rest.
    if (total <= 0.0f)
    {
        position = rest.position;
        normal = rest.normal;
    }

    const float length2 = dot(normal, normal);
    normal = length2 > 1e-12f ? normal * rsqrt(length2) : rest.normal;

    StoreVertex(index, position, normal, rest.texcoord);
}
