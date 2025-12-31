cmake_minimum_required(VERSION 3.20)
project(chip8recomp 
    VERSION 0.1.0 
    LANGUAGES CXX
    DESCRIPTION "CHIP-8 Static Recompiler"
)

# Source files
set(CHIP8RECOMP_SOURCES
    src/main.cpp
    src/decoder.cpp
    src/analyzer.cpp
    src/generator.cpp
    src/config.cpp
    src/rom.cpp
)

# Create the recompiler executable
add_executable(chip8recomp ${CHIP8RECOMP_SOURCES})

# Include directories
target_include_directories(chip8recomp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# C++20 features
target_compile_features(chip8recomp PRIVATE cxx_std_20)

# Compiler options
target_compile_options(chip8recomp PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Optional: Link toml11 for TOML parsing
# find_package(toml11 CONFIG)
# if(toml11_FOUND)
#     target_link_libraries(chip8recomp PRIVATE toml11::toml11)
#     target_compile_definitions(chip8recomp PRIVATE CHIP8_HAS_TOML)
# endif()

# Optional: Link fmt for formatting
# find_package(fmt CONFIG)
# if(fmt_FOUND)
#     target_link_libraries(chip8recomp PRIVATE fmt::fmt)
#     target_compile_definitions(chip8recomp PRIVATE CHIP8_HAS_FMT)
# endif()

# Installation
include(GNUInstallDirs)

install(TARGETS chip8recomp
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install include files (for library usage)
install(DIRECTORY include/recompiler
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
