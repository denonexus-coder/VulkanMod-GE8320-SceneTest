cmake_minimum_required(VERSION 3.22.1)
project("vulkanmod_scene_test")

add_library(vulkanmod_scene_test SHARED
    main.cpp
)

target_link_libraries(vulkanmod_scene_test
    android
    log
    vulkan
)
