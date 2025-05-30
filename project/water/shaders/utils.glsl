
//
vec3 GetCameraPosition(mat4 viewMatrix)
{
	vec3 position = viewMatrix[3].xyz;
	position = -(transpose(viewMatrix) * vec4(position, 0)).xyz;
	return position;
}

//
vec3 GetHalfVector(vec3 v1, vec3 v2)
{
   return normalize(v1 + v2);
}

//
vec3 GetDirection(vec3 fromPosition, vec3 toPosition)
{
	return normalize(toPosition - fromPosition);
}

//
float ClampedDot(vec3 v1, vec3 v2)
{
	return max(dot(v1, v2), 0.05);
}

//
vec3 GetImplicitNormal(vec2 normal)
{
	// Since the normal is in tangent space we can assume the z to be a positive value
	float z = sqrt(1.0f - normal.x * normal.x - normal.y * normal.y);
	return normalize(vec3(normal, z));
}

vec3 GetImplicitNormalClamped(vec2 normal)
{
	// Since the normal is in tangent space we can assume the z to be a positive value
	float z = clamp(sqrt(1.0f - normal.x * normal.x - normal.y * normal.y), 0, 1);
	return normalize(vec3(normal, z));
}

vec3 TransformTangentNormal(vec3 normalTangentSpace, vec3 normal, vec3 tangent)
{
	normal = normalize(normal);
	vec3 bitangent = normalize(cross(normal, tangent));
	tangent = cross(normal, bitangent);

	// Create tangent space matrix
	mat3 tangentMatrix = mat3(tangent, bitangent, normal);

	// Return matrix in world space
	return normalize(tangentMatrix * normalTangentSpace);
}

vec3 SampleDerivativeMap(sampler2D derivativeMap, vec2 texCoord)
{
	vec3 dh = texture(derivativeMap, texCoord).agb;
	dh.xy = dh.xy * 2 - 1;
	return dh;
}

vec3 SampleNormalMapScaled(sampler2D normalTexture, vec2 texCoord, vec3 normal, vec3 tangent, vec3 bitangent, float scale)
{
	// Read normalTexture
	vec2 normalMap = texture(normalTexture, texCoord).xy * 2 - vec2(1);
	normalMap *= scale;

	// Get implicit Z component
	vec3 normalTangentSpace = GetImplicitNormalClamped(normalMap);

	// Create tangent space matrix
	mat3 tangentMatrix = mat3(tangent, bitangent, normal);

	// Return matrix in world space
	return normalize(tangentMatrix * normalTangentSpace);
}

//
vec3 SampleNormalMap(sampler2D normalTexture, vec2 texCoord, vec3 normal, vec3 tangent, vec3 bitangent)
{
	// Read normalTexture
	vec2 normalMap = texture(normalTexture, texCoord).xy * 2 - vec2(1);

	// Get implicit Z component
	vec3 normalTangentSpace = GetImplicitNormalClamped(normalMap);

	// Create tangent space matrix
	mat3 tangentMatrix = mat3(tangent, bitangent, normal);

	// Return matrix in world space
	return normalize(tangentMatrix * normalTangentSpace);
}

//
vec3 SampleNormalMap(sampler2D normalTexture, vec2 texCoord, vec3 normal, vec3 tangent)
{
	// Build the tangent space base vectors
	normal = normalize(normal);
	vec3 bitangent = normalize(cross(normal, tangent));
	tangent = cross(normal, bitangent);

	return SampleNormalMap(normalTexture, texCoord, normal, tangent, bitangent);
}

//
vec3 ReconstructViewPosition(sampler2D depthTexture, vec2 texCoord, mat4 invProjMatrix)
{
	// Reconstruct the position, using the screen texture coordinates and the depth
	float depth = texture(depthTexture, texCoord).r;
	vec3 clipPosition = vec3(texCoord, depth) * 2.0f - vec3(1.0f);
	vec4 viewPosition = invProjMatrix * vec4(clipPosition, 1.0f);
	return viewPosition.xyz / viewPosition.w;
}

float GetLuminance(vec3 color)
{
   return dot(color, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 RGBToHSV(vec3 rgb)
{
   vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);

   vec4 p = mix( vec4( rgb.bg, K.wz ), vec4( rgb.gb, K.xy ), step( rgb.b, rgb.g ) );
   vec4 q = mix( vec4( p.xyw, rgb.r ), vec4( rgb.r, p.yzx ), step( p.x, rgb.r ) );

   float d = q.x - min( q.w, q.y );

   float epsilon = 1.0e-10;

   return vec3( abs(q.z + (q.w - q.y) / (6.0 * d + epsilon)), d / (q.x + epsilon), q.x);
}

vec3 HSVToRGB( vec3 hsv )
{
   vec4 K = vec4( 1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0 );

   vec3 p = abs( fract( hsv.xxx + K.xyz ) * 6.0 - K.www );

   return hsv.z * mix( K.xxx, clamp(p - K.xxx, 0, 1), hsv.y );
}

float LinearizeDepth(float d ,float zNear, float zFar)
{
	float ndc = d * 2.0 - 1.0; 
    return (2 * zNear * zFar) / (zFar + zNear - ndc * (zNear - zFar));
}

vec4 TransformViewToScreenSpace(vec4 view, mat4 projMatrix, vec2 texSize)
{
	vec4 frag = view;

	// Project frag to screen space
	frag = projMatrix * frag;

	// Perspective Division
	frag.xyz /= frag.w;

	// Convert to UV coordinates
	frag.xy = frag.xy * 0.5 + 0.5;

	// Convert to fragment coordinate
	frag.xy *= texSize;
	
	return frag;
}

