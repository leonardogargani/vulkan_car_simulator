#version 450

#define DIFFUSE_WEIGHT 0.15f
#define SPECULAR_WEIGHT 0.2f
#define AMBIENT_WEIGHT 0.05f


layout(set=1, binding = 1) uniform sampler2D texSampler;

layout(set = 1, binding = 0) uniform carUniformBufferObject {
	int spotlight_on;
	int backlights_on;
	mat4 model;
	vec3 car_pos;
	vec3 car_ang;
} cubo;

layout(location = 0) in vec3 fragViewDir;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;


vec3 rotate_pos(vec3 pos, vec3 ang, vec3 offset) {

	mat3 rotate_yaw = mat3(cos(ang.x), 0.0, -sin(ang.x),
							0.0, 1.0, 0.0,
							sin(ang.x), 0.0, cos(ang.x));

	mat3 rotate_pitch = mat3(cos(ang.y), sin(ang.y), 0.0,
							-sin(ang.y), cos(ang.y), 0.0,
							0.0, 0.0, 1.0);

	mat3 rotate_roll = mat3(1.0, 0.0, 0.0,
							0.0, cos(ang.z), sin(ang.z),
							0.0, -sin(ang.z), cos(ang.z));

	return pos + rotate_roll * rotate_yaw * rotate_pitch * offset;

}


void main() {

	const vec3 obj_color = texture(texSampler, fragTexCoord).rgb;
	vec3 light_color = vec3(1.0, 0.6, 0.6);

	if (cubo.spotlight_on == 1) {

		vec3 light_pos = vec3(0.0, 10.0, 0.0);
		vec3 target_pos = vec3(0.0, 0.0, 0.0);

		vec3 point_pos = fragPos;

		vec3 light_dir = (light_pos - point_pos) / length(light_pos - point_pos);
		vec3 spot_light_dir = (light_pos - target_pos) / length(light_pos - target_pos);

		float cos_outer_angle = 0.9;
		float cos_inner_angle = 1.0;
		float distance_g = 10.0;
		float exponent_beta = 0.5;

		vec3 spot_light_color = obj_color
								* pow((distance_g / length(light_pos - point_pos)), exponent_beta)
								* clamp((dot(light_dir, spot_light_dir) - cos_outer_angle) / (cos_inner_angle - cos_outer_angle), 0, 1);

		vec3 N = normalize(fragNorm);
		const vec3 L = normalize(vec3(-5.0f, 2.0f, -2.5f));

		vec3 lambert_diffuse = obj_color * max(dot(N, L), 0.0f) * DIFFUSE_WEIGHT;


		const vec3 specColor = vec3(1.0f, 1.0f, 1.0f);
		const float specPower = 150.0f;
		vec3 R = -reflect(spot_light_dir, N);
		vec3 V = normalize(fragViewDir);
		vec3 phong_specular = specColor * pow(max(dot(R, V), 0.0f), specPower) * SPECULAR_WEIGHT;


		vec3 ambient = obj_color * AMBIENT_WEIGHT;


		outColor = vec4((spot_light_color + lambert_diffuse + phong_specular + ambient) * light_color, 1.0);

	} else {

		const vec3 specColor = vec3(1.0f, 1.0f, 1.0f);
		const float specPower = 150.0f;
		const vec3 L = normalize(vec3(-5.0f, 3.0f, -2.5f));

		vec3 N = normalize(fragNorm);
		vec3 R = -reflect(L, N);
		vec3 V = normalize(fragViewDir);

		vec3 lambert_diffuse = obj_color * max(dot(N, L), 0.0f) * 1.5f;
		vec3 phong_specular = specColor * pow(max(dot(R, V), 0.0f), specPower);
		vec3 ambient = vec3(0.3f, 0.3f, 0.3f) * obj_color;

		outColor = vec4(clamp(ambient + lambert_diffuse + phong_specular, vec3(0.0f), vec3(1.0f)) * light_color, 1.0f);

	}

	if (cubo.backlights_on == 1) {

		vec3 point_pos = fragPos;

		vec3 left_backlight_offset = vec3(3.4, 1.32, 0.94);
		vec3 right_backlight_offset = vec3(3.4, 1.32, -0.94);
		vec3 left_target_offset = vec3(-0.2, 0.0, 0.0);
		vec3 right_target_offset = vec3(-0.2, 0.0, 0.0);


		vec3 left_light_pos = rotate_pos(cubo.car_pos, cubo.car_ang, left_backlight_offset);
		vec3 left_target_pos = rotate_pos(left_light_pos, cubo.car_ang, left_target_offset);

		vec3 right_light_pos = rotate_pos(cubo.car_pos, cubo.car_ang, right_backlight_offset);
		vec3 right_target_pos = rotate_pos(right_light_pos, cubo.car_ang, right_target_offset);

		vec3 left_light_dir = (left_light_pos - point_pos) / length(left_light_pos - point_pos);
		vec3 left_spot_light_dir = (left_light_pos - left_target_pos) / length(left_light_pos - left_target_pos);
		vec3 right_light_dir = (right_light_pos - point_pos) / length(right_light_pos - point_pos);
		vec3 right_spot_light_dir = (right_light_pos - right_target_pos) / length(right_light_pos - right_target_pos);

		float cos_outer_angle = 0.94;
		float cos_inner_angle = 1.00;
		float distance_g = 0.1;
		float exponent_beta = 1.5;

		vec3 left_spot_light_color = obj_color
										* pow((distance_g / length(left_light_pos - point_pos)), exponent_beta)
										* clamp((dot(left_light_dir, left_spot_light_dir) - cos_outer_angle) / (cos_inner_angle - cos_outer_angle),
											0, 1)
										* 2.5f;
		vec3 right_spot_light_color = obj_color
										* pow((distance_g / length(right_light_pos - point_pos)), exponent_beta)
										* clamp((dot(right_light_dir, right_spot_light_dir) - cos_outer_angle) / (cos_inner_angle - cos_outer_angle),
											0, 1)
										* 2.5f;

		outColor += vec4((left_spot_light_color + right_spot_light_color) * light_color, 1.0);

	}

}
