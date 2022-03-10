#version 330 core

uniform sampler2D colourTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform samplerCube cubeTex;
uniform sampler2D depthTex;

uniform vec2 pixelSize;
uniform vec3 cameraPos;

uniform mat4 inverseProjView;

/*
in Vertex {
	vec2 texCoord;
} IN;
*/
out vec4 fragColor;

void main()
{
	vec2 texCoord = vec2(gl_FragCoord.xy * pixelSize);
	float depth = texture(depthTex, texCoord.xy).r;
	vec3 ndcPos = vec3(texCoord, depth) * 2.0 - 1.0;
	vec4 invClipPos = inverseProjView * vec4(ndcPos, 1.0);
	vec3 worldPos = invClipPos.xyz / invClipPos.w;

	vec3 viewDir = normalize(cameraPos - worldPos);

	vec3 normal = normalize(texture(normalTex, texCoord.xy).xyz * 2.0 - 1.0);
	vec3 reflectDir = reflect(-viewDir, normalize(normal));

	vec4 reflectTex = texture(cubeTex, reflectDir);

	vec4 diffuse = texture(colourTex, texCoord);

	fragColor = reflectTex + (diffuse * 0.25);

	//fragColor = vec4(texture(colourTex, texCoord).xyz, 1.0);

}