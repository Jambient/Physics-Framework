cbuffer TransformBuffer : register(b0)
{
    float4x4 Projection;
    float4x4 View;
    float4x4 World;
}

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
    return float4(1.0f, 0.3f, 0.3f, 1.0f);
}