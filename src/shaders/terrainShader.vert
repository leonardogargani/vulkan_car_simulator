#version 450

layout(set = 0, binding = 0) uniform globalUniformBufferObject {
	mat4 view;
	mat4 proj;
} gubo;

layout(set = 1, binding = 0) uniform terrainUniformBufferObject {
	int spotlight_on;
	mat4 model;
} tubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragViewDir;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragTexCoord;
// NEW!!!
layout(location = 3) out vec3 fragPos;


void main() {

	// NEW!!!
	fragPos = (tubo.model * vec4(pos, 1.0)).xyz;

	gl_Position = gubo.proj * gubo.view * tubo.model * vec4(pos, 1.0);
	fragViewDir = (gubo.view[3]).xyz - (tubo.model * vec4(pos,  1.0)).xyz;
	fragNorm = (tubo.model * vec4(norm, 0.0)).xyz;
	fragTexCoord = 200.0 * texCoord;
}
