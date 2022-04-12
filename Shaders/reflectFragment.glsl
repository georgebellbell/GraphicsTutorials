#version 330 core 

uniform samplerCube cubeTex;

uniform vec3 cameraPos;

in Vertex {
	vec4 colour;
	vec3 normal;
	vec3 worldPos;
} IN;

layout(location = 0) out vec4 gColour;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;

void main(void) {

	vec3 viewDir = normalize(cameraPos - IN.worldPos);

	vec3 reflectDir = reflect(-viewDir, normalize(IN.normal));
	vec4 reflectTex = texture(cubeTex, reflectDir);

	gColour =  reflectTex + (vec4(1,0,0, 1) * 0.5);
	gPosition = vec4(0.0);
	gNormal = vec4(0.0);
}