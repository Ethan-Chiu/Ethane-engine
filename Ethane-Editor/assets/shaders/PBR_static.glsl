#type vertex
#version 450 core

layout(std140, set = 0, binding = 0) uniform UniformBufferObject{
	mat4 viewproj;
	vec4 ambient_color;
	vec3 view_position;
} u_UBO;

layout(push_constant) uniform TransformUniform
{
	mat4 transform;
} u_TransformUniform;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;
// layout(location = 5) in int a_EntityID;

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
	vec4 AmbientColor;
	vec3 ViewPosition;
};

layout(location = 0) out VertexOutput Output;
// layout(location = 6) out flat int v_EntityID;

void main()
{
	Output.WorldPosition = vec3(u_TransformUniform.transform * vec4(a_Position, 1.0)); 
	Output.Normal = mat3(u_TransformUniform.transform) * a_Normal;
	Output.TexCoord = vec2(a_TexCoord.x, 1.0- a_TexCoord.y);
	Output.WorldNormals = mat3(a_Tangent, a_Binormal, a_Normal); // mat3(u_TransformUniform.Transform) * 
	Output.WorldTransform = mat3(1.0);// mat3(u_TransformUniform.Transform);
	Output.Binormal = a_Binormal;
	Output.AmbientColor = u_UBO.ambient_color;
	Output.ViewPosition = u_UBO.view_position;
	// v_EntityID = a_EntityID;

	gl_Position = u_UBO.viewproj * u_TransformUniform.transform * vec4(a_Position, 1.0);
}




#type fragment
#version 450 core

layout(set = 1, binding = 0) uniform LocalUBO {
    vec4 diffuse_color;
	float shininess;
} u_LocalUBO;

layout(set = 1, binding = 1) uniform sampler2D u_DiffuseSampler;
layout(set = 1, binding = 2) uniform sampler2D u_SpecularSampler;

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
	vec4 AmbientColor;
	vec3 ViewPosition;
};

layout(location = 0) in VertexOutput Input;
// layout(location = 6) in flat int v_EntityID;

layout(location = 0) out vec4 OutColor;



const float PI = 3.141593;
const float Epsilon = 0.00001;


struct DirectionalLight
{
	vec3 Direction;
	vec4 Color;
};

DirectionalLight test_dir_light = {
	vec3(-0.57735, -0.57735, -0.57735),
	vec4(0.8, 0.8, 0.8, 1.0)
};

vec4 calculate_directional_light(DirectionalLight dir_light, vec3 normal, vec3 view_direction);

void main()
{
	vec3 view_direction = normalize(Input.ViewPosition - Input.WorldPosition);

	OutColor = calculate_directional_light(test_dir_light, Input.Normal, view_direction);
}

vec4 calculate_directional_light(DirectionalLight dir_light, vec3 normal, vec3 view_direction)
{
	float diffuse_dir_factor = max(dot(normal, -dir_light.Direction), 0.0);

	vec3 half_direction = normalize(view_direction - dir_light.Direction);
	float specular_dir_factor = pow(max(dot(half_direction, normal), 0.0), u_LocalUBO.shininess);
	
	vec4 diffuse_texture = texture(u_DiffuseSampler, Input.TexCoord);
	vec4 ambient_color = vec4(vec3(Input.AmbientColor * u_LocalUBO.diffuse_color), diffuse_texture.a);
	vec4 diffuse_color = vec4(vec3(dir_light.Color * diffuse_dir_factor), diffuse_texture.a);
	vec4 specular_color = vec4(vec3(dir_light.Color * specular_dir_factor), diffuse_texture.a);

	ambient_color *= diffuse_texture;
	diffuse_color *= diffuse_texture;
	specular_color *= vec4(texture(u_SpecularSampler, Input.TexCoord).rgb, diffuse_color.a);

	return (ambient_color + diffuse_color + specular_color);
}