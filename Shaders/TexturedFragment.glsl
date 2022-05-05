#version 330 core
uniform sampler2D diffuseTex;
uniform vec4 colour;

uniform float reflection;

in Vertex	{
	vec2 texCoord;
} IN;

out vec4 fragColour;
void main(void){
	float reflectionPercentage = reflection / 100.0f;
	fragColour = texture(diffuseTex, IN.texCoord);
	fragColour.a = reflectionPercentage;
}