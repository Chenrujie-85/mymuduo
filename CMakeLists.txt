cmake_minimum_required(VERSION 3.0)

# 设置项目名称
project(mymuduo)

# 添加搜索路径
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/http
    )

# 包含这些目录下的.cpp文件
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/http SRC_HTTP)

# 设置编译选项
set(CXX_FLAGS
    -g
    -Wall
    -std=c++11
    )

# 生成动态库 
add_library(mymuduo SHARED 
            ${SRC_LIST}
            ${SRC_HTTP}
            )

# 目标动态库所需连接的库（这里需要连接libpthread.so）
target_link_libraries(mymuduo pthread)

# 设置生成动态库的路径，放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置安装的默认路径
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

# 将目录安装到 include/mymuduo 下（只安装头文件）
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/ DESTINATION include/mymuduo
        FILES_MATCHING PATTERN "*.h")

