cmake_minimum_required(VERSION 3.28.2)
project(Server)

aux_source_directory(${PROJECT_SOURCE_DIR} source)
set(EXECUTABLE_OUTPUT_PATH ${output})

include_directories(${PoolHead})
link_directories((${LIB_PATH}))

find_package(Threads REQUIRED)
link_libraries(MyThreadPoolLib)


add_executable(server ${source})