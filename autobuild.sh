#!/bin/bash

set -e

# 如果没有build目录, 创建该项目
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build && 
    cmake .. && 
    make

cd ..

# 把头文件拷贝到 /usr/include/mymuduo so库拷贝到 /usr/lib .PATH
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi
 
for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libMyMuduo.so /usr/lib

ldconfig