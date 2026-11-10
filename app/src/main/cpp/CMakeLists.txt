cmake_minimum_required(VERSION 3.22.1)
project("vulkanmod_scene_test")

add_library(vulkanmod_scene_test SHARED
    main.cpp
    android_native_app_glue.c
)

target_include_directories(vulkanmod_scene_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vulkanmod_scene_test
    android
    log
    vulkan
)
