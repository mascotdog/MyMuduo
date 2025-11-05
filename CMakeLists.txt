cmake_minimum_required(VERSION 2.5)
project(mymuduo)

# mymuduo 最终翻译成 so 动态库，设置动态库的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置调试信息以及启动 C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

# 自动收集源文件
file(GLOB SRC_LIST
    CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/*.cc
    ${PROJECT_SOURCE_DIR}/*.h
)

# 编译生成动态库 mymuduo
add_library(mymuduo SHARED ${SRC_LIST})
