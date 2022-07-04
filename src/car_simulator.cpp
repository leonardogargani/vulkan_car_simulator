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
        Model M_SlCar;
        Texture T_SlCar;
        DescriptorSet DS_SlCar;

        Model M_SlTerrain;
        Texture T_SlTerrain;
        DescriptorSet DS_SlTerrain;

        DescriptorSet DS_global;


        void setWindowParameters() {
                windowWidth = 800;
                windowHeight = 600;
                windowTitle = "Car simulator";
                initialBackgroundColor = {1.0f, 1.0f, 1.0f, 1.0f};

                // Descriptor pool sizes
                uniformBlocksInPool = 3;
                texturesInPool = 2;
                setsInPool = 3;
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
                M_SlCar.init(this, "models/Hummer.obj");
                T_SlCar.init(this, "textures/HummerDiff.png");
                DS_SlCar.init(this, &DSLobj, {
                                // - first  element : the binding number
                                // - second element : UNIFORM or TEXTURE (an enum) depending on the type
                                // - third  element : only for UNIFORMs, the size of the corresponding C++ object
                                // - fourth element : only for TEXTUREs, the pointer to the corresponding texture object
                                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                                {1, TEXTURE, 0, &T_SlCar}
                });

                M_SlTerrain.init(this, "models/Terrain.obj");
                T_SlTerrain.init(this, "textures/SolidGreen.png");
                DS_SlTerrain.init(this, &DSLobj, {
                                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                                {1, TEXTURE, 0, &T_SlTerrain}
                });

                DS_global.init(this, &DSLglobal, {
                                {0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}
                });
        }


        void localCleanup() {
                DS_SlCar.cleanup();
                T_SlCar.cleanup();
                M_SlCar.cleanup();

                DS_SlTerrain.cleanup();
                T_SlTerrain.cleanup();
                M_SlTerrain.cleanup();

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

                VkBuffer vertexBuffers[] = {M_SlCar.vertexBuffer};
                // property .vertexBuffer of models, contains the VkBuffer handle to its vertex buffer
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                // property .indexBuffer of models, contains the VkBuffer handle to its index buffer
                vkCmdBindIndexBuffer(commandBuffer, M_SlCar.indexBuffer, 0,
                                     VK_INDEX_TYPE_UINT32);

                // property .pipelineLayout of a pipeline contains its layout.
                // property .descriptorSets of a descriptor set contains its elements.
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P1.pipelineLayout, 1, 1, &DS_SlCar.descriptorSets[currentImage],
                                        0, nullptr);

                // property .indices.size() of models, contains the number of triangles * 3 of the mesh.
                vkCmdDrawIndexed(commandBuffer,
                                 static_cast<uint32_t>(M_SlCar.indices.size()), 1, 0, 0, 0);

                VkBuffer vertexBuffers2[] = {M_SlTerrain.vertexBuffer};
                VkDeviceSize offsets2[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers2, offsets2);
                vkCmdBindIndexBuffer(commandBuffer, M_SlTerrain.indexBuffer, 0,
                                     VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P1.pipelineLayout, 1, 1, &DS_SlTerrain.descriptorSets[currentImage],
                                        0, nullptr);
                vkCmdDrawIndexed(commandBuffer,
                                 static_cast<uint32_t>(M_SlTerrain.indices.size()), 1, 0, 0, 0);

        }



        /************************************************
         *
         * We moved our code into an external .cpp file.
         *
         ************************************************/

        #include "implementation.cpp"


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
