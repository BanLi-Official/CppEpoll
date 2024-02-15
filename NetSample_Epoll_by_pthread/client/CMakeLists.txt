cmake_minimum_required(VERSION 3.28.2)
project(Client)

aux_source_directory(${PROJECT_SOURCE_DIR} source)
set(EXECUTABLE_OUTPUT_PATH ${output})

add_executable(client ${source})