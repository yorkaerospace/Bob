cmake_minimum_required(VERSION 3.13)

project(HP203B)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
	PUBLIC
    hp203b.h
    PRIVATE
	hp203b.c
)

target_include_directories(${PROJECT_NAME}
	PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC pico_stdlib hardware_i2c)
