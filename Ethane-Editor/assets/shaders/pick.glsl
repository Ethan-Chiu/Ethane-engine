// Picking: draws each mesh with a number identifying its draw item (Ethane::Renderer's pick pass), into
// an R32UI target. 0 means "nothing": the target is cleared to it.

#type vertex
#version 450 core

// set 0: the pick view (the scene's view-projection narrowed to the pixel being picked)
layout(set = 0, binding = 0) uniform Pick {
	mat4 viewproj;
} u_Pick;

// model: written by Mesh::Draw() at offset 0; id: written by the renderer before each mesh
layout(push_constant) uniform Draw {
	mat4 model;
	uint id;
} u_Draw;

// Mesh::GetVertexLayout(); only the position is needed
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

void main() {
	gl_Position = u_Pick.viewproj * u_Draw.model * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(push_constant) uniform Draw {
	mat4 model;
	uint id;
} u_Draw;

layout(location = 0) out uint outId;

void main() {
	outId = u_Draw.id;
}
