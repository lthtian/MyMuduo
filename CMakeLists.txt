cmake_minimum_required(VERSION 2.5)
project(MyMuduo)

# 设置动态库路径, 放在根目录的lib文件下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC -lpthread")
# 定义参与编译的源代码文件
# 使用这种自动识别时, 想让新添加的文件加入编译, 就需要让CMakeLists.txt重新配置.
aux_source_directory(. SRC_LIST)
# 编译生成动态库
add_library(MyMuduo SHARED ${SRC_LIST})