#include "car_simulator.hpp"


struct globalUniformBufferObject {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
};

struct UniformBufferObject {
        // matrix containing the rotation of the model
        alignas(16) glm::mat4 model;
};


class MyProject : public BaseProject {
protected:

        // Descriptor Layouts (what will be passed to the shaders)
        DescriptorSetLayout DSLglobal;
        DescriptorSetLayout DSLobj;

        // Pipelines (Shader couples)
        Pipeline P1;

        // Models, textures and Descriptors (values assigned to the uniforms)
        Model M_SlBody;
        Texture T_SlBody;
        DescriptorSet DS_SlBody;

        Model M_SlHandle;
        Texture T_SlHandle;
        DescriptorSet DS_SlHandle;

        Model M_SlWheel;
        Texture T_SlWheel;
        DescriptorSet DS_SlWheel1;
        DescriptorSet DS_SlWheel2;
        DescriptorSet DS_SlWheel3;

        DescriptorSet DS_global;


        void setWindowParameters() {
                windowWidth = 800;
                windowHeight = 600;
                windowTitle = "Car simulator";
                initialBackgroundColor = {1.0f, 1.0f, 1.0f, 1.0f};

                // Descriptor pool sizes
                uniformBlocksInPool = 6;
                texturesInPool = 5;
                setsInPool = 6;
        }

        void localInit() {
                // Descriptor Layouts (what will be passed to the shaders)
                DSLobj.init(this, {
                                // this array contains the binding:
                                // first  element : the binding number
                                // second element : the time of element (buffer or texture)
                                // third  element : the pipeline stage where it will be used
                                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
                                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
                });

                DSLglobal.init(this, {
                                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                });

                // Pipelines (Shader couples)
                // The last array is a vector of pointer to the layouts of the sets that will be used in the pipeline
                P1.init(this, "shaders/vert.spv", "shaders/frag.spv", {&DSLglobal, &DSLobj});

                // Models, textures and Descriptors (values assigned to the uniforms)
                M_SlBody.init(this, "models/Hummer.obj");
                T_SlBody.init(this, "textures/HummerDiff.png");
                DS_SlBody.init(this, &DSLobj, {
                                // - first  element : the binding number
                                // - second element : UNIFORM or TEXTURE (an enum) depending on the type
                                // - third  element : only for UNIFORMs, the size of the corresponding C++ object
                                // - fourth element : only for TEXTUREs, the pointer to the corresponding texture object
                                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                                {1, TEXTURE, 0, &T_SlBody}
                });

                M_SlHandle.init(this, "models/Terrain.obj");
                T_SlHandle.init(this, "textures/Solid_green.png");
                DS_SlHandle.init(this, &DSLobj, {
                                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                                {1, TEXTURE, 0, &T_SlHandle}
                });

                M_SlWheel.init(this, "models/SlotWheel.obj");
                T_SlWheel.init(this, "textures/SlotWheel.png");
                DS_SlWheel1.init(this, &DSLobj, {
                                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                                {1, TEXTURE, 0, &T_SlWheel}
                });
                DS_SlWheel2.init(this, &DSLobj, {
                                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                                {1, TEXTURE, 0, &T_SlWheel}
                });
                DS_SlWheel3.init(this, &DSLobj, {
                                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                                {1, TEXTURE, 0, &T_SlWheel}
                });

                DS_global.init(this, &DSLglobal, {
                                {0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}
                });
        }


        void localCleanup() {
                DS_SlBody.cleanup();
                T_SlBody.cleanup();
                M_SlBody.cleanup();

                DS_SlHandle.cleanup();
                T_SlHandle.cleanup();
                M_SlHandle.cleanup();

                DS_SlWheel1.cleanup();
                DS_SlWheel2.cleanup();
                DS_SlWheel3.cleanup();
                M_SlWheel.cleanup();
                T_SlWheel.cleanup();

                DS_global.cleanup();

                P1.cleanup();
                DSLglobal.cleanup();
                DSLobj.cleanup();
        }

