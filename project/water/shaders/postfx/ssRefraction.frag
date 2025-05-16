//Inputs
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform sampler2D BackgroundColorTexture;
uniform sampler2D DepthFromTexture;
uniform sampler2D DepthToTexture;
uniform sampler2D NormalFromTexture;
uniform sampler2D SpecularTexture;
uniform mat4 ProjectionMatrix;
uniform mat4 InvProjMatrix;
uniform int Enabled;

//SSR Properties
uniform float MaxDistance;
uniform float Resolution;
uniform int Steps;
uniform float Thickness;
uniform float Rior;
uniform float DepthMax;

vec4 TintResult(vec2 uv, vec3 positionFrom, vec3 positionTo, float visibility)
{
	vec4 backgroundColor = texture(BackgroundColorTexture, uv);
	vec4  tintColor = vec4(0.27, 0.58, 0.92, 0.3);

	float depth = length(positionTo.xyz - positionFrom.xyz);
	float mixture = clamp(depth / DepthMax, 0, 1);

	vec3 shallowColor    = backgroundColor.rgb;
	vec3 deepColor       = mix(shallowColor, tintColor.rgb, tintColor.a);
	vec3 foregroundColor = mix(shallowColor, deepColor,     mixture);

	return mix(vec4(0), vec4(foregroundColor, 1), visibility);
}

void main()
{
	float steps = Steps;
	vec2 texSize = textureSize(DepthFromTexture, 0).xy;
	vec4 uv = vec4(TexCoord, 1, 1);

	// Fragment data
	vec3 positionFrom = ReconstructViewPosition(DepthFromTexture, TexCoord, InvProjMatrix);
	vec3 unitPositionFrom = normalize(positionFrom);
	vec3 normalFrom = normalize(GetImplicitNormal(texture(NormalFromTexture, TexCoord).xy));
	vec3 refraction = normalize(refract(unitPositionFrom, normalFrom, Rior));

	float refractionMask = texture(SpecularTexture, TexCoord).a;
	vec3 positionTo = positionFrom;

	// Early exit 
	// 1. if depth is far clip
	// 2. if the reflection goes toward the camera
	if (texture(DepthFromTexture, TexCoord).r == 1 || refractionMask <= 0 || Enabled == 0)
	{
		FragColor = texture(BackgroundColorTexture, TexCoord);
		return;
	}


	vec4 startView = vec4(positionFrom, 1);
	vec4 endView = vec4(positionFrom + (refraction * MaxDistance), 1);

	// Convert our start point and end point to screen space coordinates
	vec4 startFrag = TransformViewToScreenSpace(startView, ProjectionMatrix, texSize);
	vec4 endFrag = TransformViewToScreenSpace(endView, ProjectionMatrix, texSize);

	vec2 frag = startFrag.xy;
	uv.xy = frag / texSize;

	// The differences between the start fragment and end fragment
	float deltaX = endFrag.x - startFrag.x;
	float deltaY = endFrag.y - startFrag.y;

	// Calculate how big steps we take. The sizes of the steps are adjusted by resolution
	// higher resolution leads to smaller but more steps
	float useX = abs(deltaX) >= abs(deltaY) ? 1.0 : 0.0;
	float delta = mix(abs(deltaY), abs(deltaX), useX) * clamp(Resolution, 0.0, 1.0);
	vec2 increment = vec2(deltaX, deltaY) / max(delta, 0.001);

	// Track our progress along the vector
	float search0 = 0;
	float search1 = 0;

	// Track hit in first and second pass respectively
	int hit0 = 0;
	int hit1 = 0;

	float viewDistance = startView.z;
	float depth = Thickness;

	// First Pass
	// Lets see if there is a hit at all
	float i = 0;
	for (i = 0; i < int(delta); ++i)
	{
		// Increment to new fragment
		frag += increment;
		uv.xy = frag / texSize;

		// Get fragment view space data
		positionTo = ReconstructViewPosition(DepthToTexture, uv.xy, InvProjMatrix);

		// Calculate current progress along the reflectance vector
		search1 = mix((frag.y - startFrag.y) / deltaY, (frag.x - startFrag.x) / deltaX, useX);
		search1 = clamp(search1, 0.0, 1.0);

		// Interpolate to the new point on the reflectance vector to get new viewDistance
		viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
		depth = viewDistance - positionTo.z;

		// Break if out of bounds
		if(uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || positionTo.z >= 0)
		{
			FragColor = texture(BackgroundColorTexture, TexCoord);
			return;
		}
		// Break if we register a hit
		else if (depth < 0 && depth > -Thickness)
		{
			hit0 = 1;
			break;
		}
		// Save our current search progress in search0
		else 
		{
			search0 = search1;
		}
	}

	search1 = search0 + ((search1 - search0) / 2);
	steps *= hit0;

	// Second Pass
	// If there is a hit, lets iterate around the area to find a more precise hit
	for (i = 0; i < steps; ++i)
	{
		// Increment to new fragment
		frag = mix(startFrag.xy, endFrag.xy, search1);
		uv.xy = frag / texSize;

		// Get fragment view space data
		positionTo = ReconstructViewPosition(DepthToTexture, uv.xy, InvProjMatrix);

		// Interpolate to the new point on the reflectance vector to get new viewDistance
		viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
		depth = viewDistance - positionTo.z;

		// Register another hit but keep iterating by moving the progress a small bit
		if(depth < 0 && depth > -Thickness)
		{
			hit1 = 1;
			search1 = search0 + ((search1 - search0) / 2);
		}
		// If no hit, move again and save our progess into search0
		else
		{
			float temp = search1;
			search1 = search1 + ((search1 - search0) / 2);
			search0 = temp;
		}
	}

	  float visibility = hit1
    * (1 - max(dot(-unitPositionFrom, refraction), 0))
    * (uv.x < 0 || uv.x > 1 ? 0 : 1)
    * (uv.y < 0 || uv.y > 1 ? 0 : 1);

	FragColor = TintResult(uv.xy, positionFrom, positionTo, visibility);
}
