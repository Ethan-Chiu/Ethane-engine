// Tone mapping (Ethane::Renderer::Render, after the meshes): a full-screen pass from the HDR scene image
// (linear light, any brightness) to the 8-bit output the UI shows (sRGB-encoded, 0..1).
// Per pixel: scale by the exposure, map brightness into 0..1 with the tone curve, encode as sRGB.

#type vertex
#version 450 core

// One triangle covering the screen, made from the vertex index (no vertex buffer)
void main() {
	const vec2 corner = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}

#type fragment
#version 450 core

layout(set = 0, binding = 0) uniform sampler2D u_Scene; // linear HDR color, the output's size

layout(push_constant) uniform ToneMap {
	float exposure; // multiplier: 2^EV
	int curve;      // Ethane::ToneCurve: 0 = Khronos PBR Neutral
} u_ToneMap;

layout(location = 0) out vec4 outColor;

// Khronos PBR Neutral (https://github.com/KhronosGroup/ToneMapping): colors below ~0.76 pass through
// almost unchanged, so a material's base color looks like what was authored; brighter ones are compressed
// toward 1 and desaturated a little, so highlights roll off instead of clipping.
vec3 PbrNeutral(vec3 color) {
	const float startCompression = 0.8 - 0.04;
	const float desaturation = 0.15;

	// a small offset that darkens the deepest shadows slightly (it undoes the 4% Fresnel floor of dielectrics)
	float x = min(color.r, min(color.g, color.b));
	float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
	color -= offset;

	float peak = max(color.r, max(color.g, color.b));
	if (peak < startCompression)
		return color;

	// the brightest channel's new value, approaching 1 as peak grows; the others are scaled by the same factor
	const float d = 1.0 - startCompression;
	float newPeak = 1.0 - d * d / (peak + d - startCompression);
	color *= newPeak / peak;

	// the more it was compressed, the more it moves toward white
	float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
	return mix(color, vec3(newPeak), g);
}

// Linear 0..1 -> sRGB encoding (the curve displays expect for 8-bit values)
vec3 EncodeSrgb(vec3 linear) {
	linear = clamp(linear, 0.0, 1.0);
	return mix(linear * 12.92, 1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, linear));
}

void main() {
	vec3 hdr = texelFetch(u_Scene, ivec2(gl_FragCoord.xy), 0).rgb * u_ToneMap.exposure;
	vec3 mapped = PbrNeutral(max(hdr, vec3(0.0)));
	outColor = vec4(EncodeSrgb(mapped), 1.0);
}
