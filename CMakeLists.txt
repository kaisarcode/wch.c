cmake_minimum_required(VERSION 3.14)
project(wch C)

set(CMAKE_C_STANDARD 11)


option(WCH_NATIVE "Enable native hardware optimizations" OFF)

if(NOT MSVC)
    add_compile_options(-O3 -Wall -Wextra -Werror)
    if(WCH_NATIVE)
        add_compile_options(-march=native)
    endif()
    if(WIN32)
        add_definitions(-D_GNU_SOURCE -DWINVER=0x0600 -D_WIN32_WINNT=0x0600)
    else()
        add_definitions(-D_GNU_SOURCE)
    endif()
else()
    add_compile_options(/O2 /W3 /WX)
endif()

# Main executable
add_executable(wch
    src/wch.c
    src/libwch.c
)

target_include_directories(wch PRIVATE src)

if(UNIX AND NOT ANDROID)
    target_link_libraries(wch pthread m)
endif()

# Static library
add_library(wch_static STATIC src/libwch.c)

set_target_properties(wch_static PROPERTIES OUTPUT_NAME "wch")

target_include_directories(wch_static PRIVATE src)

# Shared library
add_library(wch_shared SHARED src/libwch.c)

set_target_properties(wch_shared PROPERTIES OUTPUT_NAME "wch")

target_include_directories(wch_shared PRIVATE src)

if(UNIX AND NOT ANDROID)
    target_link_libraries(wch_shared pthread m)
endif()
