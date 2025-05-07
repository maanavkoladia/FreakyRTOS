cmake_minimum_required(VERSION 3.28)

# Project name and version
project(HEAP_OFFCHIP_TEST C)

# Set the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Add executable target
add_executable(exe.elf
    main.c       # Your Unity test harness and test cases
    heap.c       # Your heap implementation
    unity.c      # Unity test framework
)

# Optionally add compile warnings
target_compile_options(exe.elf PRIVATE -Wall -Wextra)
