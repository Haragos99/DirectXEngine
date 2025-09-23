// Texture sampler
SamplerState gTextureSampler : register(s0);

// Textures
Texture2D gTextureDiffuse : register(t0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXTURE;
};

float4 PSMain(VS_OUT input) : SV_TARGET
{
    float4 diffuse_texture = gTextureDiffuse.Sample(gTextureSampler, input.tex);
    return diffuse_texture;
}
