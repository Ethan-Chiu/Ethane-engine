// Selection outline, step 1 (Ethane::Renderer::RenderOutline): the highlighted meshes, each pixel storing
// its depth, into an R32F image cleared to 2.0 ("nothing here"; depths are 0..1). A depth buffer keeps the
// nearest. The outline pass compares these depths with the scene's.

#type vertex
#version 450 core

layout(set = 0, binding = 0) uniform Mask {
	mat4 viewproj; // the scene's
} u_Mask;

// model: written by Mesh::Draw() at offset 0
layout(push_constant) uniform Draw {
	mat4 model;
} u_Draw;

// Mesh::GetVertexLayout(); only the position is needed
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

void main() {
	gl_Position = u_Mask.viewproj * u_Draw.model * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out float outDepth;

void main() {
	outDepth = gl_FragCoord.z; // same projection as the scene: comparable with its depth image
}
