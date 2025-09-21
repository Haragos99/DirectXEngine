TextureCube environmentMap : register(t0);
SamplerState envSampler : register(s0);

cbuffer CameraData : register(b1)
{
    float3 cameraPos;
};

struct PS_IN
{
    float3 worldPos : TEXCOORD0;
    float3 worldNormal : NORMAL;
};

float4 PSMain(PS_IN input) : SV_TARGET
{
    float3 I = normalize(input.worldPos - cameraPos);
    float3 R = reflect(I, normalize(input.worldNormal));

    float4 envColor = environmentMap.Sample(envSampler, R);

    return envColor;
}
