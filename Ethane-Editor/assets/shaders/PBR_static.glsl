#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;
// layout(location = 5) in int a_EntityID;

// replace
// layout(std140, binding = 0) uniform Camera
// {
// 	mat4 u_ViewProjection;
// };

layout(std140, set = 0, binding = 0) uniform UniformBufferObject{
	mat4 viewproj;
} ubo;

layout(push_constant) uniform TransformUniform
{
	mat4 Transform;
}u_TransformUniform;

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
};

layout(location = 0) out VertexOutput Output;
// layout(location = 6) out flat int v_EntityID;

void main()
{
	Output.WorldPosition = vec3(vec4(a_Position, 1.0)); // u_TransformUniform.Transform * 
	Output.Normal = a_Normal; // mat3(u_TransformUniform.Transform) * 
	Output.TexCoord = vec2(a_TexCoord.x, 1.0- a_TexCoord.y);
	Output.WorldNormals = mat3(a_Tangent, a_Binormal, a_Normal); // mat3(u_TransformUniform.Transform) * 
	Output.WorldTransform = mat3(1.0);// mat3(u_TransformUniform.Transform);
	Output.Binormal = a_Binormal;

	// v_EntityID = a_EntityID;

	gl_Position = ubo.viewproj * u_TransformUniform.Transform * vec4(a_Position, 1.0);
}




#type fragment
#version 450 core

const float PI = 3.141593;
const float Epsilon = 0.00001;

const int LighCount = 1;

const vec3 Fdielectric = vec3(0.04);

struct DirectionalLight
{
	vec3 Direction;
	vec3 Radiance;
	float Multiplier;
};

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
};

layout(location = 0) in VertexOutput Input;
// layout(location = 6) in flat int v_EntityID;

layout(location = 0) out vec4 color;

vec3 LightDir = vec3(1.0, 1.0, 1.0);

// vec3 IBL(vec3 F0, vec3 Lr)
// {
// 	vec3 irradiance = texture(u_EnvIrradianceTex, m_Params.Normal).rgb;
// 	vec3 F = fresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
// 	vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
// 	vec3 diffuseIBL = m_Params.Albedo * irradiance;
// 
// 	int envRadianceTexLevels = textureQueryLevels(u_EnvRadianceTex);
// 	float NoV = clamp(m_Params.NdotV, 0.0, 1.0);
// 	vec3 R = 2.0 * dot(m_Params.View, m_Params.Normal) * m_Params.Normal - m_Params.View;
// 	vec3 specularIrradiance = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_MaterialUniforms.EnvMapRotation, Lr), (m_Params.Roughness) * envRadianceTexLevels).rgb;
// 	//specularIrradiance = vec3(Convert_sRGB_FromLinear(specularIrradiance.r), Convert_sRGB_FromLinear(specularIrradiance.g), Convert_sRGB_FromLinear(specularIrradiance.b));
// 
// 	// Sample BRDF Lut, 1.0 - roughness for y-coord because texture was generated (in Sparky) for gloss model
// 	vec2 specularBRDF = texture(u_BRDFLUTTexture, vec2(m_Params.NdotV, 1.0 - m_Params.Roughness)).rg;
// 	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);
// 
// 	return kd * diffuseIBL + specularIBL;
// }

void main()
{
	// Standard PBR inputs
	// m_Params.Albedo = texture(u_AlbedoTexture, Input.TexCoord).rgb * u_MaterialUniforms.AlbedoColor;
	// float alpha = texture(u_AlbedoTexture, Input.TexCoord).a;
	// m_Params.Metalness = texture(u_MetalnessTexture, Input.TexCoord).r * u_MaterialUniforms.Metalness;
	// m_Params.Roughness = texture(u_RoughnessTexture, Input.TexCoord).r * u_MaterialUniforms.Roughness;
	// m_Params.Roughness = max(m_Params.Roughness, 0.05); // Minimum roughness of 0.05 to keep specular highlight

	// Normals (either from vertex or map)
	// m_Params.Normal = normalize(Input.Normal);
	// if (u_MaterialUniforms.UseNormalMap)
	// {
	// 	m_Params.Normal = normalize(texture(u_NormalTexture, Input.TexCoord).rgb * 2.0f - 1.0f);
	// 	m_Params.Normal = normalize(Input.WorldNormals * m_Params.Normal);
	// }


	// Specular reflection vector
	// vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;

	// Fresnel reflectance, metals use albedo
	// vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);

	// vec3 lightContribution = CalculateDirLights(F0) * shadowAmount;
	// lightContribution += CalculatePointLights(F0);
	// vec3 lightContribution += m_Params.Albedo * u_MaterialUniforms.Emission;
	// vec3 iblContribution = IBL(F0, Lr) * u_EnvironmentMapIntensity;

	// color = vec4(iblContribution + lightContribution, 1.0);

	float intensity = clamp(dot(LightDir, Input.Normal), 0.2, 1.0);
	color = vec4(1.0, 0.2, 0.3, 1) * intensity;
}
