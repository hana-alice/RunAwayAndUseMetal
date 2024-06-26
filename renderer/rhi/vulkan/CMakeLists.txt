file(GLOB_RECURSE backend_vk_h ${CMAKE_CURRENT_LIST_DIR}/*.h)
file(GLOB_RECURSE backend_vk_cpp ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
source_group("Header Files" FILES ${backend_vk_h})

file(GLOB_RECURSE rhi_abstract ${renderer_dir}/rhi/base/*.h)
file(GLOB_RECURSE rhi_abstract_cpp ${renderer_dir}/rhi/base/*.cpp)
list(APPEND rhi_abstract ${rhi_abstract_cpp})
# file(GLOB_RECURSE rhi_abstract ${renderer_dir}/rhi/include/*.*)
# file(GLOB_RECURSE rhi_abstract ${renderer_dir}/rhi/*.cpp)
source_group("Abstract Layer" FILES ${rhi_abstract})

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_library(raum_rhi STATIC ${rhi_abstract} ${backend_vk_h} ${backend_vk_cpp})

target_include_directories(raum_rhi
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/
        ${renderer_dir}/rhi/base
        PRIVATE
        ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(raum_rhi PUBLIC
    ${Vulkan_LIBRARIES}
    GPUOpen::VulkanMemoryAllocator
    unofficial::shaderc::shaderc
    glm::glm
    raum_core
)