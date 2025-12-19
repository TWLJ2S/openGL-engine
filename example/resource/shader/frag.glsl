#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;

uniform sampler2D baseColor;
uniform sampler2D normal;
uniform sampler2D metallicRoughness;
uniform sampler2D occlusion;
uniform sampler2D emissive;

uniform vec3 camPos;

#define MAX_LIGHTS 8
uniform int numLights;
uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normal, TexCoords).xyz * 2.0 - 1.0;
    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (3.14159 * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;  // UE4's k formulation
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // Sample material textures
    vec4 baseSample = texture(baseColor, TexCoords);
    vec3 albedo = pow(baseSample.rgb, vec3(2.2)); // gamma → linear
    float alpha = baseSample.a;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(camPos - FragPos);
    vec3 emissiveColor = texture(emissive, TexCoords).rgb;
    float ao = texture(occlusion, TexCoords).r;

    vec3 mrSample = texture(metallicRoughness, TexCoords).rgb;
    float metallic  = mrSample.b;
    float roughness = clamp(mrSample.g, 0.05, 1.0); // Avoid 0 for stability

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Lighting accumulation
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < numLights; ++i)
    {
        vec3 L = normalize(lightPos[i] - FragPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPos[i] - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColor[i] * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / 3.14159 + specular) * radiance * NdotL;
    }

    // Simple ambient term modulated by AO
    vec3 ambient = ao * albedo * 0.03;

    vec3 color = ambient + Lo + emissiveColor;

    // Gamma correction (linear → sRGB)
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, alpha);
}
