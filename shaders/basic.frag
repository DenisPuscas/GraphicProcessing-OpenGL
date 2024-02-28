#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec3 fLightPos1;
in vec3 fLightPos2;
in vec3 fRedLightPos;
in vec4 fragPosLightSpace;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform int redLightOn;
//textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
//fog
uniform float fogDensity;
uniform vec3 fogColor;
//shadow
uniform sampler2D shadowMap;

//components
vec4 fPosEye;
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
vec3 normalEye;
float shininess = 32.0f;

float constant = 1.0f;
float linear = 0.7f;
float quadratic = 1.8f;

float linear2 = 0.09f;
float quadratic2 = 0.032f;

float shadow;
vec3 lightColorPunct;

vec3 ambientPoint;
vec3 diffusePoint;
vec3 specularPoint;

void computeDirLight()
{
    //compute eye space coordinates
    fPosEye = view * model * vec4(fPosition, 1.0f);
    normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
	
	//compute half vector
	vec3 halfVector = normalize(lightDirN + viewDir);

    //compute specular light
    //vec3 reflectDir = reflect(-/lightDirN, normalEye);
    //float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;
}

void computePunctLigth(vec3 fLightPos, vec3 lightColorPunct, float lin, float quad)
{
	vec3 cameraPosEye = vec3(0.0f);
	
	//normalize light direction
	vec3 lightDirN = normalize(fLightPos - fPosEye.xyz);

	//compute view direction
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);

	//compute half vector
	vec3 halfVector = normalize(lightDirN + viewDirN);
	
	//compute specular light
	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
	specularPoint = specularStrength * specCoeff * lightColorPunct;
	
	//compute distance to light
	float dist = length(fLightPos - fPosEye.xyz);
	//compute attenuation
	float att = 1.0f / (constant + lin * dist + quad * (dist * dist));
	
	//compute ambient light
	ambientPoint += att * ambientStrength * lightColorPunct;
	
	//compute diffuse light
	diffusePoint += att * max(dot(normalEye, lightDirN), 0.0f) * lightColorPunct;
	
	specularPoint += att * specularStrength * specCoeff * lightColorPunct;
	
	ambient += ambientPoint;
	diffuse += diffusePoint;
	specular += specularPoint;

}

void computeShadow()
{
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	normalizedCoords = normalizedCoords * 0.5 + 0.5;
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	float currentDepth = normalizedCoords.z;
	float bias = 0.005f;
	shadow = (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
	if (currentDepth > 1.0f)
		shadow = 0.0f;
}

float computeFog()
{
	float fragmentDistance = length(fPosEye);
	float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
 
	return clamp(fogFactor, 0.0f, 1.0f);
}

void main() 
{
    computeDirLight();
	computePunctLigth(fLightPos1, vec3(0.32f, 0.75f, 0.78f), linear, quadratic);
	computePunctLigth(fLightPos2, vec3(0.32f, 0.75f, 0.78f), linear, quadratic);
	if (redLightOn > 0) computePunctLigth(fRedLightPos, vec3(1.0f, 0.25f, 0.0f), linear2, quadratic2);
	
	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;
	
	computeShadow();
	vec3 color = min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular, 1.0f);

    //compute final vertex color
    //vec3 color = min((ambient + diffuse) * texture(diffuseTexture, fTexCoords).rgb + specular * texture(specularTexture, fTexCoords).rgb, 1.0f);

	float fogFactor = computeFog();
	
	fColor = mix(vec4(fogColor, 1.0f), vec4(color, 1.0f), fogFactor);
	//fColor = fogColor * (1 – fogFactor) + vec4(color, 1.0f) * fogFactor;
    //fColor = vec4(color, 1.0f);
}
