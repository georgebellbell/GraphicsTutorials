#version 330 core 

uniform samplerCube cubeTex;

uniform vec3 cameraPos;

uniform float reflectionPower;

in Vertex {
	vec4 colour;
	vec3 normal;
	vec3 worldPos;
} IN;

layout(location = 0) out vec4 gColour;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;

void main(void) {

	if (reflectionPower == 95.0f){
		gColour = vec4(1, 0, 0, 1);
		return;
	}
	float reflectPercent = reflectionPower / 100;
	float objectPercent = 1 - reflectPercent;

	vec3 viewDir = normalize(cameraPos - IN.worldPos);

	vec3 reflectDir = reflect(-viewDir, normalize(IN.normal));
	vec4 reflectTex = texture(cubeTex, reflectDir);
	vec4 grey = vec4(0.5,0.5, 0.5, 1);

	gColour =  grey * objectPercent + reflectTex * reflectPercent;
	gPosition = vec4(0.0);
	gNormal = vec4(0.0);
}