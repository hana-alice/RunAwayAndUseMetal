file(GLOB_RECURSE meshoptimize_h ${CMAKE_CURRENT_LIST_DIR}/*.h)
file(GLOB_RECURSE meshoptimize_cpp ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
source_group("Header Files" FILES ${meshoptimize_h})

find_package(meshoptimizer CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
add_executable(mesh_opt ${meshoptimize_h} ${meshoptimize_cpp})

target_include_directories(mesh_opt
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/
        ${asset_dir}/serialization
        ${TINYGLTF_INCLUDE_DIRS}
)

target_link_libraries(mesh_opt PRIVATE
        raum_core
        raum_asset
        meshoptimizer::meshoptimizer
)