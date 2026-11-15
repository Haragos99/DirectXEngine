// The result is the mesh's own vertex buffer, so it is written through a raw
// UAV: a structured buffer cannot also be an input assembler buffer.

struct RestVertex
{
    float3 position;
    float3 normal;
    float2 texcoord;
};

StructuredBuffer<RestVertex> gRest : register(t0);
StructuredBuffer<float> gWeights : register(t1);
StructuredBuffer<float3> gInfluenceColors : register(t3);
RWByteAddressBuffer gOut : register(u0);

// Mirrors SkinConstants in the C++ deformers.
cbuffer SkinConstants : register(b0)
{
    uint gVertexCount;
    uint gInfluenceCount;
    uint gViewMode;
    int gSelectedInfluence;
};

static const uint kViewOff = 0;
static const uint kViewBlended = 1;
static const uint kViewSingle = 2;

float3 HeatRamp(float t)
{
    t = saturate(t);
    return saturate(float3(1.5f - abs(4.0f * t - 3.0f),
                           1.5f - abs(4.0f * t - 2.0f),
                           1.5f - abs(4.0f * t - 1.0f)));
}

float3 BlendedInfluenceColor(uint weightBase)
{
    float3 color = float3(0.0f, 0.0f, 0.0f);
    float total = 0.0f;
    for (uint i = 0; i < gInfluenceCount; ++i)
    {
        const float weight = gWeights[weightBase + i];
        if (weight <= 0.0f)
        {
            continue;
        }
        color += weight * gInfluenceColors[i];
        total += weight;
    }
    return total > 0.0f ? color / total : float3(0.5f, 0.5f, 0.5f);
}

void StoreVertex(uint index, float3 position, float3 normal, float2 texcoord)
{
    const uint weightBase = index * gInfluenceCount;

    float3 slot = normal;
    if (gViewMode == kViewBlended)
    {
        slot = BlendedInfluenceColor(weightBase);
    }
    else if (gViewMode == kViewSingle)
    {
        const float weight = (gSelectedInfluence >= 0 && (uint) gSelectedInfluence < gInfluenceCount)
            ? gWeights[weightBase + (uint) gSelectedInfluence]
            : 0.0f;
        slot = HeatRamp(weight);
    }

    // Must match sizeof(VertexData): float3 + float3 + float2.
    const uint offset = index * 32u;
    gOut.Store3(offset, asuint(position));
    gOut.Store3(offset + 12u, asuint(slot));
    gOut.Store2(offset + 24u, asuint(texcoord));
}
