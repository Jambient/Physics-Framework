#include "Lighting.hlsl"

#define MAX_LIGHTS 10

SamplerState bilinearSampler : register(s0);

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

struct VS_Out
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPosition : POSITION0;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
    float3 color : COLOR;
};

VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, float4 Tangent : TANGENT, float3 InstancePos : INSTANCE_POSITION, float3 InstanceScale : INSTANCE_SCALE, float3 InstanceColor: INSTANCE_COLOR)
{   
    VS_Out output = (VS_Out)0;
    
    float3x3 rotation = (float3x3) World;
    float3 worldPosition = Position * InstanceScale + InstancePos;
    
    output.position = mul(mul(float4(worldPosition, 1.0f), View), Projection);
    
    output.worldPosition = worldPosition;
    output.normal = normalize(mul(Normal, rotation));
    
    output.texCoord = TexCoord;
    
    output.tangent = normalize(mul(float4(Tangent.xyz, 0.0f), World).xyz);
    output.bitangent = Tangent.w * cross(output.normal, output.tangent);
    
    output.color = InstanceColor;
    
    return output;
}

float4 PS_main(VS_Out input) : SV_TARGET
{   
    float3 normal = normalize(input.normal);
    
    float3x3 TBN = float3x3(input.tangent, input.bitangent, normal);
    float3 viewDir = normalize(CameraPosition - input.worldPosition);
    
    float3 finalColor = input.color;
    
    for (int i = 0; i < LightCount; i++)
    {
        Light lightData = Lights[i];
        float3 lightDir;
        
        float3 lightContribution = CalculateLightContribution(lightData, input.worldPosition, normal, viewDir, lightDir);
        
        finalColor += lightContribution;
    }
    
    return float4(finalColor, 1.0f);

}