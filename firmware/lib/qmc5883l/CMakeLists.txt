cmake_minimum_required(VERSION 3.13)

project(QMC5883L)

add_library(QMC5883L)

target_sources(QMC5883L
    PRIVATE
	qmc5883l.c
	qmc5883l.h
)

target_link_libraries(QMC5883L PUBLIC pico_stdlib hardware_i2c)
