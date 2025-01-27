TextureCube cubeMap : register(t0);
SamplerState bilinearSampler : register(s0);

cbuffer TransformBuffer : register(b0)
{
    float4x4 Projection;
    float4x4 View;
    float4x4 World;
}

struct VS_Out
{
    float4 position : SV_POSITION;
    float3 localPosition : POSITION;
};

VS_Out VS_main(float3 Position : POSITION, float3 Normal : NORMAL, float2 TexCoord : TEXCOORD, float4 Tangent : TANGENT)
{
    VS_Out output = (VS_Out) 0;
    
    // create a new identity world matrix that provides no rotation or translation to the skybox
    float4x4 worldMatrix = float4x4(float4(1, 0, 0, 0), float4(0, 1, 0, 0), float4(0, 0, 1, 0), float4(0, 0, 0, 1));
    
    // create a modified view matrix that has no translation
    float4x4 viewMatrix = View;
    viewMatrix._41 = 0.0f;
    viewMatrix._42 = 0.0f;
    viewMatrix._43 = 0.0f;
    
    output.position = mul(mul(mul(float4(Position, 1.0f), worldMatrix), viewMatrix), Projection).xyww;
    output.localPosition = Position;
    
    return output;
}

float4 PS_main(VS_Out input) : SV_TARGET
{
    return cubeMap.Sample(bilinearSampler, input.localPosition);
}