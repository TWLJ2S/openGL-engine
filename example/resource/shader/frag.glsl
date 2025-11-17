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

// Get normal from normal map using TBN
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normal, TexCoords).xyz * 2.0 - 1.0;
    return normalize(TBN * tangentNormal);
}

void main()
{
    vec3 albedo = pow(texture(baseColor, TexCoords).rgb, vec3(2.2));
    float metallic  = texture(metallicRoughness, TexCoords).b;
    float roughness = texture(metallicRoughness, TexCoords).g;
    float ao        = texture(occlusion, TexCoords).r;
    vec3 emissive   = texture(emissive, TexCoords).rgb;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(camPos - FragPos);

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < numLights; i++) {
        vec3 L = normalize(lightPos[i] - FragPos);
        vec3 H = normalize(V + L);

        float distance = length(lightPos[i] - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColor[i] * attenuation;

        float NDF = pow(max(dot(N, H), 0.0), 2.0) * (roughness * roughness);
        float G = max(dot(N, V), 0.0) * max(dot(N, L), 0.0);

        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / 3.14159 + specular) * radiance * NdotL;
    }

    vec3 ambient = ao * albedo * 0.03;
    vec3 color = ambient + Lo + emissive;
    color = pow(color, vec3(1.0/2.2));

    float alpha = texture(baseColor, TexCoords).a;
    FragColor = vec4(color, alpha);
}
