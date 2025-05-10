//Inputs
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform sampler2D SourceTexture;
uniform sampler2D ReflectiveTexture;
uniform sampler2D BlurReflectiveTexture;
uniform sampler2D SpecularTexture;

void main()
{
	vec4 reflectiveColor = texture(ReflectiveTexture, TexCoord);
	vec4 blurReflectiveColor = texture(BlurReflectiveTexture, TexCoord); 
	vec4 specular = texture(SpecularTexture, TexCoord);

	float specularAmount = dot(specular.rgb, vec3(1)) / 3;

	vec4 reflection = mix(reflectiveColor, blurReflectiveColor, specular.y) * specular.x;

	FragColor = texture(SourceTexture, TexCoord);
	FragColor.rgb = mix(FragColor.rgb, reflection.rgb, clamp(reflection.a, 0.0, 1.0));
}