        // Here it is the creation of the command buffer:
        // you send to the GPU all the objects you want to draw, with their buffers and textures.
        void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  P1.graphicsPipeline);
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P1.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
                                        0, nullptr);

                VkBuffer vertexBuffers[] = {M_SlBody.vertexBuffer};
                // property .vertexBuffer of models, contains the VkBuffer handle to its vertex buffer
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                // property .indexBuffer of models, contains the VkBuffer handle to its index buffer
                vkCmdBindIndexBuffer(commandBuffer, M_SlBody.indexBuffer, 0,
                                     VK_INDEX_TYPE_UINT32);

                // property .pipelineLayout of a pipeline contains its layout.
                // property .descriptorSets of a descriptor set contains its elements.
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P1.pipelineLayout, 1, 1, &DS_SlBody.descriptorSets[currentImage],
                                        0, nullptr);

                // property .indices.size() of models, contains the number of triangles * 3 of the mesh.
                vkCmdDrawIndexed(commandBuffer,
                                 static_cast<uint32_t>(M_SlBody.indices.size()), 1, 0, 0, 0);

                VkBuffer vertexBuffers2[] = {M_SlHandle.vertexBuffer};
                VkDeviceSize offsets2[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers2, offsets2);
                vkCmdBindIndexBuffer(commandBuffer, M_SlHandle.indexBuffer, 0,
                                     VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P1.pipelineLayout, 1, 1, &DS_SlHandle.descriptorSets[currentImage],
                                        0, nullptr);
                vkCmdDrawIndexed(commandBuffer,
                                 static_cast<uint32_t>(M_SlHandle.indices.size()), 1, 0, 0, 0);

                VkBuffer vertexBuffers3[] = {M_SlWheel.vertexBuffer};
                VkDeviceSize offsets3[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers3, offsets3);
                vkCmdBindIndexBuffer(commandBuffer, M_SlWheel.indexBuffer, 0,
                                     VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P1.pipelineLayout, 1, 1, &DS_SlWheel1.descriptorSets[currentImage],
                                        0, nullptr);
                vkCmdDrawIndexed(commandBuffer,
                                 static_cast<uint32_t>(M_SlWheel.indices.size()), 1, 0, 0, 0);

                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P1.pipelineLayout, 1, 1, &DS_SlWheel2.descriptorSets[currentImage],
                                        0, nullptr);
                vkCmdDrawIndexed(commandBuffer,
                                 static_cast<uint32_t>(M_SlWheel.indices.size()), 1, 0, 0, 0);

                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P1.pipelineLayout, 1, 1, &DS_SlWheel3.descriptorSets[currentImage],
                                        0, nullptr);
                vkCmdDrawIndexed(commandBuffer,
                                 static_cast<uint32_t>(M_SlWheel.indices.size()), 1, 0, 0, 0);

        }

        //glm::vec3 eye = glm::vec3(2.0f, 2.0f, 2.0f);

        // initial direction the robot (will be updated inside the function)
        float car_angle = 0.0;
        // initial position of the robot (will be updated inside the function)
        glm::vec3 car_pos = glm::vec3(0.0,0.0,0.0);

        // Here is where you update the uniforms.
        // Very likely this will be where you will be writing the logic of your application.
        void updateUniformBuffer(uint32_t currentImage) {

                // TODO use time

                globalUniformBufferObject gubo{};
                UniformBufferObject ubo{};

                void* data;

                gubo.proj = glm::perspective(glm::radians(45.0f),
                                             swapChainExtent.width / (float) swapChainExtent.height,
                                             0.1f, 10.0f);
                gubo.proj[1][1] *= -1;


                if (glfwGetKey(window, GLFW_KEY_W)) {
                        car_pos.x += 0.005;
                } else if (glfwGetKey(window, GLFW_KEY_S)) {
                        car_pos.x -= 0.005;
                }

                ubo.model = glm::translate(glm::mat4(1.0), car_pos)
                                * glm::rotate(glm::mat4(1.0), glm::radians(-90.0f), glm::vec3(0,0,1))
                                * glm::rotate(glm::mat4(1.0), glm::radians(-90.0f), glm::vec3(0,1,0))
                                * glm::rotate(glm::mat4(1.0), glm::radians(car_angle), glm::vec3(0,1,0));

                gubo.view = glm::lookAt(glm::vec3(car_pos.x + 10.0f, car_pos.y + 2.0f, car_pos.z),
                                        car_pos,
                                        glm::vec3(0.0f, 1.0f, 0.0f));


                vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0,
                            sizeof(gubo), 0, &data);
                memcpy(data, &gubo, sizeof(gubo));
                vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);

                // For the Hummer
                vkMapMemory(device, DS_SlBody.uniformBuffersMemory[0][currentImage], 0,
                            sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(device, DS_SlBody.uniformBuffersMemory[0][currentImage]);


                // For the Terrain
                //ubo.model = glm::mat4(1.0f);
                vkMapMemory(device, DS_SlHandle.uniformBuffersMemory[0][currentImage], 0,
                            sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(device, DS_SlHandle.uniformBuffersMemory[0][currentImage]);

                // For the Slot Wheel1
                vkMapMemory(device, DS_SlWheel1.uniformBuffersMemory[0][currentImage], 0,
                            sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(device, DS_SlWheel1.uniformBuffersMemory[0][currentImage]);

                // For the Slot Wheel2
                vkMapMemory(device, DS_SlWheel2.uniformBuffersMemory[0][currentImage], 0,
                            sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(device, DS_SlWheel2.uniformBuffersMemory[0][currentImage]);

                // For the Slot Wheel3
                vkMapMemory(device, DS_SlWheel3.uniformBuffersMemory[0][currentImage], 0,
                            sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(device, DS_SlWheel3.uniformBuffersMemory[0][currentImage]);
        }
};


int main() {
        MyProject app;

        try {
                app.run();
        } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}
