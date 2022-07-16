#include "car_simulator.hpp"

#define LIN_ACCEL 10.0
#define LIN_DECEL 30.0
#define PITCH_SLOWDOWN 0.3
#define ANG_SPEED 40.0
#define TOP_LIN_SPEED 20.0


struct Car {
        glm::vec3 pos;
        glm::vec3 angle = glm::vec3(1.0);
        float lin_speed;
        float ang_speed;
        std::vector<glm::vec3> last_angles{300, glm::vec3(0.0f)};

        glm::vec3 wheel_fl_pos;
        glm::vec3 wheel_fr_pos;
        glm::vec3 wheel_rl_pos;
        glm::vec3 wheel_rr_pos;
};


float terrain_scale_factor = 10.0;

float delta_time = 0.0;
float logging_time = 0.0;

int fps_sum = 0;
int fps_count = 0;

glm::vec4 selector;

enum CameraType { Normal, Distant, FirstPerson, MiniMap };
CameraType camera_type = Normal;


Car car = Car();


// Compute elapsed time between two function calls
float compute_elapsed_time() {
        static auto start_time = std::chrono::high_resolution_clock::now();
        static float last_time = 0.0f;
        auto current_time = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period> (current_time - start_time).count();
        delta_time = time - last_time;
        last_time = time;
        return delta_time;
}


// Compute the height of a point in the terrain, given its coordinates in the xz-plane
float compute_point_height(float x, float z) {

        float x_point = (x / terrain_scale_factor) + 50.0;
        float z_point = (z / terrain_scale_factor) + 50.0;

        // 109 = number of vertices  |  100 = size of the terrain before scaling
        float x_point_float_index = x_point * 109.0 / 100.0;
        float z_point_float_index = z_point * 109.0 / 100.0;

        /*
         *      D -- C        (z) <---|
         *      | \  |                |
         *      A -- B                v
         *                           (x)
         *
         */
        int x_a_index = std::ceil(x_point_float_index);
        int z_a_index = std::ceil(z_point_float_index);
        int x_b_index = std::ceil(x_point_float_index);
        int z_b_index = std::floor(z_point_float_index);
        int x_c_index = std::floor(x_point_float_index);
        int z_c_index = std::floor(z_point_float_index);
        int x_d_index = std::floor(x_point_float_index);
        int z_d_index = std::ceil(z_point_float_index);

        float x_a = x_a_index * 100.0 / 109.0;
        float x_b = x_b_index * 100.0 / 109.0;
        float x_c = x_c_index * 100.0 / 109.0;
        float x_d = x_d_index * 100.0 / 109.0;

        float y_a = terrain.altitudes[x_a_index][z_a_index];
        float y_b = terrain.altitudes[x_b_index][z_b_index];
        float y_c = terrain.altitudes[x_c_index][z_c_index];
        float y_d = terrain.altitudes[x_d_index][z_d_index];

        float z_a = z_a_index * 100.0 / 109.0;
        float z_b = z_b_index * 100.0 / 109.0;
        float z_c = z_c_index * 100.0 / 109.0;
        float z_d = z_d_index * 100.0 / 109.0;

        // used to determine in which of the two triangles the point is in
        float x_point_percentage_index = x_point_float_index - x_c_index;
        float z_point_percentage_index = z_point_float_index - z_c_index;

        float interpolated_height = 0.0;

        // check which of the two triangles the point belongs to and act as a consequence
        if (x_point_percentage_index < - z_point_percentage_index + 1) {
                float det = (z_b - z_d) * (x_c - x_d) + (x_d - x_b) * (z_c - z_d);
                float lambda_1 = ((z_b - z_d) * (x_point - x_d) + (x_d - x_b) * (z_point - z_d)) / det;
                float lambda_2 = ((z_d - z_c) * (x_point - x_d) + (x_c - x_d) * (z_point - z_d)) / det;
                float lambda_3 = 1.0f - lambda_1 - lambda_2;
                interpolated_height = lambda_1 * y_c + lambda_2 * y_b + lambda_3 * y_d;
        } else {
                float det = (z_b - z_d) * (x_a - x_d) + (x_d - x_b) * (z_a - z_d);
                float lambda_1 = ((z_b - z_d) * (x_point - x_d) + (x_d - x_b) * (z_point - z_d)) / det;
                float lambda_2 = ((z_d - z_a) * (x_point - x_d) + (x_a - x_d) * (z_point - z_d)) / det;
                float lambda_3 = 1.0f - lambda_1 - lambda_2;
                interpolated_height = lambda_1 * y_a + lambda_2 * y_b + lambda_3 * y_d;
        }

        return interpolated_height * terrain_scale_factor;

}


