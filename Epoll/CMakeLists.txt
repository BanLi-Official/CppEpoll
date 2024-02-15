cmake_minimum_required(VERSION 3.22.1)
project(NetSample)


set(APP_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/output)


add_subdirectory(client)

add_subdirectory(server)

 