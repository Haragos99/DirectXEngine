// Linear blend skinning. The result is the mesh's own vertex buffer, so it is
// written through a raw UAV: a structured buffer cannot also be an input
// assembler buffer.

struct RestVertex
{
    float3 position;
    float3 normal;
    float2 texcoord;
};

struct SkinWeight
{
    uint4 joints;
    float4 weights;
};

struct JointMatrix
{
    row_major float4x4 m;
};

StructuredBuffer<RestVertex> gRest : register(t0);
StructuredBuffer<SkinWeight> gWeights : register(t1);
StructuredBuffer<JointMatrix> gPalette : register(t2);
RWByteAddressBuffer gOut : register(u0);

cbuffer SkinConstants : register(b0)
{
    uint gVertexCount;
    uint gJointCount;
    uint gWeightColors;
    uint gPadding;
};

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    const uint index = id.x;
    if (index >= gVertexCount)
        return;

    const RestVertex rest = gRest[index];
    const SkinWeight influence = gWeights[index];

    float3 position = float3(0.0f, 0.0f, 0.0f);
    float3 normal = float3(0.0f, 0.0f, 0.0f);
    float total = 0.0f;
    uint dominantJoint = influence.joints[0];
    float dominantWeight = 0.0f;

    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        const float weight = influence.weights[i];
        if (weight <= 0.0f)
            continue;

        const uint joint = min(influence.joints[i], gJointCount - 1);
        const float4x4 m = gPalette[joint].m;

        position += weight * mul(float4(rest.position, 1.0f), m).xyz;
        normal += weight * mul(float4(rest.normal, 0.0f), m).xyz;
        total += weight;

        if (weight > dominantWeight)
        {
            dominantWeight = weight;
            dominantJoint = joint;
        }
    }

    // An unweighted vertex would collapse onto the origin, so leave it at rest.
    if (total <= 0.0f)
    {
        position = rest.position;
        normal = rest.normal;
    }

    const float length2 = dot(normal, normal);
    normal = length2 > 1e-12f ? normal * rsqrt(length2) : rest.normal;

    // SkinWeightPixelShader reads the influence out of the texcoord.
    const float2 texcoord = (gWeightColors != 0)
        ? float2((float) dominantJoint, dominantWeight)
        : rest.texcoord;

    // Must match sizeof(VertexData): float3 + float3 + float2.
    const uint offset = index * 32u;
    gOut.Store3(offset, asuint(position));
    gOut.Store3(offset + 12u, asuint(normal));
    gOut.Store2(offset + 24u, asuint(texcoord));
}
