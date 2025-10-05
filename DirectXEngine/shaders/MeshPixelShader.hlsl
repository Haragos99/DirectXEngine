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
    float3 worldPos : TEXCOORD0;
    float2 tex : TEXTURE;
};

float4 PSMain(VS_OUT input) : SV_TARGET
{
    float3 N = normalize(input.normal);
  //  float3 L = normalize(-lightColor); // direction *to* light
    float3 L = normalize(float3(0.0f, 15.0f, -5.0f) - input.worldPos);
    // View direction (assuming camera at origin for simplicity)
    float3 V = normalize(-input.worldPos);

    // Reflection vector
    float3 R = reflect(-L, N);

    // ---------------------------
    // Lighting components
    // ---------------------------

    // Ambient
    float ambientStrength = 0.2f;
    float3 ambient = ambientStrength * lightColor.rgb;

    // Diffuse
    float diff = saturate(dot(N, L));
    float3 diffuse = diff * lightColor.rgb;

    // Specular (Phong)
    float specularStrength = 0.4f;
    float shininess = 32.0f;
    float spec = pow(saturate(dot(R, V)), shininess);
    float3 specular = specularStrength * spec * lightColor.rgb;

    // Combine lighting
    float3 lightTerm = (ambient + diffuse + specular) * 0.5f;

    // Sample texture
    float4 texColor = gTextureDiffuse.Sample(gTextureSampler, input.tex);

    // Final color
    float3 finalColor = texColor.rgb * lightTerm;

    return float4(finalColor, texColor.a);
}
