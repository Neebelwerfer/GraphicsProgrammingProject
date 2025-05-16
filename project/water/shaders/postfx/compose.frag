//Inputs
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform sampler2D DepthTexture;
uniform sampler2D SourceTexture;
uniform sampler2D NormalTexture;
uniform sampler2D ReflectiveTexture;
uniform sampler2D RefractiveTexture;
uniform sampler2D BlurReflectiveTexture;
uniform sampler2D SpecularTexture;

uniform mat4 InvViewMatrix;
uniform mat4 InvProjMatrix;

// Computes the color for our ssr reflection
vec3 ComputeSSRIndirectLighting(SurfaceData data, vec3 viewDir, vec4 reflectiveColor, float ssrVisibility)
{
	vec3 reflectionDir = reflect(-viewDir, data.normal);
	vec4 blurReflectiveColor = texture(BlurReflectiveTexture, TexCoord);

	vec3 diffuseColor = blurReflectiveColor.rgb * GetAlbedo(data);

	//Blur based on visibility
	vec3 specularColor = mix(blurReflectiveColor.rgb, reflectiveColor.rgb, ssrVisibility);
	specularColor *= GeometrySmith(data.normal, reflectionDir, viewDir, data.roughness);

	// Blend SSR and EnvironmentMap reflection based on the roughness
	// This should lead to clear reflection at low roughness and nearly invisible reflection at high roughness 
	return mix(CombineIndirectLighting(diffuseColor, specularColor, data, viewDir), ComputeIndirectLighting(data, viewDir),  pow(data.roughness, 0.5f));
}

void main()
{
	// Get some geometry data so we can compute our indirect lighting properly
	vec3 position = ReconstructViewPosition(DepthTexture, TexCoord, InvProjMatrix);
	vec3 normal = GetImplicitNormal(texture(NormalTexture, TexCoord).xy);
	vec3 viewDir = GetDirection(position, vec3(0));

	// Convert position, normal and view vector to world space
	position = (InvViewMatrix * vec4(position, 1)).xyz;
	normal = (InvViewMatrix * vec4(normal, 0)).xyz;
	viewDir = (InvViewMatrix * vec4(viewDir, 0)).xyz;

	vec4 reflectiveColor = texture(ReflectiveTexture, TexCoord);
	vec4 refractiveColor = texture(RefractiveTexture, TexCoord);
	vec4 specular = texture(SpecularTexture, TexCoord);

	// This should result in 1 if the ssr pass had hit something at this pixel otherwise 0
	float ssrVisibility = reflectiveColor.a;

	// Set surface material data
	SurfaceData data;
	data.normal = normal;
	data.albedo = texture(SourceTexture, TexCoord).rgb;
	data.ambientOcclusion = specular.x;
	data.roughness = specular.y;
	data.metalness = specular.z;

	// If the ssr had no hit, we sample the environment map for reflections otherwise we sample the ssr map
	vec3 lightning = mix(ComputeIndirectLighting(data, viewDir), ComputeSSRIndirectLighting(data, viewDir, reflectiveColor, ssrVisibility), ssrVisibility > 0.0001 ? 1 : 0);

	FragColor = texture(SourceTexture, TexCoord);
	FragColor.rgb = mix(FragColor.rgb, refractiveColor.rgb, clamp(refractiveColor.a, 0, 1));
	FragColor.rgb += lightning;
}
