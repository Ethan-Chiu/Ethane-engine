#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout(std140, set = 0, binding = 1) uniform UniformBufferObject{
	mat4 viewproj;
	vec4 ambient_color;
	vec3 view_position;
} u_UBO;

layout(push_constant) uniform TransformUniform
{
	mat4 transform;
} u_TransformUniform;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outBinormal;
layout (location = 2) out vec3 outTangent;
layout (location = 3) out vec3 outWorldPos;
layout (location = 4) out vec2 outUV;

void main() 
{
	gl_Position = u_UBO.viewproj * vec4(a_Position, 1.0);
	
	outUV = a_TexCoord;

	// Vertex position in world space
	outWorldPos = a_Position;
	
	// Normal in world space
	// mat3 mNormal = transpose(inverse(mat3(ubo.model)));
	
	outNormal = normalize(a_Normal); // mNormal * 
	outTangent = normalize(a_Tangent); // mNormal * 
	outBinormal = normalize(a_Binormal); // mNormal * 
}






#type fragment
#version 450 core

// layout (binding = 1) uniform sampler2D samplerColor;
// layout (binding = 2) uniform sampler2D samplerNormalMap;


layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inBinormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec3 inWorldPos;
layout (location = 4) in vec2 inUV;


layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

void main() 
{
	outPosition = vec4(inWorldPos, 1.0);

	// Calculate normal in tangent space
	vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangent);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
	vec3 tnorm = TBN * normalize(vec3(0.5, 0.5, 1.0) * 2.0 - vec3(1.0)); // texture(samplerNormalMap, inUV).xyz
	outNormal = vec4(N, 1.0);

	// outAlbedo = texture(samplerColor, inUV);
	outAlbedo = vec4(inUV, 0.3, 1.0);
}
