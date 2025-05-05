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
uniform sampler2D DerivativeMap;
uniform float ElapsedTime;

//Water properties
uniform vec2 Jump;
uniform int Tiling;
uniform float Speed; 
uniform float FlowStrength;
uniform float FlowOffset;

//Hard coded Specular property that could be made to an uniform input
vec4 waterSpecular = vec4(1.33f, 0.0f, 0.0f, 0.0f);


//Calculate the offset uv coordinates based on certain flow variables and time
vec3 FlowUVW(vec2 uv, vec2 flowVector, vec2 jump, float flowOffset, float tiling, float time, bool flowB)
{
	float phaseOffset = flowB ? 0.5 : 0;
	float progress = fract(time + phaseOffset);
	vec3 uvw;
	uvw.xy = uv - flowVector * (progress + flowOffset);
	uvw.xy *= tiling;
	uvw.xy += phaseOffset;
	uvw.xy += (time - progress) * jump;
	uvw.z = 1 - abs(1 - 2 * progress);
	return uvw;	
}

void main()
{
	//Flow vector describing direction of flow
	vec2 flowVector = texture(FlowTexture, TexCoord).rg * 2.0f - vec2(1);
	flowVector *= FlowStrength;
	float noise = texture(FlowTexture, TexCoord).a;
	float time = ElapsedTime * Speed + noise;

	//Get uvs based on 2 offset phases
	vec3 uvwA = FlowUVW(TexCoord, flowVector, Jump, FlowOffset, Tiling, time, false);
	vec3 uvwB = FlowUVW(TexCoord, flowVector, Jump, FlowOffset, Tiling, time, true);
	
	//Get combined texture colour based on the 2 offset phases
	vec3 texA = texture(ColorTexture, uvwA.rg).rgb * uvwA.z; 
	vec3 texB = texture(ColorTexture, uvwB.rg).rgb * uvwB.z;

	//Get the normal from the derivative map
	vec3 dhA = SampleDerivativeMap(DerivativeMap, uvwA.xy) * uvwA.z;
	vec3 dhB = SampleDerivativeMap(DerivativeMap, uvwB.xy) * uvwB.z;
	vec3 derivedNormal = normalize(vec3(-(dhA.xy + dhB.xy), 1));
	derivedNormal = TransformTangentNormal(derivedNormal.xyz, normalize(ViewNormal), normalize(ViewTangent));

	FragAlbedo = vec4(Color * (texA + texB), 1);
	FragNormal = normalize(derivedNormal).xy;
	FragOthers = waterSpecular;
}
