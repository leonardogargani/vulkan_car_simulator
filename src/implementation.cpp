#include "car_simulator.hpp"

#define LIN_ACCEL 20.0
#define LIN_DECEL 80.0

#define ANG_SPEED 30.0

#define TOP_LIN_SPEED 40.0


struct Car {

        glm::vec3 pos;
        // TODO rewrite the angle as yaw-pitch-roll to take into account the terrain
        float angle;
        float lin_speed;
        float ang_speed;
        bool is_accelerating;

        Car()
        {
                pos = glm::vec3(0.0, 0.0, 0.0);
                angle = 0.0;
                lin_speed = 0.0;
        }
};


/*
 * How the position of the terrain makes the car look like:
 *  - x:  backward (-) / forward (+)
 *  - y:  up (-) / down (+)
 *  - z:  left (-) / right (+)
 */
float terrain_scale_factor = 10.0;

float delta_time = 0.0;
float logging_time = 0.0;

int w_key_pressures = 0;
int s_key_pressures = 0;

enum CameraType { Normal, Distant, Front };
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

        // steer only when the car is moving
        if (glfwGetKey(window, GLFW_KEY_A)) {
                car.angle += (car.lin_speed == 0.0) ? 0.0 : ANG_SPEED * delta_time;
        } else if (glfwGetKey(window, GLFW_KEY_D)) {
                car.angle -= (car.lin_speed == 0.0) ? 0.0 : ANG_SPEED * delta_time;
        }

        // update car position
        car.pos.x -= cos(glm::radians(car.angle)) * (car.lin_speed * delta_time);
        car.pos.z += sin(glm::radians(car.angle)) * (car.lin_speed * delta_time);


        if (glfwGetKey(window, GLFW_KEY_V)) {
                camera_type = Normal;
        } else if (glfwGetKey(window, GLFW_KEY_B)) {
                camera_type = Distant;
        } else if (glfwGetKey(window, GLFW_KEY_N)) {
                camera_type = Front;
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
                    * glm::rotate(glm::mat4(1.0), glm::radians(-90.0f), glm::vec3(0,0,1))
                    * glm::rotate(glm::mat4(1.0), glm::radians(-90.0f), glm::vec3(0,1,0))
                    * glm::rotate(glm::mat4(1.0), glm::radians(car.angle), glm::vec3(0,0,1));

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
                case Front:
                        field_of_view = 45.0;
                        camera_offset = glm::vec3(-10.0f, 3.0f, 0.0f);
                        break;
        }


        gubo.proj = glm::perspective(glm::radians(field_of_view),
                     swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 300.0f);

        gubo.proj[1][1] *= -1;

        float corda = 2.0 * camera_offset.x * sin(glm::radians(car.angle / 2.0));

        gubo.view = glm::lookAt(car.pos + camera_offset - glm::vec3(corda * sin(glm::radians(car.angle / 2.0)), 0.0, corda * cos(glm::radians(car.angle / 2.0))),
                                car.pos,
                                glm::vec3(0.0f, 1.0f, 0.0f));

        vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0, sizeof(gubo), 0, &data);
        memcpy(data, &gubo, sizeof(gubo));
        vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);

}


void log_car_pose(float logging_period) {
        if (logging_time > logging_period) {
                std::cout << "Car at  x=" << car.pos.x << " | z=" << car.pos.z
                                << " | angle=" << car.angle << " | lin_speed=" << car.lin_speed << std::endl;
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

        log_car_pose(0.1);

}
