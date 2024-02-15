cmake_minimum_required(VERSION 3.28.2)
project(NetBySelect)

set(output ${PROJECT_SOURCE_DIR}/output)
set(PoolHead ${PROJECT_SOURCE_DIR}/ThreadPool)
set(LIB_PATH ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(ThreadPool)
