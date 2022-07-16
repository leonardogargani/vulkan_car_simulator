#version 450

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(set = 1, binding = 0) uniform terrainUniformBufferObject {
	mat4 model;
	vec4 selector;
} tubo;

layout(location = 0) in vec3 fragViewDir;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	const vec3 diffColor = texture(texSampler, fragTexCoord).rgb;
	const vec3 L = normalize(vec3(-5.0f, 2.0f, -2.5f));
	
	const vec3 specColor = vec3(1.0f, 1.0f, 1.0f); /////
	const float specPower = 150.0f; /////
	
	vec3 N = normalize(fragNorm);
	vec3 R = -reflect(L, N);
	vec3 V = normalize(fragViewDir);
	
	vec3 specular = specColor * pow(max(dot(R, V), 0.0f), specPower); /////
	
	// Lambert diffuse
	vec3 diffuse = diffColor * max(dot(N, L), 0.0f) * 1.5f;
	// Hemispheric ambient
	vec3 ambient = vec3(0.3f, 0.3f, 0.3f) * diffColor;

	outColor = vec4(clamp(ambient + diffuse + specular * tubo.selector.x, vec3(0.0f), vec3(1.0f)), 1.0f);
}
