//Inputs
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform sampler2D DepthTexture;
uniform sampler2D AlbedoTexture;
uniform sampler2D NormalTexture;
uniform sampler2D OthersTexture;
uniform mat4 InvViewMatrix;
uniform mat4 InvProjMatrix;
uniform int ShowType;

void main()
{
		// Extract information from g-buffers
		vec3 position = ReconstructViewPosition(DepthTexture, TexCoord, InvProjMatrix);
		vec3 albedo = texture(AlbedoTexture, TexCoord).rgb;
		vec3 normal = GetImplicitNormal(texture(NormalTexture, TexCoord).xy);
		vec4 others = texture(OthersTexture, TexCoord);

		// Compute view vector in view space
		vec3 viewDir = GetDirection(position, vec3(0));

		// Convert position, normal and view vector to world space
		position = (InvViewMatrix * vec4(position, 1)).xyz;
		normal = (InvViewMatrix * vec4(normal, 0)).xyz;
		viewDir = (InvViewMatrix * vec4(viewDir, 0)).xyz;

		// Set surface material data
		SurfaceData data;
		data.normal = normal;
		data.albedo = albedo;
		data.ambientOcclusion = others.x;
		data.roughness = others.y;
		data.metalness = others.z;

		// Compute lighting
		// No indirect ligthning since we are going to use SSR/Environment map blending later
		vec3 lighting = ComputeLighting(position, data, viewDir, false);

		// Different options for some debug visuals
		if(ShowType == 0)
			FragColor = vec4(lighting, 1.0f);
		else if (ShowType == 1)
			FragColor = vec4(albedo, 1.0f);
		else if (ShowType == 2)
			FragColor = vec4(position, 1.0f);
		else if (ShowType == 3)
			FragColor = vec4(texture(DepthTexture, TexCoord).r, 1, 1, 1.0f);
		else if (ShowType == 4)
			FragColor = vec4(normalize(normal), 1.0f);
		else if (ShowType == 5)
			FragColor = vec4(GetImplicitNormal(texture(NormalTexture, TexCoord).xy), 1.0f);
}
