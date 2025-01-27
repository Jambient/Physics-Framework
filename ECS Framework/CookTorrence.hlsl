#define PI 3.14159265359f

#define TEXTURETYPE_ALBEDO 0
#define TEXTURETYPE_NORMAL 1
#define TEXTURETYPE_AMBIENT 2
#define TEXTURETYPE_ROUGHNESS 3
#define TEXTURETYPE_METALLIC 4
#define TEXTURETYPE_HEIGHT 5

float GGXNormalDistribution(float roughness, float3 normal, float3 halfVec)
{
    float roughnessSquared = roughness * roughness;
    float NdotH = max(dot(normal, halfVec), 0.0f);
    float NdotHSquared = NdotH * NdotH;
    float denominator = NdotHSquared * (roughnessSquared - 1.0f) + 1.0f;
    denominator *= denominator;
    return roughnessSquared / max(PI * denominator, 0.001f);
}

float3 SchlickFresnel(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

}

float CookTorrenceAttenuation(float3 normal, float3 lightVec, float3 viewVec, float3 halfVec)
{
    float NdotV = max(dot(normal, viewVec), 0.0f);
    float NdotL = max(dot(normal, lightVec), 0.0f);
    float NdotH = max(dot(normal, halfVec), 0.0f);
    float HdotV = max(dot(halfVec, viewVec), 0.001f);

    float factor1 = (2.0f * NdotH * NdotV) / HdotV;
    float factor2 = (2.0f * NdotH * NdotL) / HdotV;

    return min(1.0f, min(factor1, factor2));
}

float3 lambertianDiffuse(float3 albedo)
{
    return albedo / PI;
}

float3 CalculateReflectanceColor(SamplerState textureSampler, Texture2DArray material, float3 ambientLight, float2 texCoord, float3 viewVec, float3 lightVec, float3 normal)
{
    float3 halfVec = normalize(lightVec + viewVec);
    
    // sample textures
    float3 albedo = material.Sample(textureSampler, float3(texCoord, TEXTURETYPE_ALBEDO)).rgb;
    float ambientFactor = material.Sample(textureSampler, float3(texCoord, TEXTURETYPE_AMBIENT)).r;
    float roughness = material.Sample(textureSampler, float3(texCoord, TEXTURETYPE_ROUGHNESS)).r;
    float metallic = material.Sample(textureSampler, float3(texCoord, TEXTURETYPE_METALLIC)).r;
    
    // calculate F0
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    
    // base ambient colour
    float3 ambient = albedo * ambientLight * ambientFactor;
    
    // calculate 
    float cosTheta = max(dot(viewVec, halfVec), 0.0f);
    
    // brdf numerator equations
    float density = GGXNormalDistribution(pow(roughness, 2), normal, halfVec);
    float3 fresnel = SchlickFresnel(cosTheta, F0);
    float attenuation = CookTorrenceAttenuation(normal, lightVec, viewVec, halfVec);
    
    // calculate brdf
    float NdotL = max(dot(normal, lightVec), 0.0);
    float NdotV = max(dot(normal, viewVec), 0.0);
    float denominator = 4 * NdotL * NdotV;
    float3 specular = (density * fresnel * attenuation) / max(denominator, 0.001f);
    
    //float3 diffuse = lambertianDiffuse(albedo) * (1.0 - fresnel);
    float3 diffuse = (1.0 - metallic) * lambertianDiffuse(albedo) * (1.0 - fresnel);
    
    float3 reflectedLight = ambient + (diffuse + specular) * float3(1.0f, 1.0f, 1.0f) * NdotL;
    
    return reflectedLight;
}