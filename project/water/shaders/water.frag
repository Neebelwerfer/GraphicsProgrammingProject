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
uniform sampler2D FlowTexture;
uniform float ElapsedTime;

vec4 waterSpecular = vec4(0.1f, 0.0f, 0.0f, 0.0f);

vec3 FlowUVW(vec2 uv, vec2 flowVector, vec2 jump, float time, bool flowB)
{
	float phaseOffset = flowB ? 0.5 : 0;
	float progress = fract(time + phaseOffset);
	vec3 uvw;
	uvw.xy = uv - flowVector * progress;
	uvw.xy = (time - progress) * jump + uvw.xy;
	uvw.z = 1 - abs(1 - 2 * progress);
	return uvw;	
}

void main()
{
	vec2 Jump = vec2(0.2f, -0.2f);
	vec2 flowVector = texture(FlowTexture, TexCoord).rg * 2 - 1;
	float noise = texture(FlowTexture, TexCoord).a;
	float time = ElapsedTime + noise;

	vec3 uvwA = FlowUVW(TexCoord, flowVector, Jump, time, false);
	vec3 uvwB = FlowUVW(TexCoord, flowVector, Jump, time, true);
	vec3 texA = texture(ColorTexture, uvwA.rg).rgb * uvwA.z;
	vec3 texB = texture(ColorTexture, uvwB.rg).rgb * uvwB.z;

	//vec3 viewNormal = SampleNormalMap(NormalTexture, uvw.rg, normalize(ViewNormal), normalize(ViewTangent), normalize(ViewBitangent));

	FragAlbedo = vec4(Color * (texA + texB), 1);
	FragNormal = ViewNormal.xy;
	FragOthers = waterSpecular;
}
