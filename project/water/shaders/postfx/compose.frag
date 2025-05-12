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

	vec3 reflectionDir = reflect(-viewDir, data.normal);
	
	vec3 diffuseColor = blurReflectiveColor.rgb * GetAlbedo(data);

	vec3 specularColor = mix(reflectiveColor, blurReflectiveColor, roughness).rgb;
	specularColor *= GeometrySmith(normal, reflectionDir, viewDir, roughness);

	vec3 lightning = CombineIndirectLighting(diffuseColor, specularColor, data, viewDir);

	FragColor = texture(SourceTexture, TexCoord);
	FragColor += vec4(lightning, 1);
}
