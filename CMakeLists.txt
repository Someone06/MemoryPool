cmake_minimum_required(VERSION 3.24)
project(MemoryPool)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

add_executable(mempoolC src/main.c src/tests.c src/memory_pool.c src/memory.c src/pointer_bit_hacks.c)
add_executable(mempoolCpp src/CMemoryPool.cpp src/memory_pool.c src/memory.c src/pointer_bit_hacks.c)


include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/39a26e12d67ed6c21feeb606372bfee39a8e6d53.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
        tests
        src/tests.cpp
        src/memory_pool.c
        src/memory.c
        src/pointer_bit_hacks.c
)
target_link_libraries(
        tests
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)