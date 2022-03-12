#version 330 core

uniform sampler2D colourTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform samplerCube cubeTex;
uniform sampler2D depthTex;

uniform vec2 pixelSize;
uniform vec3 cameraPos;

uniform mat4 inverseProjView;



in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColor;

void main()
{
	// to world space...
	float depth = texture(depthTex, IN.texCoord.xy).r;
	vec3 ndcPos = vec3(IN.texCoord, depth) * 2.0 - 1.0;
	vec4 invClipPos = inverseProjView * vec4(ndcPos, 1.0);
	vec3 worldPos = invClipPos.xyz / invClipPos.w;
	float width = 1 / pixelSize.x;
	float height = 1 / pixelSize.y;

	vec3 viewDir = normalize(cameraPos - worldPos);

	vec3 normal = normalize(texture(normalTex, IN.texCoord.xy).xyz * 2.0 - 1.0);

	if (normal == vec3(0, 0, 0))
	{
		fragColor = vec4(texture(colourTex, IN.texCoord).xyz, 1.0);
		return;
	}
	vec3 reflectDir = reflect(-viewDir, normalize(normal));

	

	// ...and back to screen space
	vec4 clipSpaceDir = inverse(inverseProjView) * vec4(reflectDir, 1.0);
	
	vec3 ndcSpaceDir = clipSpaceDir.xyz / clipSpaceDir.w;

	vec2 screenSpaceDir = normalize((ndcSpaceDir.xy + 1.0) / 2.0);

	//per pixelSize
	//screenSpaceDir.x = screenSpaceDir.x / width;
	//screenSpaceDir.y = screenSpaceDir.y / height;
	// screenSpaceDir = screenSpaceDir / pixelSize;	

	vec2 currentFrag = vec2(gl_FragCoord.xy);
	vec2 currentTexCoord;
	float currentDepth;

	float iterate = 0.0f;
	//fragColor = vec4(texture(colourTex, IN.texCoord).xyz, 1.0);
	//fragColor = vec4(screenSpaceDir, texture(colourTex, IN.texCoord).z,  1.0);

	//return;
	
	while (currentFrag.x < -1 || currentFrag.x > 1 || 
			currentFrag.y < -1 || currentFrag.y > 1) 
	{
		iterate = iterate + 1.0f;

		if (iterate >= 300.0f)
		{
			break;
			fragColor = vec4(1,0, 0, 1);
			return;
		}
		//move along by one fragment
		currentFrag = currentFrag + screenSpaceDir;
		currentTexCoord = vec2(currentFrag.xy * pixelSize);
		currentDepth = texture(depthTex , currentTexCoord.xy).r;

		// check if something has been hit
		if (currentDepth < depth) {
			fragColor = vec4(texture(colourTex, currentTexCoord).xyz, 1.0);
			return;
		}

	}
	vec4 reflectTex = texture(cubeTex, reflectDir);

	vec4 diffuse = texture(colourTex, IN.texCoord);

	fragColor = reflectTex + (diffuse * 0.25);

	fragColor = vec4(texture(colourTex, IN.texCoord).xyz, 1.0);

}