#type vertex
#version 450 core

// per frame: one camera for every draw
layout(set = 0, binding = 0) uniform Camera {
	mat4 viewproj;
} u_Camera;

// per draw
layout(push_constant) uniform Transform {
	mat4 model;
} u_Transform;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

layout(location = 0) out vec3 v_Normal;
layout(location = 1) out vec2 v_TexCoord;

void main() {
	gl_Position = u_Camera.viewproj * u_Transform.model * vec4(a_Position, 1.0);
	v_Normal = mat3(u_Transform.model) * a_Normal; // rotation + uniform scale only
	v_TexCoord = a_TexCoord;
}


#type fragment
#version 450 core

layout(set = 0, binding = 1) uniform sampler2D u_Texture;

layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec2 v_TexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	const vec3 lightDir = normalize(vec3(0.4, 0.8, 0.6));
	float diffuse = max(dot(normalize(v_Normal), lightDir), 0.0);
	vec3 albedo = texture(u_Texture, v_TexCoord).rgb;
	outColor = vec4(albedo * (0.25 + 0.75 * diffuse), 1.0);
}
