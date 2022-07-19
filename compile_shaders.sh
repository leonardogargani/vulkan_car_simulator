readonly SHADERS_DIR="src/shaders"


glslc "${SHADERS_DIR}"/carShader.frag -o "${SHADERS_DIR}"/carFrag.spv
glslc "${SHADERS_DIR}"/carShader.vert -o "${SHADERS_DIR}"/carVert.spv

glslc "${SHADERS_DIR}"/skyBoxShader.frag -o "${SHADERS_DIR}"/skyBoxFrag.spv
glslc "${SHADERS_DIR}"/skyBoxShader.vert -o "${SHADERS_DIR}"/skyBoxVert.spv

glslc "${SHADERS_DIR}"/terrainShader.frag -o "${SHADERS_DIR}"/terrainFrag.spv
glslc "${SHADERS_DIR}"/terrainShader.vert -o "${SHADERS_DIR}"/terrainVert.spv
