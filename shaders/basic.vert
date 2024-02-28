#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
out vec3 fLightPos1;
out vec3 fLightPos2;
out vec3 fRedLightPos;
out vec4 fragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos1;
uniform vec3 lightPos2;
uniform vec3 redLightPos;
uniform mat4 lightModel;
uniform mat4 redLightModel;
uniform mat4 lightSpaceTrMatrix;

void main() 
{
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);
	fragPosLightSpace = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f);
	fPosition = vPosition;
	fNormal = vNormal;
	//vec4 viewPos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	fLightPos1 = vec3(view * lightModel * vec4(lightPos1, 1.0f));
	fLightPos2 = vec3(view * lightModel * vec4(lightPos2, 1.0f));
	fRedLightPos = vec3(view * redLightModel * vec4(redLightPos, 1.0f));
	fTexCoords = vTexCoords;
}
