#version 330 core

uniform samplerCube cubeTex;
in Vertex {
	vec3 viewDir;
} IN;

layout(location = 0) out vec4 gColour;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;

void main(void) {
	gColour = texture(cubeTex, normalize(IN.viewDir));
	gPosition = vec4(0, 0, 0, 1);
	gNormal = vec4(0, 0, 0, 1);
}