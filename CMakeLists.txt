cmake_minimum_required(VERSION 3.28)

project(talmon
    VERSION 0.1.0
    DESCRIPTION "Talos Linux monitor for Windows"
    LANGUAGES CXX)

if(NOT MSVC)
    message(FATAL_ERROR "Talmon targets Windows and must be built with MSVC.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Same CRT as the WinUI app (/MD, /MDd) and the x64-windows-static-md vcpkg triplet.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Stable output folders consumed by the WinUI project (src/app): out/<Config>/*.lib|*.dll
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")

option(TALMON_BUILD_TESTS "Build the GoogleTest suite" ON)

add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
add_compile_options(/utf-8 /permissive- /Zc:__cplusplus /Zc:preprocessor /EHsc /MP)

function(talmon_set_warnings target)
    target_compile_options(${target} PRIVATE /W4 /w14265 /w14062)
endfunction()

find_package(Threads REQUIRED)

add_subdirectory(src/domain)
add_subdirectory(src/application)
add_subdirectory(src/infrastructure)

if(TALMON_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
