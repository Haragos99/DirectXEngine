// Texture sampler
SamplerState gTextureSampler : register(s0);

// Textures  
TextureCube gTextureDiffuse : register(t0);

struct PS_IN
{
    float4 positionClipSpace : SV_POSITION;
    float3 position : POSITION;
};

float4 PSMain(PS_IN input) : SV_TARGET
{

    float4 envColor =  gTextureDiffuse.Sample(gTextureSampler, input.position);

    return envColor;
}
