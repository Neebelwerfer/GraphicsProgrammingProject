//Inputs
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform sampler2D DepthTexture;
uniform sampler2D SourceTexture;
uniform sampler2D NormalTexture;
uniform mat4 ProjectionMatrix;
uniform mat4 InvProjMatrix;

//SSR Properties
uniform float MaxDistance;
uniform float Resolution;
uniform int Steps;
uniform float Thickness;

void main()
{
	float steps = Steps;
	vec2 texSize = textureSize(DepthTexture, 0).xy;
	vec4 uv = vec4(0);

	// Fragment data
	vec3 positionFrom = ReconstructViewPosition(DepthTexture, TexCoord, InvProjMatrix);
	vec3 unitPositionFrom = normalize(positionFrom);
	vec3 normal = normalize(GetImplicitNormal(texture(NormalTexture, TexCoord).xy));
	vec3 reflection = normalize(reflect(unitPositionFrom, normal));

	vec4 startView = vec4(positionFrom, 1);
	vec4 endView = vec4(positionFrom + (reflection * MaxDistance), 1);

	// Early exit 
	// 1. if depth is far clip
	// 2. if the is near parallel or parallel with the viewing angle
	if (texture(DepthTexture, TexCoord).r == 1 || dot(normal, -unitPositionFrom) > 0.9)
	{
		FragColor = vec4(0, 0, 0, 0);
		return;
	}

	// Convert our start point and end point to screen space coordinates
	vec4 startFrag = startView;
	// Project frag to screen space
	startFrag = ProjectionMatrix * startFrag;
	// Perspective Division
	startFrag.xyz /= startFrag.w;
	// Convert to UV coordinates
	startFrag.xy = startFrag.xy * 0.5 + 0.5;
	// Convert to fragment coordinates
	startFrag.xy *= texSize;

	// Do the same to the endFrag as startFrag
	vec4 endFrag = endView;
	endFrag = ProjectionMatrix * endFrag;
	endFrag.xyz /= endFrag.w;
    endFrag.xy = endFrag.xy * 0.5 + 0.5;
	endFrag.xy *= texSize;

	vec2 frag = startFrag.xy;
	uv.xy = frag;

	float deltaX = endFrag.x - startFrag.x;
	float deltaY = endFrag.y - startFrag.y;

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

	vec3 positionTo = positionFrom;
	vec3 normalTo = normal;
	float minimumDepth = 0;

	// First Pass
	// Lets see if there is a hit at all
	float i = 0;
	for (i = 0; i < int(delta); ++i)
	{
		// Increment to new fragment
		frag += increment;
		uv.xy = frag / texSize;

		// Get fragment view space data
		positionTo = ReconstructViewPosition(DepthTexture, uv.xy, InvProjMatrix);
		normalTo = normalize(GetImplicitNormal(texture(NormalTexture, uv.xy).xy));

		// Scaling minimun depth difference based on if the normal of start position and the normal of sampled position is near parallel
		// Hack to remove self reflection on a plane
		minimumDepth = mix(0, Thickness, dot(normal, normalTo) < 0.9 ? 0.0 : 1.0);

		// Calculate current progress along the reflectance vector
		search1 = mix((frag.y - startFrag.y) / deltaY, (frag.x - startFrag.x) / deltaX, useX);
		search1 = clamp(search1, 0.0, 1.0);

		// Interpolate to the new point on the reflectance vector to get new viewDistance
		viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
		depth = viewDistance - positionTo.z;

		// Break if out of bounds
		if(uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || positionTo.z >= 0)
		{
			uv = vec4(0);
			break;
		}
		// Break if we register a hit
		else if (depth < 0 && depth > -Thickness && abs(positionFrom.z - positionTo.z) > minimumDepth)
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
		positionTo = ReconstructViewPosition(DepthTexture, uv.xy, InvProjMatrix);

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

	vec3 reflectiveColor = mix(vec3(0.0), texture(SourceTexture, uv.xy).rgb, hit0);
	FragColor = vec4(reflectiveColor, hit1);
}
