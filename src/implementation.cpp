#include "car_simulator.hpp"


struct Car {

        glm::vec3 pos;
        // TODO rewrite the angle as yaw-pitch-roll to take into account the terrain
        float angle;
        float lin_speed;
        float ang_speed;

        Car(int terrain_scale_factor)
        {
                pos = glm::vec3(0.0, 0.0, 0.0);
                angle = 0.0;
                lin_speed = 1.2 * terrain_scale_factor;
                ang_speed = 1.8 * terrain_scale_factor;
        }
};


/*
 * How the position of the terrain makes the car look like:
 *  - x:  backward (-) / forward (+)
 *  - y:  up (-) / down (+)
 *  - z:  left (-) / right (+)
 */
float terrain_scale_factor = 10.0;
glm::vec3 terrain_pos = glm::vec3(-22.0,-1.5,-16.0) * terrain_scale_factor;

float delta_time = 0.0;
float logging_time = 0.0;

enum CameraType { Normal, Distant, Front };
CameraType camera_type = Normal;


Car car = Car(terrain_scale_factor);


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

        if (glfwGetKey(window, GLFW_KEY_W)) {
                car.pos.x -= car.lin_speed * cos(glm::radians(car.angle)) * delta_time;
                car.pos.z += car.lin_speed * sin(glm::radians(car.angle)) * delta_time;
        } else if (glfwGetKey(window, GLFW_KEY_S)) {
                car.pos.x += car.lin_speed * cos(glm::radians(car.angle)) * delta_time;
                car.pos.z -= car.lin_speed * sin(glm::radians(car.angle)) * delta_time;
        }

        if (glfwGetKey(window, GLFW_KEY_A)) {
                car.angle += car.ang_speed * delta_time;
        } else if (glfwGetKey(window, GLFW_KEY_D)) {
                car.angle -= car.ang_speed * delta_time;
        }

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

        ubo.model = glm::translate(glm::mat4(1.0), terrain_pos)
                    * glm::rotate(glm::mat4(1.0), glm::radians(-90.0f), glm::vec3(0,0,1))
                    * glm::rotate(glm::mat4(1.0), glm::radians(-90.0f), glm::vec3(0,1,0))
                    * glm::scale(glm::mat4(1.0), glm::vec3(terrain_scale_factor));

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
                     swapChainExtent.width / (float) swapChainExtent.height,
                     0.1f, 100.0f);

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
                std::cout << "Car at  x=" << car.pos.x << ", z=" << car.pos.z << ", angle=" << car.angle << std::endl;
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

        log_car_pose(0.1);

}
