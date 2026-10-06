// Dual quaternion skinning. Blending rigid transforms instead of matrices keeps
// the volume of a twisted joint, which is where linear blending collapses.
//
// The result is the mesh's own vertex buffer, so it is written through a raw
// UAV: a structured buffer cannot also be an input assembler buffer.

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

// Rigid only: any scale in the palette was dropped on the CPU side.
struct DualQuat
{
    float4 real;
    float4 dual;
};

StructuredBuffer<RestVertex> gRest : register(t0);
StructuredBuffer<SkinWeight> gWeights : register(t1);
StructuredBuffer<DualQuat> gPalette : register(t2);
RWByteAddressBuffer gOut : register(u0);

cbuffer SkinConstants : register(b0)
{
    uint gVertexCount;
    uint gJointCount;
    uint gWeightColors;
    uint gPadding;
};

float3 RotateByQuaternion(float4 q, float3 v)
{
    return v + 2.0f * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    const uint index = id.x;
    if (index >= gVertexCount)
        return;

    const RestVertex rest = gRest[index];
    const SkinWeight influence = gWeights[index];

    // q and -q are the same rotation, so every quaternion has to be pulled into
    // the same hemisphere as the first one or the blend takes the long way round.
    const float4 pivot = gPalette[min(influence.joints[0], gJointCount - 1)].real;

    float4 real = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 dual = float4(0.0f, 0.0f, 0.0f, 0.0f);
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
        const DualQuat dq = gPalette[joint];
        const float signedWeight = (dot(pivot, dq.real) < 0.0f) ? -weight : weight;

        real += dq.real * signedWeight;
        dual += dq.dual * signedWeight;
        total += weight;

        if (weight > dominantWeight)
        {
            dominantWeight = weight;
            dominantJoint = joint;
        }
    }

    float3 position = rest.position;
    float3 normal = rest.normal;

    // An unweighted vertex would collapse onto the origin, so leave it at rest.
    const float realLength = length(real);
    if (total > 0.0f && realLength > 1e-8f)
    {
        real /= realLength;
        dual /= realLength;

        const float3 translation =
            2.0f * (real.w * dual.xyz - dual.w * real.xyz + cross(real.xyz, dual.xyz));

        position = RotateByQuaternion(real, rest.position) + translation;
        normal = RotateByQuaternion(real, rest.normal);
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
