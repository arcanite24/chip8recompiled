cmake_minimum_required(VERSION 3.20)
project(chip8rt 
    VERSION 0.1.0 
    LANGUAGES C CXX
    DESCRIPTION "CHIP-8 Runtime Library"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui source files
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

# Source files
set(CHIP8RT_SOURCES
    src/context.c
    src/runtime.c
    src/instructions.c
    src/font.c
    src/platform_sdl.c
    src/settings.c
    src/menu.c
    src/imgui_overlay.cpp
)

# Create the runtime library
add_library(chip8rt STATIC ${CHIP8RT_SOURCES} ${IMGUI_SOURCES})

# Include directories
target_include_directories(chip8rt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

# Link SDL2
target_link_libraries(chip8rt
    PUBLIC
        SDL2::SDL2
)

# Platform-specific settings
if(APPLE)
    # macOS specific
    target_compile_definitions(chip8rt PRIVATE CHIP8_PLATFORM_MACOS)
elseif(UNIX)
    # Linux specific
    target_compile_definitions(chip8rt PRIVATE CHIP8_PLATFORM_LINUX)
elseif(WIN32)
    # Windows specific
    target_compile_definitions(chip8rt PRIVATE CHIP8_PLATFORM_WINDOWS)
endif()

# Compiler options
target_compile_options(chip8rt PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
    $<$<C_COMPILER_ID:MSVC>:/W4>
)

# Installation
include(GNUInstallDirs)

install(TARGETS chip8rt
    EXPORT chip8rt-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/chip8rt
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT chip8rt-targets
    FILE chip8rt-config.cmake
    NAMESPACE chip8rt::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/chip8rt
)
