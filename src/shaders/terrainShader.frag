#version 450

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(set = 1, binding = 0) uniform terrainUniformBufferObject {
	int spotlight_on;
	mat4 model;
} tubo;

layout(location = 0) in vec3 fragViewDir;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;
// NEW!!!
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;


// By default we had:
//  - diffuse lighting: Lambert
//  - specular lighting: Phong
//  - indirect lighting: Hemispheric oriented along the y-axis
//  - light model: one directional light
void main() {

	const vec3 objColor = texture(texSampler, fragTexCoord).rgb;

	if (tubo.spotlight_on == 1) {

		vec3 light_pos = vec3(0.0, 10.0, 0.0);
		vec3 target_pos = vec3(0.0, 0.0, 0.0);

		vec3 point_pos = fragPos;

		vec3 light_dir = (light_pos - point_pos) / length(light_pos - point_pos);
		vec3 spot_light_dir = (light_pos - target_pos) / length(light_pos - target_pos);

		float cos_outer_angle = 0.9;
		float cos_inner_angle = 1.0;
		float distance_g = 10.0;
		float exponent_beta = 0.5;

		vec3 spot_light_color = objColor
								* pow((distance_g / length(light_pos - point_pos)), exponent_beta)
								* clamp((dot(light_dir, spot_light_dir) - cos_outer_angle) / (cos_inner_angle - cos_outer_angle), 0, 1);

		vec3 N = normalize(fragNorm);
		const vec3 L = normalize(vec3(-5.0f, 2.0f, -2.5f));

		vec3 lambert_diffuse = objColor * max(dot(N, L), 0.0f) * 0.15f;


		outColor = vec4(spot_light_color + lambert_diffuse, 1.0);

	} else {

		const vec3 L = normalize(vec3(-5.0f, 2.0f, -2.5f));

		vec3 N = normalize(fragNorm);
		vec3 R = -reflect(L, N);
		vec3 V = normalize(fragViewDir);

		// Lambert diffuse
		vec3 diffuse = objColor * max(dot(N, L), 0.0f) * 1.5f;
		// Hemispheric ambient
		vec3 ambient = vec3(0.3f, 0.3f, 0.3f) * objColor;

		// NEW!!!
		vec3 light_color = vec3(1.0, 0.7, 0.7);

		outColor = vec4(clamp(ambient + diffuse, vec3(0.0f), vec3(1.0f)) * light_color, 1.0f);

	}

}
