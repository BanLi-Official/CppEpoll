cmake_minimum_required(VERSION 3.22.1)
project(Server)

aux_source_directory(${PROJECT_SOURCE_DIR} source)
set(EXECUTABLE_OUTPUT_PATH ${APP_OUTPUT_PATH})

add_executable(server ${source})

