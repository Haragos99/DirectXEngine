// Dual quaternion skinning. Blending rigid transforms instead of matrices keeps
// the volume of a twisted joint, which is where linear blending collapses.

#include "SkinningCommon.hlsli"

// Rigid only: any scale in the palette was dropped on the CPU side.
struct DualQuat
{
    float4 real;
    float4 dual;
};

StructuredBuffer<DualQuat> gPalette : register(t2);

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
    const uint weightBase = index * gInfluenceCount;

    float4 real = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 dual = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float total = 0.0f;

    // q and -q are the same rotation, so every quaternion has to be pulled into
    // the same hemisphere as the first contributing one or the blend takes the
    // long way round.
    float4 pivot = float4(0.0f, 0.0f, 0.0f, 1.0f);
    bool havePivot = false;

    for (uint i = 0; i < gInfluenceCount; ++i)
    {
        const float weight = gWeights[weightBase + i];
        if (weight <= 0.0f)
            continue;

        const DualQuat dq = gPalette[i];
        if (!havePivot)
        {
            pivot = dq.real;
            havePivot = true;
        }

        const float signedWeight = (dot(pivot, dq.real) < 0.0f) ? -weight : weight;

        real += dq.real * signedWeight;
        dual += dq.dual * signedWeight;
        total += weight;
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

    StoreVertex(index, position, normal, rest.texcoord);
}
