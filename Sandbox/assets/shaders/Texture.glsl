#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in float a_TilingFactor;

layout(binding = 0) uniform UniformBufferObject{
	mat4 viewproj;
} ubo;

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec4 v_Color;
layout(location = 2) out float v_TexIndex;
layout(location = 3) out float v_TilingFactor;

void main()
{
	v_Color = a_Color;
	v_TexCoord = a_TexCoord;
	v_TexIndex = a_TexIndex;
	v_TilingFactor = a_TilingFactor;
	gl_Position = ubo.viewproj * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec4 v_Color;
layout(location = 2) in float v_TexIndex;
layout(location = 3) in float v_TilingFactor;

// uniform sampler2D u_Textures[32];
layout(binding = 1) uniform sampler2D u_Textures[32];

void main()
{
	color = texture(u_Textures[int(TexIndex)], v_TexCoord * v_TilingFactor) * v_Color;
}