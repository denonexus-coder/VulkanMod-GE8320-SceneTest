cmake_minimum_required(VERSION 3.22.1)
project("vulkanmod_scene_test")

# Localizar as fontes android_native_app_glue fornecidas pelo NDK
if(DEFINED ANDROID_NDK)
    set(NDK_DIR "${ANDROID_NDK}")
elseif(DEFINED CMAKE_ANDROID_NDK)
    set(NDK_DIR "${CMAKE_ANDROID_NDK}")
else()
    get_filename_component(NDK_DIR "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
    get_filename_component(NDK_DIR "${NDK_DIR}" DIRECTORY)
    get_filename_component(NDK_DIR "${NDK_DIR}" DIRECTORY)
endif()
set(GLUE_DIR "${NDK_DIR}/sources/android/native_app_glue")

add_library(vulkanmod_scene_test SHARED
    main.cpp
    ${GLUE_DIR}/native_app_glue.c
)

target_include_directories(vulkanmod_scene_test PRIVATE ${GLUE_DIR})

target_link_libraries(vulkanmod_scene_test
    android
    log
    vulkan
)
