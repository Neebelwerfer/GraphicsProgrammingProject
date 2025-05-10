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
uniform float HeightScale;
uniform float HeightScaleModulated;

//Visual Properties
uniform float AmbientOcclusion;
uniform float Roughness;
uniform float Metalness;

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
	vec4 waterSpecular = vec4(AmbientOcclusion, Roughness, Metalness, 0.0f);

	//Flow vector describing direction of flow
	vec3 flowVector = texture(FlowTexture, TexCoord).rgb;
	float noise = texture(FlowTexture, TexCoord).a;
	float time = ElapsedTime * Speed + noise;
	flowVector.xy *= 2 - 1;
	flowVector *= FlowStrength;

	//Get uvs based on 2 offset phases
	vec3 uvwA = FlowUVW(TexCoord, flowVector.xy, Jump, FlowOffset, Tiling, time, false);
	vec3 uvwB = FlowUVW(TexCoord, flowVector.xy, Jump, FlowOffset, Tiling, time, true);
	
	//Get combined texture colour based on the 2 offset phases
	vec3 texA = texture(ColorTexture, uvwA.rg).rgb * uvwA.z; 
	vec3 texB = texture(ColorTexture, uvwB.rg).rgb * uvwB.z;

	//The height scale for our waves
	float finalHeightScale = flowVector.z * HeightScaleModulated + HeightScale;
	
	//Get the normal based on the 2 offset phases
	vec3 normalA = SampleNormalMapScaled(NormalTexture, uvwA.xy, normalize(ViewNormal), normalize(ViewTangent), normalize(ViewBitangent), -finalHeightScale);
	vec3 normalB = SampleNormalMapScaled(NormalTexture, uvwB.xy, normalize(ViewNormal), normalize(ViewTangent), normalize(ViewBitangent), -finalHeightScale);
	normalA *= uvwA.z;
	normalB *= uvwB.z;

	vec3 combinedViewSpaceNormal = normalize(normalA + normalB);

	//Get the normal from the derivative map
	vec3 dhA = SampleDerivativeMap(DerivativeMap, uvwA.xy) * (uvwA.z * finalHeightScale);
	vec3 dhB = SampleDerivativeMap(DerivativeMap, uvwB.xy) * (uvwB.z * finalHeightScale);

	vec3 derivedNormal = normalize(vec3(-(dhA.xy + dhB.xy), 1));
	derivedNormal.y *= -1;
	derivedNormal = TransformTangentNormal(derivedNormal.xyz, normalize(ViewNormal), normalize(ViewTangent));

	FragAlbedo = vec4(Color * (texA + texB), 1);
	FragNormal = normalize(derivedNormal).xy;
	FragNormal = normalize(combinedViewSpaceNormal).xy;
	FragOthers = waterSpecular;
}
