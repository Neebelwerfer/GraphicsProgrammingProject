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
uniform mat4 InvViewMatrix;

uniform samplerCube EnvironmentTexture;
uniform float EnvironmentMaxLod;

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

	vec3 positionFrom = ReconstructViewPosition(DepthTexture, TexCoord, InvProjMatrix);
	vec3 unitPositionFrom = normalize(positionFrom);
	vec3 normal = normalize(GetImplicitNormal(texture(NormalTexture, TexCoord).xy));
	vec3 pivot = normalize(reflect(unitPositionFrom, normal));

	vec4 startView = vec4(positionFrom, 1);
	vec4 endView = vec4(positionFrom + (pivot * MaxDistance), 1);

	//Early stop if depth is far clip
	if (texture(DepthTexture, TexCoord).r == 1)
	{
		FragColor = vec4(0, 0, 0, 0);
		return;
	}

	//Convert our start point and end point to screen space coordinates
	vec4 startFrag = startView;
	//Project frag to screen space
	startFrag = ProjectionMatrix * startFrag;
	//Perspective Division
	startFrag.xyz /= startFrag.w;
	//Convert to UV coordinates
	startFrag.xy = startFrag.xy * 0.5 + 0.5;
	//Convert to fragment coordinates
	startFrag.xy *= texSize;

	//Do the same to the endFrag as startFrag
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

	//Track our progress along the vector
	float search0 = 0;
	float search1 = 0;

	//Track hit in first and second pass respectively
	int hit0 = 0;
	int hit1 = 0;

	float viewDistance = startView.z;
	float depth = Thickness;

	vec3 positionTo = positionFrom;

	if(dot(normal, vec3(0, 0, 1)) > 0.85 || dot(normal, -unitPositionFrom) > 0.70)
	{
		delta = 0;
	}

	//First Pass
	//Lets see if there is a hit at all
	float i = 0;
	for (i = 0; i < int(delta); ++i)
	{
		frag += increment;
		uv.xy = frag / texSize;
		positionTo = ReconstructViewPosition(DepthTexture, uv.xy, InvProjMatrix);

		search1 = mix((frag.y - startFrag.y) / deltaY, (frag.x - startFrag.x) / deltaX, useX);
		search1 = clamp(search1, 0.0, 1.0);

		viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
		depth = viewDistance - positionTo.z;

		if(uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1 || positionTo.z >= 0)
		{
			uv = vec4(0);
			break;
		}
		else if (depth < 0 && depth > -Thickness)
		{
			hit0 = 1;
			break;
		}
		else 
		{
			search0 = search1;
		}
	}

	search1 = search0 + ((search1 - search0) / 2);
	steps *= hit0;

	//Second Pass
	//If there is a hit, lets iterate around the area to find a more precise hit
	for (i = 0; i < steps; ++i)
	{
		frag = mix(startFrag.xy, endFrag.xy, search1);
		uv.xy = frag / texSize;
		positionTo = ReconstructViewPosition(DepthTexture, uv.xy, InvProjMatrix);

		viewDistance = (startView.z * endView.z) / mix(endView.z, startView.z, search1);
		depth = viewDistance - positionTo.z;

		if(depth < 0 && depth > -Thickness)
		{
			hit1 = 1;
			search1 = search0 + ((search1 - search0) / 2);
		}
		else
		{
			float temp = search1;
			search1 = search1 + ((search1 - search0) / 2);
			search0 = temp;
		}
	}


	float visibility = hit1;
	uv.ba = vec2(visibility);

	vec4 worldReflectiveDir = (InvViewMatrix * vec4(pivot, 0));
	worldReflectiveDir.z *= -1;
	vec3 reflectiveColor = mix(textureLod(EnvironmentTexture, worldReflectiveDir.xyz, 0).rgb, texture(SourceTexture, uv.xy).rgb, hit0);
	FragColor = vec4(reflectiveColor, 1);
}