void handle_key_presses() {

        // switch to the selected camera
        if (glfwGetKey(window, GLFW_KEY_V)) {
                camera_type = Normal;
        } else if (glfwGetKey(window, GLFW_KEY_B)) {
                camera_type = Distant;
        } else if (glfwGetKey(window, GLFW_KEY_N)) {
                camera_type = FirstPerson;
        } else if (glfwGetKey(window, GLFW_KEY_M)) {
                camera_type = MiniMap;
        }
        
        if (glfwGetKey(window, GLFW_KEY_1)) {
        		selector = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        } else if (glfwGetKey(window, GLFW_KEY_2)) {
        		selector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        }

        // change velocity with a constant acceleration/deceleration profile
        if (glfwGetKey(window, GLFW_KEY_W)) {
                car.lin_speed = std::min(car.lin_speed + LIN_ACCEL * delta_time, TOP_LIN_SPEED - (-car.angle.z * PITCH_SLOWDOWN));
        } else if (glfwGetKey(window, GLFW_KEY_S)) {
                car.lin_speed = std::max(car.lin_speed - LIN_ACCEL * delta_time, -(TOP_LIN_SPEED + (-car.angle.z * PITCH_SLOWDOWN)));
        } else {
                // decelerate until 0
                if (car.lin_speed > 0.1) {
                        car.lin_speed -= LIN_DECEL * delta_time;
                } else if (car.lin_speed < -0.1) {
                        car.lin_speed += LIN_DECEL * delta_time;
                } else {
                        car.lin_speed = 0.0;
                }
        }

        // steer only when the car is moving, and invert steering when going backward
        if (glfwGetKey(window, GLFW_KEY_A)) {
                if (car.lin_speed > 0.0) {
                        car.angle.y += ANG_SPEED * delta_time;
                } else if (car.lin_speed < 0.0) {
                        car.angle.y -= ANG_SPEED * delta_time;
                }
        } else if (glfwGetKey(window, GLFW_KEY_D)) {
                if (car.lin_speed > 0.0) {
                        car.angle.y -= ANG_SPEED * delta_time;
                } else if (car.lin_speed < 0.0) {
                        car.angle.y += ANG_SPEED * delta_time;
                }
        }

        // reset to the initial position
        if (glfwGetKey(window, GLFW_KEY_R)) {
                car.pos = glm::vec3(0.0f);
                car.angle = glm::vec3(0.0f);
                car.lin_speed = 0.0;
                car.last_angles = std::vector<glm::vec3>(300, glm::vec3(0.0f));
        }

        // keep car angle in the range [-360,360]
        if (car.angle.y >= 360.0) {
                car.angle.y -= 360.0;
        } else if (car.angle.y <= -360.0) {
                car.angle.y += 360.0;
        }

        // update car position (the car cannot escape from the map),
        // dividing by 2.03 instead of 2.0 in order to have a margin from the real border
        if (car.pos.x >= terrain.height * terrain_scale_factor / 2.03) {
                // step behind the map border so that movement is allowed again,
                // otherwise the car gets stuck at that border
                car.pos.x -= 0.1;
        } else if (car.pos.x <= terrain.height * terrain_scale_factor / -2.03) {
                car.pos.x += 0.1;
        } else {
                car.pos.x -= cos(glm::radians(car.angle.y)) * (car.lin_speed * delta_time);
        }

        if (car.pos.z >= terrain.width * terrain_scale_factor / 2.03) {
                car.pos.z -= 0.1;
        } else if (car.pos.z <= terrain.width * terrain_scale_factor / -2.03) {
                car.pos.z += 0.1;
        } else {
                car.pos.z += sin(glm::radians(car.angle.y)) * (car.lin_speed * delta_time);
        }

        // update car height
        car.pos.y = compute_point_height(car.pos.x, car.pos.z);

        // compute real pitch and roll only if useful
        if (camera_type == FirstPerson) {
                car.angle.x = 0.0;
                car.angle.z = 0.0;
        } else {
                // compute new position of the wheels
                car.wheel_fl_pos.x = car.pos.x + (-1.0814 * cos(glm::radians(-car.angle.y))) -
                                     (1.0 * sin(glm::radians(-car.angle.y)));
                car.wheel_fl_pos.z = car.pos.z + (1.0 * cos(glm::radians(-car.angle.y))) +
                                     (-1.0814 * sin(glm::radians(-car.angle.y)));
                car.wheel_fr_pos.x = car.pos.x + (-1.0814 * cos(glm::radians(-car.angle.y))) -
                                     (-1.0 * sin(glm::radians(-car.angle.y)));
                car.wheel_fr_pos.z = car.pos.z + (-1.0 * cos(glm::radians(-car.angle.y))) +
                                     (-1.0814 * sin(glm::radians(-car.angle.y)));
                car.wheel_rl_pos.x = car.pos.x + (2.4023 * cos(glm::radians(-car.angle.y))) -
                                     (1.0 * sin(glm::radians(-car.angle.y)));
                car.wheel_rl_pos.z = car.pos.z + (1.0 * cos(glm::radians(-car.angle.y))) +
                                     (2.4023 * sin(glm::radians(-car.angle.y)));
                car.wheel_rr_pos.x = car.pos.x + (2.4023 * cos(glm::radians(-car.angle.y))) -
                                     (-1.0 * sin(glm::radians(-car.angle.y)));
                car.wheel_rr_pos.z = car.pos.z + (-1.0 * cos(glm::radians(-car.angle.y))) +
                                     (2.4023 * sin(glm::radians(-car.angle.y)));

                // compute the height of the wheels
                car.wheel_fl_pos.y = compute_point_height(car.wheel_fl_pos.x, car.wheel_fl_pos.z);
                car.wheel_fr_pos.y = compute_point_height(car.wheel_fr_pos.x, car.wheel_fr_pos.z);
                car.wheel_rl_pos.y = compute_point_height(car.wheel_rl_pos.x, car.wheel_rl_pos.z);
                car.wheel_rr_pos.y = compute_point_height(car.wheel_rr_pos.x, car.wheel_rr_pos.z);

                // to compute car roll, make an average between front and rear wheels
                float delta_y_front_rear = ((car.wheel_fl_pos.y - car.wheel_fr_pos.y) +
                                            (car.wheel_rl_pos.y - car.wheel_rr_pos.y)) / 2.0;
                // width between wheels
                float delta_z_front_rear = 1.0 - (-1.0);

                car.angle.x = -glm::degrees(atan(delta_y_front_rear / delta_z_front_rear));

                // to compute car pitch, make an average between front and rear wheels
                float delta_y_left_right = ((car.wheel_fl_pos.y - car.wheel_rl_pos.y) +
                                            (car.wheel_fr_pos.y - car.wheel_rr_pos.y)) / 2.0;
                // distance between wheels front and rear wheels
                float delta_x_left_right = 2.4023 - (-1.0814);

                car.angle.z = -glm::degrees(atan(delta_y_left_right / delta_x_left_right));
        }

}


