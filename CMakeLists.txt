cmake_minimum_required (VERSION 3.8)

project (chip8)

set (SOURCES src/chip8.c)

include_directories (include)

add_executable (${PROJECT_NAME} ${SOURCES})