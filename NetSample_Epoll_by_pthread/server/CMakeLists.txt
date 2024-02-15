cmake_minimum_required(VERSION 3.28.2)
project(Server)

aux_source_directory(${PROJECT_SOURCE_DIR} source)
set(EXECUTABLE_OUTPUT_PATH ${output})

link_libraries(pthread)
find_package(Threads REQUIRED)


add_executable(server ${source})