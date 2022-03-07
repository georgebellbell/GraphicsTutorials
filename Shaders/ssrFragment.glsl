#version 330 core

uniform sampler2D colour;
uniform sampler2D position;
uniform sampler2D normal;
uniform sampler2D effect;
uniform vec2 texSizeInv;

const float rStep = 0.25;
const float minRStep = 0.1;
const float maxSteps = 20;
const float searchDistance = 5;
const float searchDistanceInv = 0.2;
const int binarySearchSteps = 5;
const float maxDDepth = 1.0;
const float maxDDepthInv = 1.0;

const float fallOffExponent = 3.0;

uniform mat4 projMatrix;

vec3 BSearch(vec3 direction, inout vec3 hitCoord, out float dDepth)
{

	float depth;

	for (int i = 0; i < binarySearchSteps; i++) {

		vec4 projectedCoordinate = projMatrix * vec4(hitCoord, 1.0);
		projectedCoordinate.xy /= projectedCoordinate.w;
		projectedCoordinate.xy = projectedCoordinate.xy * 0.5 + 0.5;

		depth = texture(position, projectedCoordinate.xy).z;

		dDepth = hitCoord.z - depth;

		if(dDepth > 0.0) {
			hitCoord += direction;
		}

		direction *= 0.5;
		hitCoord -= direction;
	}

	vec4 projectedCoordinate = projMatrix * vec4(hitCoord, 1.0);
		projectedCoordinate.xy /= projectedCoordinate.w;
		projectedCoordinate.xy = projectedCoordinate.xy * 0.5 + 0.5;

	return vec3(projectedCoordinate.xy, depth);
}

vec4 RayMarch(vec3 direction, inout vec3 hitCoord, out float dDepth)
{
	direction *= rStep;

	float depth;

	for (int i = 0; i < maxSteps; i++)
	{
		hitCoord += direction;

		vec4 projectedCoordinate = projMatrix * vec4(hitCoord, 1.0);
		projectedCoordinate.xy /= projectedCoordinate.w;
		projectedCoordinate.xy = projectedCoordinate.xy * 0.5 + 0.5;

		depth = texture(position, projectedCoordinate.xy).z;

		dDepth = hitCoord.z - depth;

		if (dDepth < 0.0)
		{
			return vec4(BSearch(direction, hitCoord, dDepth), 1.0);
		}

	}

	return vec4(0.0, 0.0, 0.0, 0.0);
}


in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColor;

void main()
{
	vec2 gTexCoord = IN.texCoord.xy * texSizeInv;

	float specular = texture(colour, gTexCoord).a;

	if(specular == 0.0)
	{
		fragColor = vec4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	vec3 viewNormal = texture(normal, gTexCoord).xyz;
	vec3 viewPos = texture(position, gTexCoord).xyz;

	vec3 reflected = normalize(reflect(normalize(viewPos), normalize(viewNormal)));

	vec3 hitPos = viewPos;
	float dDepth;

	vec4 coords = RayMarch(reflected * max(minRStep, -viewPos.z), hitPos, dDepth);

	vec2 dCoords = abs(vec2(0.5, 0.5) - coords.xy);

	float screenEdgeFactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0);

	fragColor = vec4(texture(effect, coords.xy).rgb, pow(specular, fallOffExponent) * screenEdgeFactor * clamp(-reflected.z, 0.0, 1.0) *
				clamp((searchDistance - length(viewPos - hitPos)) * searchDistanceInv, 0.0, 1.0) * coords.w);

	//fragColor = vec4(texture(colour, IN.texCoord).xyz, 1.0);
}