TextureCube EnvMap : register(t0);
SamplerState EnvSampler : register(s0);

cbuffer CameraCB : register(b0)
{
    matrix invViewProj;
    float3 camPos;
    float pad;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0; // must match vertex shader output
};

float4 PSMain(PSInput input) : SV_TARGET
{
    // Convert to NDC
    float2 ndc = input.uv * 2.0f - 1.0f;

    // Homogeneous clip-space position
    float4 clip = float4(ndc, 0.0f, 1.0f);

    // Reconstruct world position
    float4 worldPos = mul(clip, invViewProj);
    worldPos /= worldPos.w;
    float3 dir = normalize(worldPos.xyz - camPos);

    // Sphere intersection
    float3 center = float3(0, 0, 0);
    float radius = 1.0;

    float3 oc = camPos - center;
    float b = dot(oc, dir);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;

    if (h < 0.0f)
    {
        // Miss  sample environment map
        return EnvMap.Sample(EnvSampler, dir);
    }

    // Hit point
    h = sqrt(h);
    float t = -b - h;
    if (t < 0)
        t = -b + h;

    float3 p = camPos + dir * t;
    float3 n = normalize(p - center);

    // Reflection ray
    float3 refl = reflect(dir, n);

    return float4(1,0,0,1);
}
