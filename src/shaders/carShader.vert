#version 450

layout(set = 0, binding = 0) uniform globalUniformBufferObject {
	mat4 view;
	mat4 proj;
} gubo;

layout(set = 1, binding = 0) uniform carUniformBufferObject {
	int spotlight_on;
	int backlights_on;
	mat4 model;
	vec3 car_pos;
	vec3 car_ang;
} cubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragViewDir;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragPos;


void main() {

	fragPos = (cubo.model * vec4(pos, 1.0)).xyz;

	gl_Position = gubo.proj * gubo.view * cubo.model * vec4(pos, 1.0);
	fragViewDir = (gubo.view[3]).xyz - (cubo.model * vec4(pos,  1.0)).xyz;
	fragNorm = (cubo.model * vec4(norm, 0.0)).xyz;
	fragTexCoord = texCoord;

}
