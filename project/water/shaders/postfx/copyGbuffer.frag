//Inputs
in vec2 TexCoord;

//Outputs
out vec4 FragAlbedo;
out vec2 FragNormal;
out vec4 FragOthers;

//Uniforms
uniform sampler2D SourceTexture;
uniform sampler2D DepthTexture;
uniform sampler2D NormalTexture;
uniform sampler2D OtherTexture;

void main()
{
	FragAlbedo = texture(SourceTexture, TexCoord);
	FragNormal = texture(NormalTexture, TexCoord).xy;
	FragOthers = texture(OtherTexture, TexCoord);
	gl_FragDepth = texture(DepthTexture, TexCoord).r;
}
