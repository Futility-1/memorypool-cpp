# 1. 规定 CMake 的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 2. 给你的工程起个名字，并指定使用 C++ (CXX)
project(NginxMemoryPool CXX)

# 3. 核心魔法：告诉编译器去哪个文件夹找“菜单”（头文件）
include_directories(include)

# 4. 组装可执行程序：指明具体的"房间号/文件名"
# 语法：add_executable(最终生成的程序名 源文件1 源文件2 ...)
add_executable(test_pool 
    src/ngx_mem_pool.cpp 
    tests/testnginxpool.cpp
)

# 添加综合测试程序
add_executable(comprehensive_test
    src/ngx_mem_pool.cpp
    tests/comprehensive_test.cpp
)