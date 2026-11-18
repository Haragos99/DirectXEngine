// Texture sampler
SamplerState gTextureSampler : register(s0);

// Textures
Texture2D gTextureDiffuse : register(t0);


Texture2D gNormalMap : register(t1);
SamplerState gSampler : register(s1);

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
    float3 worldPos : TEXCOORD0;
    float2 tex : TEXTURE;
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
};

float4 PSMain(VS_OUT input) : SV_TARGET
{
    float3 normalFromMap = gNormalMap.Sample(gSampler, input.tex).rgb;
    normalFromMap.xy *= 1.0F; // Scale normal map influence
    normalFromMap = normalize(normalFromMap * 2.0f - 1.0f);
    // Build TBN matrix and transform to world space
    float3x3 TBN = float3x3(normalize(input.tangent),
                            normalize(input.bitangent),
                            normalize(input.normal));

    
    float3 worldNormal = normalize(mul(normalFromMap, TBN));
    
    
    float4 texColor = gTextureDiffuse.Sample(gTextureSampler, input.tex);
    
    
    float NdotL = saturate(dot(worldNormal, lightDirection));
    
    
    float ambientStrength = 0.2f;
    float4 ambient = lightColor * ambientStrength;
    // Apply lighting
    float4 litColor = texColor * (ambient + lightColor * NdotL);

    float3 diffuse = texColor * lightColor * NdotL;
    return float4(diffuse, 1.0);
}
