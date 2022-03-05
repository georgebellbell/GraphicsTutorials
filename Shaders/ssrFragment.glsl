#version 330 core

uniform sampler2D reflectionTex;

in vec4 clipSpace;

out vec4 fragColor;

void main(void) {
	vec2 ndc = (clipSpace.xy/clipSpace.w)/2.0 +0.5;
	vec2 reflectionTexCoords = vec2(ndc.x, -ndc.y);

	vec4 reflectColour = texture(reflectionTex, reflectionTexCoords);

	fragColor = reflectColour;
}