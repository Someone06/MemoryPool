cmake_minimum_required(VERSION 3.24)
project(dfs C)

set(CMAKE_C_STANDARD 17)

add_executable(dfs main.c  src/pointer_bit_hacks.c src/memory.c)
