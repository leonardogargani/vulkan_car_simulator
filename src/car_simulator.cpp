#include "car_simulator.hpp"

#define VERTICES_NUMBER 110


struct Terrain {
        float width;
        float height;
        std::vector<std::vector<float>> altitudes{VERTICES_NUMBER, std::vector<float>(VERTICES_NUMBER, 0.0f)};
};

struct Point {
	float x;
	float y;
	float z;
};

Terrain terrain = Terrain();


struct globalUniformBufferObject {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
};

struct skyboxUniformBufferObject {
        // matrix containing the rotation of the model
        alignas(16) glm::mat4 model;
};

struct carUniformBufferObject {
        alignas(4) int spotlight_on;
        alignas(4) int backlights_on;
        // matrix containing the rotation of the model
        alignas(16) glm::mat4 model;
        alignas(4) glm::vec3 car_pos;
        alignas(4) glm::vec3 car_ang;
};

struct terrainUniformBufferObject {
        alignas(4) int headlights_on;
        alignas(4) int spotlight_on;
        // matrix containing the rotation of the model
        alignas(16) glm::mat4 model;
        alignas(4) glm::vec3 car_pos;
        alignas(4) glm::vec3 car_ang;
};


class MyProject : public BaseProject {
protected:

        // Descriptor Layouts (what will be passed to the shaders)
        DescriptorSetLayout DSLglobal;
        DescriptorSetLayout DSLobj;
        DescriptorSetLayout DSLSkyBox;

        // Pipelines (Shader couples)
        Pipeline P_Car;
        Pipeline P_SkyBox;
        Pipeline P_Terrain;

        // Models, textures and Descriptors (values assigned to the uniforms)
        Model M_SlCar;
        Texture T_SlCar;
        DescriptorSet DS_SlCar;

        Model M_SlTerrain;
        Texture T_SlTerrain;
        DescriptorSet DS_SlTerrain;
        
        Model M_SlSkyBox;
        SkyBoxTexture T_SlSkyBox;
        DescriptorSet DS_SlSkyBox;

        DescriptorSet DS_global;


        void setWindowParameters() {
                windowWidth = 800;
                windowHeight = 600;
                windowTitle = "Car Simulator";
                initialBackgroundColor = {1.0f, 1.0f, 1.0f, 1.0f};

                // Descriptor pool sizes
                uniformBlocksInPool = 5; //con 8 compila senza errori, 3 è il valore prima dello skybox
                texturesInPool = 8; //con 8 compila senza errori, 2 è il valore prima dello skybox
                setsInPool = 5; //con 8 compila senza errori, 3 è il valore prima dello skybox
        }

        // Function used to compare two Points.
        static bool isP1beforeP2(Point p1, Point p2) {
                return ((p1.x < p2.x)
                        || ((fabs(p1.x - p2.x) < 0.0001) && (p1.z < p2.z)));
        }
        
		void recreateSwapChainDSInit() {
		
                        DS_SlCar.init(this, &DSLobj, {
                                                {0, UNIFORM, sizeof(carUniformBufferObject), nullptr},
                                                {1, TEXTURE, 0, &T_SlCar}});
				        
                        DS_SlTerrain.init(this, &DSLobj, {
                                                {0, UNIFORM, sizeof(terrainUniformBufferObject), nullptr},
                                                {1, TEXTURE, 0, &T_SlTerrain}});

                        DS_SlSkyBox.initDSSkyBox(this, &DSLSkyBox, {
                                                {0, UNIFORM, sizeof(skyboxUniformBufferObject), nullptr},
                                                {1, TEXTURE, 0, &T_SlSkyBox}});

                        DS_global.init(this, &DSLglobal, {
                                                {0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}});
	
		}
		
		void recreateSwapChainPipelinesInit() {
                        P_Car.init(this, "shaders/carVert.spv", "shaders/carFrag.spv", {&DSLglobal, &DSLobj}, VK_COMPARE_OP_LESS);
                        P_Terrain.init(this, "shaders/terrainVert.spv", "shaders/terrainFrag.spv", {&DSLglobal, &DSLobj}, VK_COMPARE_OP_LESS);
                        P_SkyBox.init(this, "shaders/skyBoxVert.spv", "shaders/skyBoxFrag.spv", {&DSLglobal, &DSLSkyBox}, VK_COMPARE_OP_LESS_OR_EQUAL);
		}


