#version 330 core

#define NUM_SAMPLES 100.0f //antilag
#define G_SCATTERING 0.2f
#define PI 3.14159f
#define TAU 0.00002f

out vec4 color;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    
} fs_in;

//view and directional light
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

//shadow map
uniform sampler2D shadowMap;

uniform mat4 lightSpaceMatrix;

float ComputeScattering(float lightDotView)
{
  float numerator = 1.0f - G_SCATTERING * G_SCATTERING;
  float denominator = (4.0f * PI * pow(1.0f + G_SCATTERING * G_SCATTERING - (2.0f * G_SCATTERING *  lightDotView), 1.5f));
  return numerator/denominator;
}

void main()
{
  vec3 rayVector = fs_in.FragPos - viewPos;

  float rayLength = length(rayVector);
  vec3 rayDirection = rayVector / rayLength;
  vec3 lightDir = normalize(lightPos);
  float mie_phase = ComputeScattering(dot(rayDirection, lightDir));
  float stepSize = rayLength / NUM_SAMPLES;

  vec3 currentPosition = viewPos;

   //total light contribution accumulated along the ray
  float L_insc = 1.0f * exp(- rayLength * TAU);
  float prev_ins = 1.0f;
  float curr_ins = 0.0f;

  //start the actual ray marching
  for(int i = 0; i < NUM_SAMPLES; i++)
  {
    // transform into light space and perform perspective divide
    vec4 sampleLS = lightSpaceMatrix * vec4(currentPosition, 1.0f);
    vec3 sample = sampleLS.xyz / sampleLS.w;
    sample = sample * 0.5f + 0.5f;

    float shadowMapValue = texture(shadowMap, sample.xy).r;
	float d = stepSize * i; //travelled distance on the ray
    curr_ins = exp(- d * TAU);
    if (shadowMapValue > sample.z){
      L_insc += mie_phase;
    }
    
	prev_ins = curr_ins;
	currentPosition += stepSize * rayDirection;
  }
  //vec3 scattering = L_insc * mie_phase * lightColor;
  vec3 scattering = L_insc / NUM_SAMPLES * lightColor;
  color = 6 * vec4(scattering, 1.0f);
}
