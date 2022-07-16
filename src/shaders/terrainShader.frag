#version 450

layout(set=1, binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragViewDir;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	const vec3 diffColor = texture(texSampler, fragTexCoord).rgb;
	const vec3 L = normalize(vec3(-5.0f, 2.0f, -2.5f));
	
	vec3 N = normalize(fragNorm);
	vec3 R = -reflect(L, N);
	vec3 V = normalize(fragViewDir);
	
	// Lambert diffuse
	vec3 diffuse = diffColor * max(dot(N, L), 0.0f) * 1.5f;
	// Hemispheric ambient
	vec3 ambient = vec3(0.3f, 0.3f, 0.3f) * diffColor;

	outColor = vec4(clamp(ambient + diffuse, vec3(0.0f), vec3(1.0f)), 1.0f);
}
