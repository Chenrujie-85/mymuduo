#!/bin/bash

set -e

if[ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi
#删除之前编译的文件
rm -rf `pwd`/build/*

cd `pwd`/build && 
    cmake .. &&
    make

#回到项目根目录
cd ..

#把头文件拷贝到/usr/include/mymuduo so库拷贝到 /usr/lib PATH
if[ ! -d /usr/include/mymuduo]; then
    mkdir /usr/include/mymuduo
fi

for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib

ldconfig