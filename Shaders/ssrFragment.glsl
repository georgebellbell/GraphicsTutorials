#version 330 core

uniform sampler2D colourTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform sampler2D depthTex;
uniform samplerCube cubeTex;

uniform vec2 pixelSize;
uniform vec3 cameraPos;
uniform mat4 inverseProjView;
uniform float reflection;

in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColor;

void main()
{
	float depth = texture(depthTex, IN.texCoord.xy).r;
	if (depth == 1.0){
		fragColor = vec4(texture(colourTex, IN.texCoord).xyz, 1.0);
		return;
	}

	float reflectionPercent = reflection / 100;
	float objectPercent = 1 - reflectionPercent;
	// Calculating world space position of fragment
	vec3 ndcPos = vec3(IN.texCoord, depth)* 0.5 + 0.5;
	vec4 invClipPos = inverseProjView * vec4(ndcPos, 1.0);
	vec3 worldPos = invClipPos.xyz / invClipPos.w;

	// Calculating reflection vector
	vec3 viewDir = normalize(worldPos - cameraPos);
	vec3 normal = normalize(texture(normalTex, IN.texCoord.xy).xyz * 2.0 - 1.0);
	vec3 reflectDir = reflect(viewDir, normal);

	// Convert reflection vector back to screen space
	vec4 clipSpaceDir = inverse(inverseProjView) * vec4(reflectDir, 1.0);
	vec3 ndcSpaceDir = clipSpaceDir.xyz / clipSpaceDir.w;
	vec2 screenSpaceDir = normalize((ndcSpaceDir.xy + 1.0) / 2.0);

	vec2 newFrag = vec2(gl_FragCoord.xy);
	vec2 newTexCoord;
	float newDepth;

	float iterate = 0.0f;
	
	while (newFrag.x < -1 || newFrag.x > 1 || newFrag.y < -1 || newFrag.y > 1) 
	{
		iterate = iterate + 1.0f;

		if (iterate >=300.0f)
		{
			break;
		}
		//move along by one "fragment"
		newFrag = newFrag + screenSpaceDir;

		newTexCoord = vec2(newFrag.xy * pixelSize);
		newDepth = texture(depthTex , newTexCoord.xy).r;

		// check if something has been hit
		if (newDepth < depth) {
			fragColor = vec4(texture(colourTex, newTexCoord).xyz, 1.0) * reflectionPercent + vec4(texture(colourTex, IN.texCoord).xyz, 1.0) * objectPercent;
			return;
		}

	}
	
	fragColor = vec4(texture(colourTex, IN.texCoord).xyz, 1.0);
}