#include "CookTorrence.hlsl"
#include "Lighting.hlsl"
//#include "Fog.hlsl"
#define MAX_LIGHTS 10

SamplerState bilinearSampler : register(s0);
Texture2DArray material1 : register(t0);
Texture2DArray material2 : register(t1);

cbuffer TransformBuffer : register(b0)
{
    float4x4 Projection;
    float4x4 View;
    float4x4 World;
}

cbuffer GlobalBuffer : register(b1)
{
    float4 AmbientLight;
    float3 CameraPosition;
    float _padding;
}

cbuffer LightBuffer : register(b2)
{
    Light Lights[MAX_LIGHTS];
    int LightCount;
}

//cbuffer FogBuffer : register(b3)
//{
//    float3 FogColor;
//    float FogDensity;
//}

struct VS_Out
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION0;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
};

VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, float4 Tangent : TANGENT)
{
    VS_Out output = (VS_Out) 0;
    
    float3x3 rotation = (float3x3) World;
    float4 position = float4(Position, 1.0f);
    
    output.position = mul(mul(mul(position, World), View), Projection);
    
    output.worldPosition = mul(position, World).xyz;
    output.normal = normalize(mul(Normal, rotation));
    
    output.texCoord = TexCoord;
    
    output.tangent = normalize(mul(float4(Tangent.xyz, 0.0f), World).xyz);
    output.bitangent = Tangent.w * cross(output.normal, output.tangent);
    
    return output;
}


float4 PS_main(VS_Out input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float offset = dot(float3(0.0f, 1.0f, 0.0f), normal);

    float3x3 TBN = float3x3(input.tangent, input.bitangent, normal);
    float3 viewDir = normalize(CameraPosition - input.worldPosition);

    // Calculate the final color for material1 (grass)
    float3 sampledNormal1 = normalize(2.0f * material1.Sample(bilinearSampler, float3(input.texCoord, TEXTURETYPE_NORMAL)).rgb - 1.0f);
    normal = normalize(mul(sampledNormal1, TBN));
    
    float3 finalColor1 = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < LightCount; i++)
    {
        Light lightData = Lights[i];
        float3 lightDir;
        
        float3 lightContribution = CalculateLightContribution(lightData, input.worldPosition, normal, viewDir, lightDir);
        float3 reflectance = CalculateReflectanceColor(bilinearSampler, material1, AmbientLight.rgb, input.texCoord, viewDir, lightDir, normal);
        
        finalColor1 += lightContribution * reflectance;
    }

    // Calculate the final color for material2 (rock)
    sampledNormal1 = normalize(2.0f * material2.Sample(bilinearSampler, float3(input.texCoord, TEXTURETYPE_NORMAL)).rgb - 1.0f);
    normal = normalize(mul(sampledNormal1, TBN));
    
    float3 finalColor2 = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < LightCount; i++)
    {
        Light lightData = Lights[i];
        float3 lightDir;
        
        float3 lightContribution = CalculateLightContribution(lightData, input.worldPosition, normal, viewDir, lightDir);
        float3 reflectance = CalculateReflectanceColor(bilinearSampler, material2, AmbientLight.rgb, input.texCoord, viewDir, lightDir, normal);
        
        finalColor2 += lightContribution * reflectance;
    }
    
    // Blend the two final colors based on the offset
    float blendFactor = smoothstep(0.7f, 0.8f, offset);
    float3 blendedColor = lerp(finalColor2, finalColor1, blendFactor);

    return float4(blendedColor, 1.0f);
    //float distance = length(CameraPosition - input.worldPosition);
    //return float4(CalculateFogEffect(blendedColor, distance, FogColor, FogDensity), 1.0f);
}