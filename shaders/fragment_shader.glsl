#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in float StarBrightness;

uniform vec3 lightPos;        // Sun position (0,0,0)
uniform vec3 lightColor;      // Sun light color
uniform vec3 cameraPos;       // Camera position
uniform float ambientStrength;
uniform float specularStrength;
uniform int shininess;
uniform bool isEmissive;      // True for Sun, false for planets/moons
uniform bool isPoint;         // True for stars
uniform float time;           // For star twinkling

// PBR texture samplers
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

// Fallback for legacy texture
uniform sampler2D texture1;

// Helper: get normal from normal map
vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, TexCoord).xyz * 2.0 - 1.0;
    vec3 Q1 = dFdx(FragPos);
    vec3 Q2 = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoord);
    vec2 st2 = dFdy(TexCoord);
    vec3 N = normalize(Normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}

float starTwinkle(vec3 pos, float t) {
    float twinkleSpeed = 0.5 + mod(pos.x + pos.y + pos.z, 1.0);
    float twinklePhase = mod(pos.x * 7.0 + pos.y * 13.0 + pos.z * 17.0, 6.28);
    float twinkle = 0.7 + 0.3 * sin(t * twinkleSpeed + twinklePhase);
    return twinkle;
}

// Cook-Torrance BRDF
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    // For point rendering (stars)
    if (isPoint) {
        float twinkle = starTwinkle(FragPos, time);
        FragColor = vec4(vec3(StarBrightness * twinkle), 1.0);
        return;
    }

    // Sample PBR textures
    vec3 albedo = texture(albedoMap, TexCoord).rgb;
    float metallic = texture(metallicMap, TexCoord).r;
    float roughness = texture(roughnessMap, TexCoord).r;
    float ao = texture(aoMap, TexCoord).r;
    vec3 norm = getNormalFromMap();

    // Fallback for legacy texture
    if (albedo == vec3(0.0)) {
        albedo = texture(texture1, TexCoord).rgb;
    }

    // For emissive objects (Sun), use albedo directly
    if (isEmissive) {
        FragColor = vec4(albedo, 1.0);
        return;
    }

    // === PBR LIGHTING ===
    vec3 N = normalize(norm);
    vec3 V = normalize(cameraPos - FragPos);
    vec3 L = normalize(lightPos - FragPos);
    vec3 H = normalize(V + L);

    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.00001 * distance + 0.0000001 * distance * distance);
    vec3 radiance = lightColor * attenuation;

    // Cook-Torrance BRDF
    float NDF = pow(roughness, 4.0) / (3.141592 * pow((dot(N, H) * dot(N, H)) * (roughness * roughness - 1.0) + 1.0, 2.0));
    float G = min(1.0, min(2.0 * dot(N, H) * dot(N, V) / dot(V, H), 2.0 * dot(N, H) * dot(N, L) / dot(V, H)));
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 ambient = ambientStrength * albedo * ao;
    vec3 color = ambient + (kD * albedo / 3.141592 + specular) * radiance * NdotL;

    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    FragColor = vec4(color, 1.0);
}