void update_tubo_for_terrain(uint32_t currentImage) {

        terrainUniformBufferObject tubo{};
        void* data;

        tubo.model = glm::scale(glm::mat4(1.0), glm::vec3(terrain_scale_factor));
        
        tubo.selector = selector;

        vkMapMemory(device, DS_SlTerrain.uniformBuffersMemory[0][currentImage], 0, sizeof(tubo), 0, &data);
        memcpy(data, &tubo, sizeof(tubo));
        vkUnmapMemory(device, DS_SlTerrain.uniformBuffersMemory[0][currentImage]);

}


void update_cubo_for_car(uint32_t currentImage) {

        carUniformBufferObject cubo{};
        void* data;

        cubo.model = glm::translate(glm::mat4(1.0), car.pos)
                    * glm::rotate(glm::mat4(1.0), glm::radians(car.angle.y), glm::vec3(0, 1, 0))
                    * glm::rotate(glm::mat4(1.0), glm::radians(car.angle.x), glm::vec3(1, 0, 0))
                    * glm::rotate(glm::mat4(1.0), glm::radians(car.angle.z), glm::vec3(0, 0, 1));

        vkMapMemory(device, DS_SlCar.uniformBuffersMemory[0][currentImage], 0, sizeof(cubo), 0, &data);
        memcpy(data, &cubo, sizeof(cubo));
        vkUnmapMemory(device, DS_SlCar.uniformBuffersMemory[0][currentImage]);

}

void update_ubo_for_skybox(uint32_t currentImage) {
		
        UniformBufferObject skybox_ubo{};
        void* data;

        // model is scaled to make it appear as at infinite distance
        skybox_ubo.model = glm::scale(glm::mat4(1.0), glm::vec3(100000.0f));

        vkMapMemory(device, DS_SlSkyBox.uniformBuffersMemory[0][currentImage], 0, sizeof(skybox_ubo), 0, &data);
        memcpy(data, &skybox_ubo, sizeof(skybox_ubo));
        vkUnmapMemory(device, DS_SlSkyBox.uniformBuffersMemory[0][currentImage]);

}


