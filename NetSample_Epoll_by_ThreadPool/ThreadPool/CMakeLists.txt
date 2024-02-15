cmake_minimum_required(VERSION 3.22.1)
project(ThreadPoolLib)

aux_source_directory(${PROJECT_SOURCE_DIR} source)
include_directories(ThreadPool)

set(LIBRARY_OUTPUT_PATH  ${LIB_PATH})
message(${LIBRARY_OUTPUT_PATH})

find_package(Threads REQUIRED)   #寻找系统安装的pthread库

add_library(MyThreadPoolLib STATIC ${source})
target_link_libraries(MyThreadPoolLib Threads::Threads) #链接到可执行文件中
