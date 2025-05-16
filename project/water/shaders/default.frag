//Inputs
in vec3 ViewNormal;
in vec3 ViewTangent;
in vec3 ViewBitangent;
in vec2 TexCoord;

//Outputs
out vec4 FragAlbedo;
out vec2 FragNormal;
out vec4 FragOthers;

//Uniforms
uniform vec3 Color;
uniform vec3 HaveTextures;
uniform sampler2D ColorTexture;
uniform sampler2D NormalTexture;
uniform sampler2D SpecularTexture;

vec4 defaultSpecular = vec4(0.0, 0.5, 0.0, 0);

void main()
{
	FragAlbedo = mix(vec4(Color, 1), vec4(Color.rgb * texture(ColorTexture, TexCoord).rgb, 1), HaveTextures.x);

	vec3 viewNormal = SampleNormalMap(NormalTexture, TexCoord, normalize(ViewNormal), normalize(ViewTangent), normalize(ViewBitangent));
	FragNormal = mix(ViewNormal.xy, viewNormal.xy, HaveTextures.y);

	FragOthers = mix(defaultSpecular, vec4(texture(SpecularTexture, TexCoord).rgb, 0), HaveTextures.z);
}
