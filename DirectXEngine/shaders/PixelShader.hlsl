// Texture sampler
SamplerState gTextureSampler : register(s0);

// Textures
Texture2D gTextureDiffuse : register(t0);


// Light parameters
cbuffer LightBuffer : register(b1)
{
    float3 lightDirection; // normalized direction to the light
    float padding; // align to 16 bytes
    float4 lightColor; // RGBA
}


struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXTURE;
};

float4 PSMain(VS_OUT input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightColor); // direction *to* light

    // Diffuse factor (clamped to [0,1])
    float NdotL = saturate(dot(N, L));
    
    float4 texColor = gTextureDiffuse.Sample(gTextureSampler, input.tex);
    
    float ambientStrength = 0.2f;
    float4 ambient = lightColor * ambientStrength;
    // Apply lighting
    float4 litColor = texColor * (ambient + lightColor * NdotL);
    
    return litColor;
}
