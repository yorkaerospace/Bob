cmake_minimum_required(VERSION 3.13)

project(HP203B)

add_library(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
	hp203b.c
	hp203b.h
)

target_link_libraries(${PROJECT_NAME} PUBLIC pico_stdlib hardware_i2c)
