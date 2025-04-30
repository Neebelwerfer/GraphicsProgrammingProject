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

vec2 FlowUV(vec2 uv)
{
	return uv + ElapsedTime * 0.01;	
}

void main()
{
	vec2 uv = FlowUV(TexCoord);
	FragAlbedo = vec4(Color.rgb * texture(ColorTexture, uv).rgb, 1);

	vec3 viewNormal = SampleNormalMap(NormalTexture, uv, normalize(ViewNormal), normalize(ViewTangent), normalize(ViewBitangent));
	FragNormal = viewNormal.xy;

	FragOthers = waterSpecular;
}
