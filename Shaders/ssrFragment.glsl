#version 330 core

uniform sampler2D colourTex;
uniform sampler2D positionTex;
uniform sampler2D normalTex;
uniform samplerCube cubeTex;
uniform sampler2D depthTex;

uniform vec2 pixelSize;
uniform vec3 cameraPos;
uniform float shininess;
uniform mat4 inverseProjView;

in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColor;

void main()
{
	if (shininess == 5.3f)
	{
		fragColor = vec4(1, 0, 0, 1);
		return;
	}
	// to world space...
	float depth = texture(depthTex, IN.texCoord.xy).r;
	if (depth == 1.0){
		fragColor = vec4(texture(colourTex, IN.texCoord).xyz, 1.0);
		return;
	}

	vec3 ndcPos = vec3(IN.texCoord, depth)* 0.5 + 0.5;
	vec4 invClipPos = inverseProjView * vec4(ndcPos, 1.0);
	vec3 worldPos = invClipPos.xyz / invClipPos.w;
	float width = 1 / pixelSize.x;
	float height = 1 / pixelSize.y;

	vec3 viewDir = normalize(cameraPos - worldPos);
	vec3 normal = normalize(texture(normalTex, IN.texCoord.xy).xyz * 2.0 - 1.0);
	vec3 reflectDir = reflect(-viewDir, normalize(normal));

	// Check for if reflection is directly towards camera

	//float dotProduct = max (dot(reflectDir, viewDir), 0.0f);
	//if (dotProduct <= 0.1) {
	//	fragColor = vec4(1,0, 0, 1);
	//	return;
	//}


	// ...and back to screen space
	vec4 clipSpaceDir = inverse(inverseProjView) * vec4(reflectDir, 1.0);
	
	vec3 ndcSpaceDir = clipSpaceDir.xyz / clipSpaceDir.w;

	vec2 screenSpaceDir = normalize((ndcSpaceDir.xy + 1.0) / 2.0);

	//Different attempts at changing screenSpaceDir to per fragment
	//screenSpaceDir.x = screenSpaceDir.x / width;
	//screenSpaceDir.y = screenSpaceDir.y / height;

	//screenSpaceDir = screenSpaceDir * pixelSize;	
	fragColor = vec4(screenSpaceDir, 0.0, 1.0);
	//return;


	vec2 currentFrag = vec2(gl_FragCoord.xy);
	vec2 currentTexCoord;
	float currentDepth;

	float iterate = 0.0f;
	
	while (currentFrag.x < -1 || currentFrag.x > 1 || 
			currentFrag.y < -1 || currentFrag.y > 1) 
	{
		iterate = iterate + 1.0f;

		if (iterate >= 300.0f)
		{
			break;
		}
		//move along by one "fragment"
		currentFrag = currentFrag + screenSpaceDir;
		currentTexCoord = vec2(currentFrag.xy * pixelSize);
		currentDepth = texture(depthTex , currentTexCoord.xy).r;

		// check if something has been hit
		if (currentDepth < depth) {
			fragColor = vec4(texture(colourTex, currentTexCoord).xyz, 1.0);
			return;
		}

	}
	//vec4 reflectTex = texture(cubeTex, reflectDir);
	//vec4 diffuse = texture(colourTex, IN.texCoord);
	//fragColor = reflectTex + (diffuse * 0.25);

	fragColor = vec4(texture(colourTex, IN.texCoord).xyz, 1.0);

}