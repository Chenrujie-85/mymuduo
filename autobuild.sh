#!/bin/bash

set -e

SOURCE_DIR=`pwd`
SRC_LIST=${SOURCE_DIR}/src


# 如果没有 build 目录 创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# 如果没有 include 目录 创建该目录
if [ ! -d `pwd`/include ]; then
    mkdir `pwd`/include
fi

# 如果没有 lib 目录 创建该目录
if [ ! -d `pwd`/lib ]; then
    mkdir `pwd`/lib
fi

# 删除存在 build 目录生成文件并执行 cmake 命令
rm -fr ${SOURCE_DIR}/build/*
cd  ${SOURCE_DIR}/build &&
    cmake .. &&
    make install

# 将头文件复制到 /usr/include
cp ${SOURCE_DIR}/include/mymuduo -r /usr/include/mymuduo 

# 将动态库文件复制到/usr/lib
cp ${SOURCE_DIR}/lib/libmymuduo.so /usr/lib

# 使操作生效
ldconfig