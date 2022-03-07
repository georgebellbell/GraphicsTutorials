#version 330 core 

uniform sampler2D diffuseTex;
uniform samplerCube cubeTex;

uniform vec3 cameraPos;

in Vertex {
	vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 worldPos;
} IN;

layout(location = 0) out vec4 gColour;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;

void main(void) {
	vec4 diffuse = texture(diffuseTex, IN.texCoord);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);

	vec3 reflectDir = reflect(-viewDir, normalize(IN.normal));
	vec4 reflectTex = texture(cubeTex, reflectDir);

	gColour = reflectTex + (diffuse * 0.25f);
	gPosition = vec4(0.0);
	gNormal = vec4(0.0);
}