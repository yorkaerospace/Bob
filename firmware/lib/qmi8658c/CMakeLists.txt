cmake_minimum_required(VERSION 3.13)

project(QMI8658C)

add_library(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
	qmi8658c.c
	qmi8658c.h
)

target_include_directories(${PROJECT_NAME}
	PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC pico_stdlib hardware_i2c)
