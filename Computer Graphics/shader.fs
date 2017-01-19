#version 330 core

struct DirectionalLight {
	vec3 direction;
	vec3 color;
	float intensity;
};

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

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

out vec4 color;

uniform vec3 viewPos;

//textures
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

//lights
uniform DirectionalLight dirLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;

//functions to compute light components
vec3 ComputeDir(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 objectColor, vec3 specMap);
vec3 ComputePoint(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objectColor, vec3 specMap);
vec3 ComputeSpot(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objectColor, vec3 specMap);

void main()
{  
	//fixed properties
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos-FragPos);
	vec3 objectColor =  vec3(texture(texture_diffuse1, TexCoords));
	vec3 specMap = vec3(texture(texture_specular1, TexCoords));
	
	vec3 directional = ComputeDir(dirLight, norm, viewDir, objectColor, specMap);
	vec3 point = ComputePoint(pointLight, norm, FragPos, viewDir, objectColor, specMap);
	vec3 spot = ComputeSpot(spotLight, norm, FragPos, viewDir, objectColor, specMap);
	vec3 light = directional + point + spot;
		
	color = vec4(light, 1.0f);	
}

vec3 ComputeDir(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 objectColor, vec3 specMap)
{
	float ambientStrength = 0.1f;
	vec3 lightDir = normalize(-light.direction);
	//Diffuse
	float diff = max(dot(normal, lightDir),0.0);
	//Specular
	vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

	vec3 ambient = ambientStrength * objectColor;
	vec3 diffuse =  light.intensity * diff * objectColor;
	vec3 specular = light.intensity * spec * specMap;

	return ((ambient + diffuse + specular) * light.color);
}

vec3 ComputePoint(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objectColor, vec3 specMap)
{
	float ambientStrength = 0.1f;
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




