#include "car_simulator.hpp"

#define VERTECES 110

struct Terrain {
        float width;
        float height;
        
        /**/
        std::vector<std::vector<float>> altitudes{VERTECES, std::vector<float>(VERTECES, 0.0f)};
        /**/

        Terrain()
        {
                width = 0.0;
                height = 0.0;
        }
};

/**/
struct Point {
	
	float x;
	float y;
	float z;

};

std::vector<Point> points;
/**/

Terrain terrain = Terrain();


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
        DescriptorSetLayout DSLSkyBox;

        // Pipelines (Shader couples)
        Pipeline P1;
        Pipeline P_SkyBox;

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
		static bool comparePoints(Point p1, Point p2) {
			if(p1.x < p2.x) {
				return true;
			}
			else if(fabs(p1.x - p2.x) < 0.0001) {
				if(p1.z < p2.z) {
					return true;
				}
			}

			return false;
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
                
                DSLSkyBox.init(this, {
                				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
                				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
                });

                // Pipelines (Shader couples)
                // The last array is a vector of pointer to the layouts of the sets that will be used in the pipeline
                P1.init(this, "shaders/vert.spv", "shaders/frag.spv", {&DSLglobal, &DSLobj}, VK_COMPARE_OP_LESS);
                P_SkyBox.init(this, "shaders/SkyBoxVert.spv", "shaders/SkyBoxFrag.spv", {&DSLglobal, &DSLSkyBox}, VK_COMPARE_OP_LESS_OR_EQUAL);

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


                float map_min_x = 0.0;
                float map_max_x = 0.0;
                float map_min_z = 0.0;
                float map_max_z = 0.0;
                
                

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
                        
                        /**/
						// Modo non efficiente per avere un array con i valori unici di ogni vertice --> 12100 elementi.
                        
                        bool flag = true;
                        for(int j = 0; j < points.size(); j++) {
                        		if(points[j].x == M_SlTerrain.vertices[i].pos.x && points[j].z == M_SlTerrain.vertices[i].pos.z) {
                        				flag = false;
                        		}
                        }
                        
                        if(flag){
                        	Point point = {M_SlTerrain.vertices[i].pos.x, M_SlTerrain.vertices[i].pos.y, M_SlTerrain.vertices[i].pos.z};
                        	points.push_back(point);
                        }
                        
                        /**/
                             
                }
                
                /**/
                
                // Reorder Points vector using comparePoints (declared above) static function.
				sort(points.begin(), points.end(), comparePoints);

				// Controllo se primo e ultimo elemento sono corretti: cioè se sono il vertice in basso a sinistra e quello in alto a destra.
				//std::cout << points[0].x << " --- " << points[0].z << std::endl;
				//std::cout << points[12099].x << " --- " << points[12099].z << std::endl;
                
                // Fill terrain.altitudes vector.
                int col = 0;
                int row = 0;

                for(int i = 0; i < points.size(); i++) {
            		terrain.altitudes[col][row] = points[i].y;
            		
            		if(row < VERTECES - 1) {
            			row = row + 1;
            		} else {
            			col = col + 1;
            			row = 0;
            		}
                }

				/**/
				
				// Stampe varie per verificare che i valori fossero assegnati correttamente
				/*
				std::cout << "Y in (54,0) altitudes: " << terrain.altitudes[53][0] << std::endl;
				std::cout << "Y in [5831] points: " << points[5830].y << std::endl;
				
				std::cout << "Y in (54,4) altitudes: " << terrain.altitudes[53][3] << std::endl;
				std::cout << "Y in [5834] points: " << points[5833].y << std::endl;	
				
				std::cout << "Y in (0,0) altitudes: " << terrain.altitudes[0][0] << std::endl;
				std::cout << "Y in [0] points: " << points[0].y << std::endl;
				
				std::cout << "Y in (25,0) altitudes: " << terrain.altitudes[24][0] << std::endl;
				std::cout << "Y in [2641] points: " << points[2640].y << std::endl;
				
				std::cout << "Y in (0,0): " << terrain.altitudes[0][0] << std::endl;
				std::cout << "Y in (0,1): " << terrain.altitudes[0][1] << std::endl;
				std::cout << "Y in (0,2): " << terrain.altitudes[0][2] << std::endl;
				std::cout << "Y in (0,3): " << terrain.altitudes[0][3] << std::endl;
				
				std::cout << "Y in (1,0): " << terrain.altitudes[1][0] << std::endl;
				std::cout << "Y in (1,1): " << terrain.altitudes[1][1] << std::endl;
				std::cout << "Y in (1,2): " << terrain.altitudes[1][2] << std::endl;
				std::cout << "Y in (1,3): " << terrain.altitudes[1][3] << std::endl;
				
				std::cout << "Y in (2,0): " << terrain.altitudes[2][0] << std::endl;
				std::cout << "Y in (2,1): " << terrain.altitudes[2][1] << std::endl;
				std::cout << "Y in (2,2): " << terrain.altitudes[2][2] << std::endl;
				std::cout << "Y in (2,3): " << terrain.altitudes[2][3] << std::endl;
				
				std::cout << "Y in (3,0): " << terrain.altitudes[3][0] << std::endl;
				std::cout << "Y in (3,1): " << terrain.altitudes[3][1] << std::endl;
				std::cout << "Y in (3,2): " << terrain.altitudes[3][2] << std::endl;
				std::cout << "Y in (3,3): " << terrain.altitudes[3][3] << std::endl;
				
				std::cout << "Y in (4,0): " << terrain.altitudes[4][0] << std::endl;
				std::cout << "Y in (4,1): " << terrain.altitudes[4][1] << std::endl;
				std::cout << "Y in (4,2): " << terrain.altitudes[4][2] << std::endl;
				std::cout << "Y in (4,3): " << terrain.altitudes[4][3] << std::endl;
				
				std::cout << "Y in (5,0): " << terrain.altitudes[5][0] << std::endl;
				std::cout << "Y in (5,1): " << terrain.altitudes[5][1] << std::endl;
				std::cout << "Y in (5,2): " << terrain.altitudes[5][2] << std::endl;
				std::cout << "Y in (5,3): " << terrain.altitudes[5][3] << std::endl;
				
				std::cout << "MAX X: " << map_max_x << std::endl;
				std::cout << "MIN X: " << map_min_x << std::endl;
				std::cout << "MAX Z: " << map_max_z << std::endl;
				std::cout << "MIN Z: " << map_min_z << std::endl;
				*/
				
                terrain.height = map_max_x - map_min_x;
                terrain.width = map_max_z - map_min_z;



                /*
                Vertex terrain_vertices[std::size(M_SlTerrain.vertices)];
                std::cout << "M_SlTerrain.vertices[0].pos.y = " << M_SlTerrain.vertices[0].pos.y << std::endl;
                std::cout << "M_SlTerrain.vertices.size() = " << M_SlTerrain.vertices.size() << std::endl;

                for (int i = 0; i < 20; i++) {
                        std::cout << i << " - x=" << M_SlTerrain.vertices[i].pos.x
                                        << "  y=" << M_SlTerrain.vertices[i].pos.y
                                        << "  z=" << M_SlTerrain.vertices[i].pos.z << std::endl;
                }
                */

                /*
                int i = 0;
                for (auto & vertex : M_SlTerrain.vertices) {
                        std::cout << i << " - y=" << M_SlTerrain.vertices[i].pos.y << std::endl;
                        i++;
                }
                 */


                M_SlSkyBox.init(this, "models/SkyBoxCube.obj");
                T_SlSkyBox.init(this, {"sky/bkg1_right.png", "sky/bkg1_left.png", "sky/bkg1_top.png", "sky/bkg1_bot.png", "sky/bkg1_front.png", "sky/bkg1_back.png"});
                DS_SlSkyBox.initDSSkyBox(this, &DSLSkyBox, {
								{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
								{1, TEXTURE, 0, &T_SlSkyBox}
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
                
                DS_SlSkyBox.cleanup();
                T_SlSkyBox.cleanup();
                M_SlSkyBox.cleanup();
                
                DS_global.cleanup();
                
                P_SkyBox.cleanup();
                P1.cleanup();
                
                DSLglobal.cleanup();
                DSLobj.cleanup();
                DSLSkyBox.cleanup();
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
                                 
                                 
                                 
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P_SkyBox.graphicsPipeline);
				VkBuffer vertexBuffers3[] = {M_SlSkyBox.vertexBuffer};
				VkDeviceSize offsets3[] = {0};
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers3, offsets3);
				vkCmdBindIndexBuffer(commandBuffer, M_SlSkyBox.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
				
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P_SkyBox.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage], 0, nullptr);
				
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, P_SkyBox.pipelineLayout, 1, 1, &DS_SlSkyBox.descriptorSets[currentImage], 0, nullptr);			
								
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
