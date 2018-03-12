cmake_minimum_required(VERSION 3.3)

project (vulkanCompute)

find_package(Vulkan REQUIRED)

set( SRC 
    VulkanCompute/main.cpp
    VulkanCompute/VulkanCompute.cpp
)

set( HDR
    VulkanCompute/BufferExtension.h
    VulkanCompute/Helpers.h
    VulkanCompute/VulkanCompute.h
)

add_executable(vulkanCompute ${SRC} ${HDR})
target_link_libraries(vulkanCompute ${Vulkan_LIBRARY}) 