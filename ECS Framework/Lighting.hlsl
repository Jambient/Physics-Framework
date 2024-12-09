// LIGHT TYPE ENUM
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

struct Light
{
    int Type;
    float3 Position;
    float3 Direction;
    float Range;
    float3 Color;
    float Intensity;
    float SpotAngle;
    float3 _padding;
};

float3 CalculateLightContribution(Light light, float3 worldPos, float3 normal, float3 viewDir, out float3 lightDir)
{
    float3 contribution = float3(0.0f, 0.0f, 0.0f);
    
    if (light.Type == LIGHT_TYPE_DIRECTIONAL)
    {
        lightDir = normalize(-light.Direction);
        float NdotL = saturate(dot(normal, lightDir));
        contribution = light.Color * NdotL;
    }
    else if (light.Type == LIGHT_TYPE_POINT)
    {
        lightDir = normalize(light.Position - worldPos);
        float distance = length(light.Position - worldPos);
        float attenuation = saturate(1.0f - (distance / light.Range));
        float NdotL = saturate(dot(normal, lightDir));
        contribution = light.Color * light.Intensity * NdotL * attenuation;
    }
    else if (light.Type == LIGHT_TYPE_SPOT)
    {
        lightDir = normalize(light.Position - worldPos);
        float distance = length(light.Position - worldPos);
        float attenuation = saturate(1.0f - (distance / light.Range));
        
        float spotEffect = dot(normalize(-light.Direction), lightDir);
        float spotFactor = smoothstep(cos(light.SpotAngle), 1.0, spotEffect);
        
        float NdotL = saturate(dot(normal, lightDir));
        contribution = light.Color * light.Intensity * NdotL * attenuation * spotFactor;
    }
    
    return contribution;
}