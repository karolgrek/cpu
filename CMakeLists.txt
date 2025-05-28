cmake_minimum_required(VERSION 3.18)

project(hw03 C)

# Source files
set(SRC_CPU cpu.c)
set(SRC_MAIN main.c)
set(SRC_COMPILER compiler.c)

# Executables
add_executable(cpu ${SRC_CPU} ${SRC_MAIN})
# Executable for compiler
add_executable(compiler ${SRC_COMPILER})
target_compile_definitions(compiler PRIVATE _POSIX_C_SOURCE=200809L)


# Compiler flags
if (CMAKE_C_COMPILER_ID MATCHES Clang OR CMAKE_C_COMPILER_ID STREQUAL GNU)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -pedantic")
elseif (CMAKE_C_COMPILER_ID STREQUAL MSVC)
    target_compile_definitions(cpu PRIVATE _CRT_SECURE_NO_DEPRECATE)
    target_compile_definitions(compiler PRIVATE _CRT_SECURE_NO_DEPRECATE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /permissive- /W4 /EHsc")
endif()

# MinGW compatibility
if (MINGW)
    target_compile_definitions(cpu PRIVATE __USE_MINGW_ANSI_STDIO=1)
endif()