        void localInit() {
                // Descriptor Layouts (what will be passed to the shaders)
                DSLobj.init(this, {
                                // this array contains the binding:
                                // first  element : the binding number
                                // second element : the time of element (buffer or texture)
                                // third  element : the pipeline stage where it will be used
                                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}});

                DSLglobal.init(this, {
                                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},});
                
                DSLSkyBox.init(this, {
                                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
                                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}});

                // Pipelines (Shader couples)
                // The last array is a vector of pointer to the layouts of the sets that will be used in the pipeline
                P_Car.init(this, "shaders/carVert.spv", "shaders/carFrag.spv", {&DSLglobal, &DSLobj}, VK_COMPARE_OP_LESS);
                P_Terrain.init(this, "shaders/terrainVert.spv", "shaders/terrainFrag.spv", {&DSLglobal, &DSLobj}, VK_COMPARE_OP_LESS);
                P_SkyBox.init(this, "shaders/skyBoxVert.spv", "shaders/skyBoxFrag.spv", {&DSLglobal, &DSLSkyBox}, VK_COMPARE_OP_LESS_OR_EQUAL);

                // Models, textures and Descriptors (values assigned to the uniforms)
                M_SlCar.init(this, "models/Hummer.obj");
                T_SlCar.init(this, "textures/HummerDiff.png");
                DS_SlCar.init(this, &DSLobj, {
                                // - first  element : the binding number
                                // - second element : UNIFORM or TEXTURE (an enum) depending on the type
                                // - third  element : only for UNIFORMs, the size of the corresponding C++ object
                                // - fourth element : only for TEXTUREs, the pointer to the corresponding texture object
                                {0, UNIFORM, sizeof(carUniformBufferObject), nullptr},
                                {1, TEXTURE, 0, &T_SlCar}});

                M_SlTerrain.init(this, "models/Terrain.obj");
                T_SlTerrain.init(this, "textures/Terrain.png");
                DS_SlTerrain.init(this, &DSLobj, {
                                {0, UNIFORM, sizeof(terrainUniformBufferObject), nullptr},
                                {1, TEXTURE, 0, &T_SlTerrain}});


                float map_min_x = 0.0;
                float map_max_x = 0.0;
                float map_min_z = 0.0;
                float map_max_z = 0.0;

                std::vector<Point> terrain_points;

                for (int i = 0; i < std::size(M_SlTerrain.vertices); i++) {

                        if (M_SlTerrain.vertices[i].pos.x < map_min_x) {
                                map_min_x = M_SlTerrain.vertices[i].pos.x;
                        } else if (M_SlTerrain.vertices[i].pos.x > map_max_x) {
                                map_max_x = M_SlTerrain.vertices[i].pos.x;
                        }

                        if (M_SlTerrain.vertices[i].pos.z < map_min_z) {
                                map_min_z = M_SlTerrain.vertices[i].pos.z;
                        } else if (M_SlTerrain.vertices[i].pos.z > map_max_z) {
                                map_max_z = M_SlTerrain.vertices[i].pos.z;
                        }

                        // Store the unique values of all the vertices, discarding the repeated ones.
                        bool is_point_present = false;
                        for (int j = 0; j < terrain_points.size(); j++) {
                                if (terrain_points[j].x == M_SlTerrain.vertices[i].pos.x && terrain_points[j].z == M_SlTerrain.vertices[i].pos.z) {
                                        is_point_present = true;
                                }
                        }
                        
                        if (!is_point_present){
                        	Point point = {M_SlTerrain.vertices[i].pos.x, M_SlTerrain.vertices[i].pos.y, M_SlTerrain.vertices[i].pos.z};
                                terrain_points.push_back(point);
                        }

                }

                // Reorder Points vector using comparePoints (declared above) static function.
                sort(terrain_points.begin(), terrain_points.end(), isP1beforeP2);

                int col = 0;
                int row = 0;

                // Fill terrain.altitudes vector.
                for (int i = 0; i < terrain_points.size(); i++) {
            		terrain.altitudes[col][row] = terrain_points[i].y;
            		if (row < VERTICES_NUMBER - 1) {
            			row++;
            		} else {
            			col++;
            			row = 0;
            		}
                }

				
                terrain.height = map_max_x - map_min_x;
                terrain.width = map_max_z - map_min_z;


                M_SlSkyBox.init(this, "models/SkyBoxCube.obj");
                //T_SlSkyBox.init(this, {"sky/bkg1_right.png", "sky/bkg1_left.png", "sky/bkg1_top.png", "sky/bkg1_bot.png", "sky/bkg1_front.png", "sky/bkg1_back.png"});
                T_SlSkyBox.init(this, {"sky/sky_universert.png", "sky/sky_univerself.png", "sky/sky_universeup.png", "sky/sky_universedn.png", "sky/sky_universeft.png", "sky/sky_universebk.png"});
                DS_SlSkyBox.initDSSkyBox(this, &DSLSkyBox, {
                                                {0, UNIFORM, sizeof(skyboxUniformBufferObject), nullptr},
                                                {1, TEXTURE, 0, &T_SlSkyBox}});

                DS_global.init(this, &DSLglobal, {
                                                {0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}});
        }


        void localCleanup() {
                T_SlCar.cleanup();
                M_SlCar.cleanup();

                T_SlTerrain.cleanup();
                M_SlTerrain.cleanup();    
                
                T_SlSkyBox.cleanup();
                M_SlSkyBox.cleanup();
                
                DSLglobal.cleanup();
                DSLobj.cleanup();
                DSLSkyBox.cleanup();
        }

        void recreateSwapChainLocalCleanupDS() {
        		DS_SlCar.cleanup();
        		DS_SlTerrain.cleanup();
        		DS_SlSkyBox.cleanup();
        		DS_global.cleanup();      		
        }
        
        void recreateSwapChainLocalCleanupPipelines() {
                P_SkyBox.cleanup();
                P_Car.cleanup();
                P_Terrain.cleanup();
        }

        // Here it is the creation of the command buffer:
        // you send to the GPU all the objects you want to draw, with their buffers and textures.
        void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P_Car.graphicsPipeline);
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P_Car.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
                                        0, nullptr);

                VkBuffer vertexBuffers[] = {M_SlCar.vertexBuffer};
                // property .vertexBuffer of models, contains the VkBuffer handle to its vertex buffer
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                // property .indexBuffer of models, contains the VkBuffer handle to its index buffer
                vkCmdBindIndexBuffer(commandBuffer, M_SlCar.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                // property .pipelineLayout of a pipeline contains its layout.
                // property .descriptorSets of a descriptor set contains its elements.
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P_Car.pipelineLayout, 1, 1, &DS_SlCar.descriptorSets[currentImage],
                                        0, nullptr);

                // property .indices.size() of models, contains the number of triangles * 3 of the mesh.
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(M_SlCar.indices.size()), 1, 0, 0, 0);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P_Terrain.graphicsPipeline);
                VkBuffer vertexBuffers2[] = {M_SlTerrain.vertexBuffer};
                VkDeviceSize offsets2[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers2, offsets2);
                vkCmdBindIndexBuffer(commandBuffer, M_SlTerrain.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P_Terrain.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
                                        0, nullptr);
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P_Terrain.pipelineLayout, 1, 1, &DS_SlTerrain.descriptorSets[currentImage],
                                        0, nullptr);
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(M_SlTerrain.indices.size()), 1, 0, 0, 0);
                                 
                                 
                                 
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P_SkyBox.graphicsPipeline);
                VkBuffer vertexBuffers3[] = {M_SlSkyBox.vertexBuffer};
                VkDeviceSize offsets3[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers3, offsets3);
                vkCmdBindIndexBuffer(commandBuffer, M_SlSkyBox.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P_SkyBox.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
                                        0, nullptr);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        P_SkyBox.pipelineLayout, 1, 1, &DS_SlSkyBox.descriptorSets[currentImage],
                                        0, nullptr);
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(M_SlSkyBox.indices.size()), 1, 0, 0, 0);
				

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
