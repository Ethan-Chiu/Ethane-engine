// Selection outline, step 2 (Ethane::Renderer::RenderOutline): a full-screen pass into a transparent
// overlay. A pixel outside the highlighted silhouette but within `width` pixels of it gets the outline
// color: solid where the highlighted surface there is in front of the scene, faded where it's hidden.

#type vertex
#version 450 core

// One triangle covering the screen, made from the vertex index (no vertex buffer)
void main() {
	const vec2 corner = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}

#type fragment
#version 450 core

layout(set = 0, binding = 0) uniform sampler2D u_Mask;       // highlighted depth, 2.0 where there's none
layout(set = 0, binding = 1) uniform sampler2D u_SceneDepth; // the scene's depth image (the renderer's HDR target)

layout(push_constant) uniform Outline {
	vec4 color;
	float hiddenAlpha; // opacity factor where the highlighted surface is hidden
	int width;         // in pixels
} u_Outline;

layout(location = 0) out vec4 outColor;

void main() {
	const ivec2 pixel = ivec2(gl_FragCoord.xy);
	if (texelFetch(u_Mask, pixel, 0).r <= 1.0) {
		discard; // on the silhouette itself: the outline goes around it
	}

	const ivec2 last = textureSize(u_Mask, 0) - 1;
	const int width = u_Outline.width;
	bool near = false;
	bool visible = false;
	for (int dy = -width; dy <= width; dy++) {
		for (int dx = -width; dx <= width; dx++) {
			if (dx * dx + dy * dy > width * width) {
				continue; // round corners
			}
			const ivec2 p = clamp(pixel + ivec2(dx, dy), ivec2(0), last);
			const float depth = texelFetch(u_Mask, p, 0).r;
			if (depth <= 1.0) {
				near = true;
				visible = visible || depth <= texelFetch(u_SceneDepth, p, 0).r + 1e-5;
			}
		}
	}
	if (!near) {
		discard;
	}
	outColor = visible ? u_Outline.color : vec4(u_Outline.color.rgb, u_Outline.color.a * u_Outline.hiddenAlpha);
}
