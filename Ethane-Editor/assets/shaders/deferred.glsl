#type vertex
#version 450 core

layout (location = 0) out vec2 outUV;

void main() 
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}





#type fragment
#version 450 core

struct DirectionalLight {
    vec4 color;
    vec3 direction;
};

layout(std140, set = 0, binding = 1) uniform GlobalUBO{
	mat4 viewproj;
	vec4 ambientColor;
	vec3 viewPosition;
} u_GlobalUBO;

layout (set = 1, binding = 1) uniform sampler2D samplerPosition;
layout (set = 1, binding = 2) uniform sampler2D samplerNormal;
layout (set = 1, binding = 3) uniform sampler2D samplerAlbedo;

// layout(set = 1, binding = 4) uniform LightUBO {
//    DirectionalLight dirLight;
    // point_light p_lights[MAX_POINT_LIGHTS];
    // int num_p_lights;
// } u_LightUBO;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;



DirectionalLight test_dir_light = {
	vec4(0.8, 0.8, 0.8, 1.0),
	vec3(-0.57735, -0.57735, -0.57735)
};

vec4 calculate_directional_light(DirectionalLight dir_light, vec3 normal, vec4 diffuse, vec3 view_direction);

void main() 
{
	// Get G-Buffer values
	vec3 frag_pos = texture(samplerPosition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);

	#define lightCount 6
	#define ambient 0.5
	
	// Ambient part
	vec4 frag_color = vec4(albedo.rgb * ambient, 1.0);
	
	// for(int i = 0; i < lightCount; ++i)
	// {
	// 	// Vector to light
	// 	vec3 L = ubo.lights[i].position.xyz - fragPos;
	// 	// Distance from light to fragment position
	// 	float dist = length(L);
	// 
	// 	// Viewer to fragment
	// 	vec3 V = ubo.viewPos.xyz - fragPos;
	// 	V = normalize(V);
	// 	
	// 	//if(dist < ubo.lights[i].radius)
	// 	{
	// 		// Light to fragment
	// 		L = normalize(L);
	// 
	// 		// Attenuation
	// 		float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);
	// 
	// 		// Diffuse part
	// 		vec3 N = normalize(normal);
	// 		float NdotL = max(0.0, dot(N, L));
	// 		vec3 diff = ubo.lights[i].color * albedo.rgb * NdotL * atten;
	// 
	// 		// Specular part
	// 		// Specular map values are stored in alpha of albedo mrt
	// 		vec3 R = reflect(-L, N);
	// 		float NdotR = max(0.0, dot(R, V));
	// 		vec3 spec = ubo.lights[i].color * albedo.a * pow(NdotR, 16.0) * atten;
	// 
	// 		fragcolor += diff + spec;	
	// 	}	
	// }    	
   
	vec3 view_direction = normalize(u_GlobalUBO.viewPosition - frag_pos);
	frag_color = calculate_directional_light(test_dir_light, normal, albedo, view_direction);

	outFragcolor = frag_color;	
}

#define SHININESS 0.3
#define DIFFUSE_COLOR vec4(0.8, 0.8, 0.8, 1.0)

vec4 calculate_directional_light(DirectionalLight dir_light, vec3 normal, vec4 diffuse, vec3 view_direction)
{
	float diffuse_dir_factor = max(dot(normal, -dir_light.direction), 0.0);

	vec3 half_direction = normalize(view_direction - dir_light.direction);
	float specular_dir_factor = pow(max(dot(half_direction, normal), 0.0), 0.3);
	
	vec4 ambient_color = vec4(vec3(u_GlobalUBO.ambientColor * DIFFUSE_COLOR), diffuse.a);
	vec4 diffuse_color = vec4(vec3(dir_light.color * diffuse_dir_factor), diffuse.a);
	vec4 specular_color = vec4(vec3(dir_light.color * specular_dir_factor), diffuse.a);

	ambient_color *= diffuse;
	diffuse_color *= diffuse;

	return (ambient_color + diffuse_color);
}