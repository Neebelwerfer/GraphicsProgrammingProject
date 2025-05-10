//Inputs
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform sampler2D DepthTexture;
uniform sampler2D SourceTexture;
uniform sampler2D NormalTexture;
uniform sampler2D ReflectiveTexture;
uniform sampler2D BlurReflectiveTexture;
uniform sampler2D SpecularTexture;

uniform mat4 InvViewMatrix;
uniform mat4 InvProjMatrix;

void main()
{
	vec3 position = ReconstructViewPosition(DepthTexture, TexCoord, InvProjMatrix);
	vec3 normal = GetImplicitNormal(texture(NormalTexture, TexCoord).xy);

	// Compute view vector in view space
	vec3 viewDir = GetDirection(position, vec3(0));

	// Convert position, normal and view vector to world space
	position = (InvViewMatrix * vec4(position, 1)).xyz;
	normal = (InvViewMatrix * vec4(normal, 0)).xyz;
	viewDir = (InvViewMatrix * vec4(viewDir, 0)).xyz;

	vec4 reflectiveColor = texture(ReflectiveTexture, TexCoord);
	vec4 blurReflectiveColor = texture(BlurReflectiveTexture, TexCoord); 
	vec4 specular = texture(SpecularTexture, TexCoord);

	float ambientOcclusion = specular.x;
	float roughness = specular.y;
	float ssrVisibility = reflectiveColor.a;

	// Set surface material data
	SurfaceData data;
	data.normal = normal;
	data.albedo = texture(SourceTexture, TexCoord).rgb;
	data.ambientOcclusion = specular.x;
	data.roughness = specular.y;
	data.metalness = specular.z;

	float ssrBlend = smoothstep(0.05, 0.2, ssrVisibility);
	vec3 srrMix = mix(reflectiveColor, blurReflectiveColor, roughness).rgb ;
	vec3 specularColor = srrMix;
	//specularColor *= GeometrySmith(normal, reflectionDir, viewDir, roughness);

	vec3 diffuseColor = GetAlbedo(data);

	vec3 lightning = CombineIndirectLighting(diffuseColor, specularColor, data, viewDir);

	FragColor = vec4(specularColor, 1);
	FragColor = vec4(diffuseColor, 1);
	FragColor += vec4(lightning, 1);

	vec4 reflection = mix(reflectiveColor, blurReflectiveColor, specular.y) * specular.x;

	FragColor = texture(SourceTexture, TexCoord);
	FragColor.rgb = mix(FragColor.rgb, srrMix.rgb * data.ambientOcclusion, clamp(ssrVisibility, 0.0, 1.0));
}
