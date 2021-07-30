#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;
// layout(location = 5) in int a_EntityID;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

layout(std140, binding = 1) uniform Transform
{
	mat4 u_Transform;
};

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
};

layout(location = 0) out VertexOutput Output;
// layout(location = 6) out flat int v_EntityID;

void main()
{
	Output.WorldPosition = vec3(u_Transform * vec4(a_Position, 1.0));
	Output.Normal = mat3(u_Transform) * a_Normal;
	Output.TexCoord = vec2(a_TexCoord.x, 1.0- a_TexCoord.y);
	Output.WorldNormals = mat3(u_Transform) * mat3(a_Tangent, a_Binormal, a_Normal);
	Output.WorldTransform = mat3(u_Transform);
	Output.Binormal = a_Binormal;

	// v_EntityID = a_EntityID;

	gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}




#type fragment
#version 450 core

const float PI = 3.141593;
const float Epsilon = 0.00001;

const int LighCount = 1;

const vec3 Fdielectric = vec3(0.04);

struct Light 
{
	vec3 Direction;
	vec3 Radiance;
	float Multiplier;
};

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
};

layout(location = 0) in VertexOutput Input;
// layout(location = 6) in flat int v_EntityID;

layout(location = 0) out vec4 color;

void main()
{
	color = vec4(1.0, 0.2, 0.3, 1);
}
