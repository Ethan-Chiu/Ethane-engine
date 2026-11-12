#type vertex
#version 450 core

// set 0, per frame: Ethane::Renderer
layout(set = 0, binding = 0) uniform Frame {
	mat4 viewproj;
	vec4 cameraPosition; // xyz: world space
	vec4 lightDirection; // xyz: towards the directional light
	vec4 lightRadiance;  // rgb: its color * intensity (0: no light)
} u_Frame;

// per draw, written by Mesh::Draw()
layout(push_constant) uniform Draw {
	mat4 model;
} u_Draw;

// Mesh::GetVertexLayout()
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout(location = 0) out vec3 v_WorldPosition;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec3 v_Tangent;
layout(location = 3) out vec3 v_Binormal;
layout(location = 4) out vec2 v_TexCoord;

void main() {
	vec4 world = u_Draw.model * vec4(a_Position, 1.0);
	gl_Position = u_Frame.viewproj * world;
	v_WorldPosition = world.xyz;
	// inverse-transpose for the normal (correct under non-uniform scale); tangents follow the surface
	v_Normal = transpose(inverse(mat3(u_Draw.model))) * a_Normal;
	v_Tangent = mat3(u_Draw.model) * a_Tangent;
	v_Binormal = mat3(u_Draw.model) * a_Binormal;
	v_TexCoord = a_TexCoord;
}


#type fragment
#version 450 core

layout(set = 0, binding = 0) uniform Frame {
	mat4 viewproj;
	vec4 cameraPosition; // xyz: world space
	vec4 lightDirection; // xyz: towards the directional light
	vec4 lightRadiance;  // rgb: its color * intensity (0: no light)
} u_Frame;

// set 1: Ethane::Material
layout(set = 1, binding = 0) uniform MaterialUniforms {
	vec4 BaseColor;
	float Metallic;
	float Roughness;
} u_Material;
layout(set = 1, binding = 1) uniform sampler2D u_BaseColorMap;
layout(set = 1, binding = 2) uniform sampler2D u_NormalMap;
layout(set = 1, binding = 3) uniform sampler2D u_MetallicRoughnessMap; // glTF: G = roughness, B = metallic

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec3 v_Tangent;
layout(location = 3) in vec3 v_Binormal;
layout(location = 4) in vec2 v_TexCoord;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265;

vec3 SurfaceNormal() {
	vec3 n = normalize(v_Normal);
	// meshes without UVs have no tangent frame: keep the vertex normal
	if (dot(v_Tangent, v_Tangent) < 1e-8 || dot(v_Binormal, v_Binormal) < 1e-8)
		return n;
	vec3 t = normalize(v_Tangent - n * dot(n, v_Tangent)); // re-orthogonalize after interpolation
	vec3 b = normalize(v_Binormal);
	vec3 tangentNormal = texture(u_NormalMap, v_TexCoord).xyz * 2.0 - 1.0;
	return normalize(mat3(t, b, n) * tangentNormal);
}

void main() {
	vec4 base = u_Material.BaseColor * texture(u_BaseColorMap, v_TexCoord);
	vec3 mr = texture(u_MetallicRoughnessMap, v_TexCoord).rgb;
	float roughness = clamp(u_Material.Roughness * mr.g, 0.05, 1.0);
	float metallic = clamp(u_Material.Metallic * mr.b, 0.0, 1.0);

	vec3 n = SurfaceNormal();
	vec3 l = u_Frame.lightDirection.xyz;
	vec3 v = normalize(u_Frame.cameraPosition.xyz - v_WorldPosition);
	vec3 h = normalize(l + v);
	float nDotL = max(dot(n, l), 0.0);

	// metals tint their reflection and have no diffuse; dielectrics reflect ~4% white
	vec3 diffuseColor = base.rgb * (1.0 - metallic);
	vec3 specularColor = mix(vec3(0.04), base.rgb, metallic);

	// normalized Blinn-Phong, exponent from roughness
	float shininess = max(2.0 / pow(roughness, 4.0) - 2.0, 1.0);
	float specular = pow(max(dot(n, h), 0.0), shininess) * (shininess + 8.0) / (8.0 * PI);

	// hemisphere ambient: lighter from above
	vec3 ambient = mix(vec3(0.10, 0.09, 0.08), vec3(0.22, 0.24, 0.28), 0.5 + 0.5 * n.y);
	vec3 color = ambient * (diffuseColor + specularColor * 0.5)
	           + (diffuseColor + specularColor * specular) * nDotL * u_Frame.lightRadiance.rgb;
	outColor = vec4(color, 1.0);
}
