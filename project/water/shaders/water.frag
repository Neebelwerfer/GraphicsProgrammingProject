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
uniform sampler2D ColorTexture;
uniform sampler2D NormalTexture;
uniform float ElapsedTime;

vec4 waterSpecular = vec4(0.1f, 0.0f, 0.0f, 0.0f);

void main()
{
	vec2 uv = vec2(TexCoord.x + ElapsedTime, TexCoord.y + ElapsedTime);
	FragAlbedo = vec4(Color.rgb * texture(ColorTexture, uv).rgb, 1);

	vec3 viewNormal = SampleNormalMap(NormalTexture, uv, normalize(ViewNormal), normalize(ViewTangent), normalize(ViewBitangent));
	FragNormal = ViewNormal.xy;

	FragOthers = waterSpecular;
}
