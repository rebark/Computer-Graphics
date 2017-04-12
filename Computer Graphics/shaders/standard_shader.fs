#version 330 core

struct PointLight {
	vec3 position;
	vec3 color;
	float intensity;

	float constant;
	float linear;
	float quadratic;
};

struct SpotLight {
	 vec3 position;
	 vec3 direction;
	 vec3 color;
	 float intensity;

	 float cutOff;
	 float outerCutOff;
	 float constant;
	 float linear;
	 float quadratic;
};

out vec4 color;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
	mat3 TBN;
} fs_in;


//view and directional light
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightInt;

//textures
uniform sampler2D shadowMap;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

uniform bool hasNormalMap;

//lights
uniform PointLight pointLight;
uniform SpotLight spotLight;

//functions to compute light components
vec3 ComputePoint(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objectColor, vec3 specMap);
vec3 ComputeSpot(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objectColor, vec3 specMap);

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // Calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightDir = normalize(lightPos);
    float bias = 0.0005;
    // Check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}


void main()
{
	//fixed properties
	vec3 normal = normalize(fs_in.Normal);

	//apply normal mapping
	if(hasNormalMap)
	{
		normal = vec3(texture(texture_normal1, fs_in.TexCoords));
		normal = normalize(normal * 2.0 - 1.0);   
		normal = normalize(fs_in.TBN * normal);
	}	

	vec3 lightDir = normalize(lightPos);
	vec3 objectColor =  vec3(texture(texture_diffuse1, fs_in.TexCoords));

	//Ambient
	vec3 ambient = 0.2 * objectColor * lightColor;

	//Diffuse
	float diff = max(dot(normal, lightDir),0.0);
	vec3 diffuse = diff * objectColor * lightColor * lightInt;

	//Specular
	vec3 viewDir = normalize(viewPos- fs_in.FragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
	vec3 specMap = vec3(texture(texture_specular1, fs_in.TexCoords));  
	vec3 specular = spec * specMap * lightColor * lightInt;

	//shadows
	float shadow = ShadowCalculation(fs_in.FragPosLightSpace);
	shadow = min(shadow, 0.75); // reduce shadow strength a little: allow some diffuse/specular light in shadowed regions
	vec3 light = (ambient + (1.0 - shadow) * (diffuse + specular));

	if(pointLight.intensity > 0)
		light += ComputePoint(pointLight, normal, fs_in.FragPos, viewDir, objectColor, specMap);
	if(spotLight.intensity > 0)
		light += ComputeSpot(spotLight, normal, fs_in.FragPos, viewDir, objectColor, specMap);

	color = vec4(light, 1.0f);
}

vec3 ComputePoint(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objectColor, vec3 specMap)
{
	float ambientStrength = 0.2f;
	vec3 lightDir = normalize(light.position - fragPos);
	// Diffuse
  	float diff = max(dot(normal, lightDir), 0.0);
  	// Specular
	vec3 halfwayDir = normalize(lightDir + viewDir);
  	float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

  	// Attenuation
  	float distance = length(light.position - fragPos);
  	float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

  	vec3 ambient = ambientStrength * objectColor;
  	vec3 diffuse = light.intensity * diff * objectColor;
  	vec3 specular = spec * specMap;

  return ((ambient + diffuse + specular) * attenuation * light.color);
}

vec3 ComputeSpot(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objectColor, vec3 specMap)
{
	float ambientStrength = 0.1f;
	vec3 lightDir = normalize(light.position - fragPos);

  	// Diffuse
  	float diff = max(dot(normal, lightDir), 0.0);
  	// Specular
  	vec3 reflectDir = reflect(-lightDir, normal);
  	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
  	// Attenuation
  	float distance = length(light.position - fragPos);
  	float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
  	// Spotlight intensity
  	float theta = dot(lightDir, normalize(-light.direction));
  	float epsilon = light.cutOff - light.outerCutOff;
  	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

  	vec3 ambient = ambientStrength * objectColor;
  	vec3 diffuse = light.intensity * diff * objectColor;
  	vec3 specular = spec * specMap;

  	return ((ambient + diffuse + specular) * attenuation * intensity);
}