void update_gubo_for_camera(uint32_t currentImage) {

        globalUniformBufferObject gubo{};
        void* data;

        glm::vec3 camera_offset;
        float field_of_view;

        switch(camera_type)
        {
                case Normal:
                        field_of_view = 60.0;
                        camera_offset = glm::vec3(12.0f, 3.0f, 0.0f);
                        break;
                case Distant:
                        field_of_view = 90.0;
                        camera_offset = glm::vec3(20.0f, 15.0f, 0.0f);
                        break;
                case FirstPerson:
                        field_of_view = 90.0;
                        camera_offset = glm::vec3(0.01f, 1.7f, 0.5f);
                        break;
                case MiniMap:
                        field_of_view = 45.0;
                        break;
        }


        gubo.proj = glm::perspective(glm::radians(field_of_view),
                     swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 300.0f);

        gubo.proj[1][1] *= -1;

        float camera_offset_angle = atan(camera_offset.z / camera_offset.x);

        if (camera_type == FirstPerson) {
                float corda = 2.0 * sqrt(pow(camera_offset.x, 2.0) + pow(camera_offset.z, 2.0)) * sin(glm::radians(car.angle.y / 2.0));

                glm::vec3 camera_offset_rotation = glm::vec3(corda * sin(glm::radians(car.angle.y / 2.0) - camera_offset_angle),
                                                             0.0,
                                                             corda * cos(glm::radians(car.angle.y / 2.0) - camera_offset_angle));

                gubo.view = glm::lookAt(car.pos + camera_offset - camera_offset_rotation,
                                        glm::vec3(car.pos.x - 100.0 * cos(glm::radians(car.angle.y)),
                                                  car.pos.y + 4.0,
                                                  car.pos.z + 100.0 * sin(glm::radians(car.angle.y))),
                                        glm::vec3(0.0f, 1.0f, 0.0f));

        } else if (camera_type == MiniMap) {

                gubo.view = glm::rotate(glm::mat4(1.0), glm::radians(135.0f), glm::vec3(0.0f, 0.0f, 1.0f))
                                * glm::lookAt(glm::vec3(car.pos.x, 50.0f, car.pos.z),
                                                        car.pos + glm::vec3(0.1f, 0.1f, 0.1f),
                                                        glm::vec3(0.0f, 1.0f, 0.0f));

        } else {
                glm::vec3 car_angle;

                // handle the "delay" of the camera in a way so that it is independent from
                // the performance of the pc (discard values to display if delta_time is high)
                for (float i = 0.0; i < delta_time; i += 0.001) {
                        car_angle = car.last_angles.front();
                        car.last_angles.erase(car.last_angles.begin());
                        car.last_angles.push_back(car.angle);
                }

                float corda = 2.0 * sqrt(pow(camera_offset.x, 2.0) + pow(camera_offset.z, 2.0)) * sin(glm::radians(car_angle.y / 2.0));

                glm::vec3 camera_offset_rotation = glm::vec3(corda * sin(glm::radians(car_angle.y / 2.0) - camera_offset_angle),
                                                             0.0,
                                                             corda * cos(glm::radians(car_angle.y / 2.0) - camera_offset_angle));

                glm::vec3 camera_position = car.pos + camera_offset - camera_offset_rotation;

                // since the camera is following the car, never allow it to be under the terrain
                // (and keep a margin of 2,0 above the terrain)
                camera_position.y = std::max(camera_position.y, compute_point_height(camera_position.x, camera_position.z) + 2.0f);

                gubo.view = glm::lookAt(camera_position,
                                        glm::vec3(car.pos.x, car.pos.y + 2.0f, car.pos.z),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
        }
        

        vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0, sizeof(gubo), 0, &data);
        memcpy(data, &gubo, sizeof(gubo));
        vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);

}


void compute_fps() {
        fps_sum += 1 / delta_time;
        fps_count++;
}


void log_info(float logging_period) {
        if (logging_time > logging_period) {
                std::cout << std::fixed << std::setprecision(3);
                std::cout << "Car at:    x=" << std::setw(8) << car.pos.x
                                << "    |    z=" << std::setw(8) << car.pos.z
                                << "    |    y=" << std::setw(7) << car.pos.y
                                << "    |    yaw=" << std::setw(8) << car.angle.y
                                << "    |    pitch=" << std::setw(8) << car.angle.z
                                << "    |    roll=" << std::setw(8) << car.angle.x
                                << "    |    speed=" << std::setw(7) << car.lin_speed
                                << "         [" << std::setw(4) << (fps_sum / fps_count) << " FPS]" << std::endl;
                fps_sum = 0;
                fps_count = 0;
                logging_time = 0.0;
        } else {
                logging_time += delta_time;

        }
}


// Update the uniforms (ubo and gubo)
void updateUniformBuffer(uint32_t currentImage) {

        delta_time = compute_elapsed_time();

        handle_key_presses();

        update_cubo_for_car(currentImage);
        update_gubo_for_camera(currentImage);
        update_tubo_for_terrain(currentImage);
        update_ubo_for_skybox(currentImage);

        //compute_fps();
        //log_info(0.3);

}
