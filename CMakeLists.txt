cmake_minimum_required(VERSION 3.20)
project(chip8-recompiled 
    VERSION 0.1.0 
    LANGUAGES C CXX
    DESCRIPTION "Static recompiler for CHIP-8 ROMs"
)

# C/C++ Standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(CHIP8_BUILD_RECOMPILER "Build the chip8recomp recompiler tool" ON)
option(CHIP8_BUILD_RUNTIME "Build the libchip8rt runtime library" ON)
option(CHIP8_BUILD_TESTS "Build the test suite" OFF)
option(CHIP8_BUILD_EXAMPLES "Build example recompiled ROMs" OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Find SDL2 (required for runtime)
if(CHIP8_BUILD_RUNTIME)
    find_package(SDL2 REQUIRED)
endif()

# Subdirectories
if(CHIP8_BUILD_RECOMPILER)
    add_subdirectory(recompiler)
endif()

if(CHIP8_BUILD_RUNTIME)
    add_subdirectory(runtime)
endif()

if(CHIP8_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(CHIP8_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install rules
include(GNUInstallDirs)

install(DIRECTORY docs/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)
