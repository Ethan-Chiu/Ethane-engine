#type vertex
#version 450 core

// per frame
layout(set = 0, binding = 0) uniform Camera {
	mat4 viewproj;
} u_Camera;

// per draw: model is written by Mesh::Draw() (offset 0), color by the caller (offset 64)
layout(push_constant) uniform Draw {
	mat4 model;
	vec4 color;
} u_Draw;

// Mesh::GetVertexLayout()
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout(location = 0) out vec3 v_Normal;

void main() {
	gl_Position = u_Camera.viewproj * u_Draw.model * vec4(a_Position, 1.0);
	// inverse-transpose: correct for non-uniform scale too (model files often carry one)
	v_Normal = transpose(inverse(mat3(u_Draw.model))) * a_Normal;
}


#type fragment
#version 450 core

layout(push_constant) uniform Draw {
	mat4 model;
	vec4 color;
} u_Draw;

layout(location = 0) in vec3 v_Normal;

layout(location = 0) out vec4 outColor;

void main() {
	const vec3 lightDir = normalize(vec3(0.4, 0.8, 0.6));
	vec3 n = normalize(v_Normal);
	float diffuse = max(dot(n, lightDir), 0.0);
	float sky = 0.5 + 0.5 * n.y; // hemisphere ambient: lighter from above
	vec3 ambient = mix(vec3(0.10, 0.09, 0.08), vec3(0.22, 0.24, 0.28), sky);
	outColor = vec4(u_Draw.color.rgb * (ambient + 0.8 * diffuse), 1.0);
}
