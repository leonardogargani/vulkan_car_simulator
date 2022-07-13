#include "car_simulator.hpp"

#define LIN_ACCEL 20.0
#define LIN_DECEL 80.0

#define ANG_SPEED 30.0

#define TOP_LIN_SPEED 40.0


struct Car {

        glm::vec3 pos;
        glm::vec3 angle;
        float lin_speed;
        float ang_speed;
        bool is_accelerating;
        std::vector<glm::vec3> last_angles;

        Car()
        {
                pos = glm::vec3(0.0f);
                angle = glm::vec3(0.0f);
                lin_speed = 0.0;
                last_angles = std::vector<glm::vec3>(300, glm::vec3(0.0f));
        }
};


float terrain_scale_factor = 10.0;

float delta_time = 0.0;
float logging_time = 0.0;

int w_key_pressures = 0;
int s_key_pressures = 0;

enum CameraType { Normal, Distant, FirstPerson };
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


void handle_key_presses() {

        // change velocity with a constant acceleration/deceleration profile
        if (glfwGetKey(window, GLFW_KEY_W)) {
                car.lin_speed = std::min(car.lin_speed + LIN_ACCEL * delta_time, TOP_LIN_SPEED);
        } else if (glfwGetKey(window, GLFW_KEY_S)) {
                car.lin_speed = std::max(car.lin_speed - LIN_ACCEL * delta_time, -TOP_LIN_SPEED);
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

        // update car position (the car cannot escape from the map)
        if (car.pos.x >= terrain.height * terrain_scale_factor / 2.0) {
                // step behind the map border so that movement is allowed again,
                // otherwise the car gets stuck at that border
                car.pos.x -= 0.1;
        } else if (car.pos.x <= terrain.height * terrain_scale_factor / -2.0) {
                car.pos.x += 0.1;
        } else {
                car.pos.x -= cos(glm::radians(car.angle.y)) * (car.lin_speed * delta_time);
        }

        if (car.pos.z >= terrain.width * terrain_scale_factor / 2.0) {
                car.pos.z -= 0.1;
        } else if (car.pos.z <= terrain.width * terrain_scale_factor / -2.0) {
                car.pos.z += 0.1;
        } else {
                car.pos.z += sin(glm::radians(car.angle.y)) * (car.lin_speed * delta_time);
        }
        
        /*********************************/

        float car_pos_x;
        
        /*if (fabs(car.pos.x - 0.0f) < 0.0001) {
        		car_pos_x = car.pos.x;
        } else {
        		car_pos_x = (car.pos.x / terrain_scale_factor) + 50.0001f;
        }*/
        
        car_pos_x = (car.pos.x / terrain_scale_factor) + 50.0001f;
        
        float car_pos_z;
        
        /*if (fabs(car.pos.z - 0.0f) < 0.0001) {
        		car_pos_z = car.pos.z;
        } else {
        		car_pos_z = (car.pos.z / terrain_scale_factor) + 50.0f;
        }*/
        
        car_pos_z = (car.pos.z / terrain_scale_factor) + 50.0f;
        
        int x_index = round(car_pos_x * 109 / 100); // dove 109 è il numero di vertici --> usa define VERTECES ---- 100 è l'intervallo da -50 a 50 delle x.
        int z_index = round(car_pos_z * 109 / 100);
        
        car.pos.y = terrain.altitudes[x_index][z_index] * terrain_scale_factor;
        
        /*********************************/
        


        if (glfwGetKey(window, GLFW_KEY_V)) {
                camera_type = Normal;
        } else if (glfwGetKey(window, GLFW_KEY_B)) {
                camera_type = Distant;
        } else if (glfwGetKey(window, GLFW_KEY_N)) {
                camera_type = FirstPerson;
        }
}


void update_ubo_for_terrain(uint32_t currentImage) {

        UniformBufferObject ubo{};
        void* data;

        ubo.model = glm::scale(glm::mat4(1.0), glm::vec3(terrain_scale_factor));

        vkMapMemory(device, DS_SlTerrain.uniformBuffersMemory[0][currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, DS_SlTerrain.uniformBuffersMemory[0][currentImage]);

}


void update_ubo_for_car(uint32_t currentImage) {

        UniformBufferObject ubo{};
        void* data;

        ubo.model = glm::translate(glm::mat4(1.0), car.pos)
                    * glm::rotate(glm::mat4(1.0), glm::radians(car.angle.y), glm::vec3(0,1,0));

        vkMapMemory(device, DS_SlCar.uniformBuffersMemory[0][currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
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
                        field_of_view = 45.0;
                        camera_offset = glm::vec3(10.0f, 3.0f, 0.0f);
                        break;
                case Distant:
                        field_of_view = 90.0;
                        camera_offset = glm::vec3(20.0f, 15.0f, 0.0f);
                        break;
                case FirstPerson:
                        field_of_view = 90.0;
                        camera_offset = glm::vec3(0.01f, 1.7f, 0.5f);
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
                                        glm::vec3(car.pos.x - 100 * cos(glm::radians(car.angle.y)),
                                                  0.0f,
                                                  car.pos.z + 100 * sin(glm::radians(car.angle.y))),
                                        glm::vec3(0.0f, 1.0f, 0.0f));
        } else {
                // handle the "delay" of the camera
                glm::vec3 car_angle = car.last_angles.front();
                car.last_angles.erase(car.last_angles.begin());
                car.last_angles.push_back(car.angle);
                float corda = 2.0 * sqrt(pow(camera_offset.x, 2.0) + pow(camera_offset.z, 2.0)) * sin(glm::radians(car_angle.y / 2.0));

                glm::vec3 camera_offset_rotation = glm::vec3(corda * sin(glm::radians(car_angle.y / 2.0) - camera_offset_angle),
                                                             0.0,
                                                             corda * cos(glm::radians(car_angle.y / 2.0) - camera_offset_angle));
                gubo.view = glm::lookAt(car.pos + camera_offset - camera_offset_rotation,
                                        car.pos,
                                        glm::vec3(0.0f, 1.0f, 0.0f));
        }

        vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0, sizeof(gubo), 0, &data);
        memcpy(data, &gubo, sizeof(gubo));
        vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);

}


void log_car_pose(float logging_period) {
        if (logging_time > logging_period) {
                std::cout << std::fixed << std::setprecision(6);
                /*std::cout << "Car at:    pos.x=" << std::setw(7) << car.pos.x << "    |    pos.z=" << std::setw(7) << car.pos.z  << "    |    pos.y=" << std::setw(7) << car.pos.y
                                << "    |    angle.y=" << std::setw(7) << car.angle.y << "    |    speed=" << std::setw(6) << car.lin_speed << std::endl;*/
                std::cout << "Car at:    pos.x=" << car.pos.x << "    |    pos.z=" << car.pos.z  << "    |    pos.y=" << car.pos.y
                                << "    |    angle.y=" << std::setw(7) << car.angle.y << "    |    speed=" << std::setw(6) << car.lin_speed << std::endl;
                logging_time = 0.0;
        } else {
                logging_time += delta_time;
        }
}


// Update the uniforms (ubo and gubo)
void updateUniformBuffer(uint32_t currentImage) {

        delta_time = compute_elapsed_time();

        handle_key_presses();

        update_ubo_for_car(currentImage);
        update_gubo_for_camera(currentImage);
        update_ubo_for_terrain(currentImage);
        update_ubo_for_skybox(currentImage);

        log_car_pose(0.3);

}
