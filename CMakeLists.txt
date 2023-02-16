cmake_minimum_required(VERSION 3.24)
project(MemoryPool)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

add_executable(mempool_c src/main.c src/pointer_bit_hacks.c src/memory.c src/tests.c src/memory_pool.c)
