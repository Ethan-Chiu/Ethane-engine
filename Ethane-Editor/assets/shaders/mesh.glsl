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
	vec4 lightRadiance;  // rgb: its color * intensity, linear (0: no light)
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

// --- Lighting (all in linear light) ------------------------------------------------------------------
// A metallic-roughness microfacet model (the glTF / Filament / Unreal one): GGX distribution, height-
// correlated Smith visibility, Schlick Fresnel; Lambert diffuse. Lit by the directional light, and by an
// environment: an analytic sky (zenith / horizon / ground colors) standing in for an environment map, so
// surfaces (metals above all, which only reflect) have something to reflect.

// Sky radiance arriving from direction `d` (world space, unit)
vec3 Sky(vec3 d) {
	const vec3 zenith  = vec3(0.20, 0.27, 0.42);
	const vec3 horizon = vec3(0.46, 0.46, 0.48);
	const vec3 ground  = vec3(0.07, 0.065, 0.06);
	return d.y >= 0.0 ? mix(horizon, zenith, sqrt(d.y))
	                  : mix(horizon * 0.45, ground, pow(-d.y, 0.4));
}

// The sky's light averaged over the hemisphere around normal `n` (diffuse irradiance / PI): a white matte
// surface facing `n` reflects this much
vec3 SkyIrradiance(vec3 n) {
	const vec3 above = vec3(0.32, 0.35, 0.43); // the upper hemisphere's average
	const vec3 below = vec3(0.11, 0.10, 0.095);
	return mix(below, above, 0.5 + 0.5 * n.y);
}

// GGX normal distribution; a = roughness^2
float DistributionGGX(float NoH, float a) {
	float a2 = a * a;
	float d = NoH * NoH * (a2 - 1.0) + 1.0;
	return a2 / (PI * d * d);
}

// Height-correlated Smith visibility (geometry term / (4 NoV NoL))
float VisibilitySmithGGX(float NoV, float NoL, float a) {
	float a2 = a * a;
	float v = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
	float l = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
	return 0.5 / max(v + l, 1e-5);
}

vec3 FresnelSchlick(float cosTheta, vec3 f0) {
	return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

// How much of the environment a surface reflects toward the eye, integrated over its lobe (Karis's fit of
// the split-sum lookup table, "Physically Based Shading on Mobile")
vec3 EnvironmentBRDF(vec3 f0, float roughness, float NoV) {
	const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
	const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
	vec4 r = roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	vec2 ab = vec2(-1.04, 1.04) * a004 + r.zw;
	return f0 * ab.x + ab.y;
}

void main() {
	// all colors here are linear light: the base color was converted on the CPU, the map is an sRGB image
	// decoded by the sampler; the result goes to an HDR image, tone mapped afterwards (tonemap.glsl)
	vec4 base = u_Material.BaseColor * texture(u_BaseColorMap, v_TexCoord);
	vec3 mr = texture(u_MetallicRoughnessMap, v_TexCoord).rgb;
	float roughness = clamp(u_Material.Roughness * mr.g, 0.045, 1.0); // perceptual roughness
	float metallic = clamp(u_Material.Metallic * mr.b, 0.0, 1.0);
	float a = roughness * roughness;

	vec3 n = SurfaceNormal();
	vec3 v = normalize(u_Frame.cameraPosition.xyz - v_WorldPosition);
	float NoV = clamp(dot(n, v), 1e-4, 1.0);

	// metals reflect in their color and have no diffuse; dielectrics reflect ~4%, uncolored
	vec3 diffuseColor = base.rgb * (1.0 - metallic);
	vec3 f0 = mix(vec3(0.04), base.rgb, metallic);

	// The directional light. Its radiance * PI is the illuminance it brings, so a white matte surface facing
	// it reflects exactly its intensity.
	vec3 color = vec3(0.0);
	vec3 l = u_Frame.lightDirection.xyz;
	float NoL = dot(n, l);
	if (NoL > 0.0 && dot(u_Frame.lightRadiance.rgb, vec3(1.0)) > 0.0) {
		vec3 h = normalize(l + v);
		float NoH = clamp(dot(n, h), 0.0, 1.0);
		float VoH = clamp(dot(v, h), 0.0, 1.0);
		vec3 F = FresnelSchlick(VoH, f0);
		vec3 specular = DistributionGGX(NoH, a) * VisibilitySmithGGX(NoV, NoL, a) * F;
		vec3 diffuse = (1.0 - F) * diffuseColor / PI;
		color += (diffuse + specular) * (u_Frame.lightRadiance.rgb * PI) * NoL;
	}

	// The environment: diffuse light from the sky around the normal; the reflection along the mirror
	// direction, blurred toward the hemisphere's average as roughness grows
	vec3 envBRDF = EnvironmentBRDF(f0, roughness, NoV);
	vec3 r = reflect(-v, n);
	vec3 reflected = mix(Sky(r), SkyIrradiance(r), roughness * roughness);
	color += diffuseColor * (1.0 - envBRDF) * SkyIrradiance(n) + envBRDF * reflected;

	outColor = vec4(color, 1.0);
}
