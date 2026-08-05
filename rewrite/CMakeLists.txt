cmake_minimum_required(VERSION 3.24)
project(ReaperCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ReaperCore SHARED
    src/dllmain.cpp
    src/core/application.cpp
    src/core/logger.cpp
    src/natives/native_registry.cpp
)

target_include_directories(ReaperCore PRIVATE src)
target_compile_definitions(ReaperCore PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

if(MSVC)
    target_compile_options(ReaperCore PRIVATE /W4 /permissive- /EHsc /Zc:preprocessor)
    set_property(TARGET ReaperCore PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set_target_properties(ReaperCore PROPERTIES
    OUTPUT_NAME "ReaperCore"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
