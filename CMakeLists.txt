#A bit arbitrary
cmake_minimum_required(VERSION 3.12) 
project(mz800emu)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING "Choose the type of build." FORCE)
    set_property(
        CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS
        "Release"
        "Debug"
        "MinSizeRel"
        "RelWithDebInfo")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/dependencies.cmake NO_POLICY_SCOPE) 

message("Building for : ${CMAKE_SYSTEM_NAME}")

add_library(mz800emu_options INTERFACE)
target_include_directories(mz800emu_options INTERFACE .)
target_link_libraries(mz800emu_options INTERFACE
    SDL2::SDL2
    PkgConfig::GLIB2
    $<$<C_COMPILER_ID:GNU>:m>
    $<$<BOOL:${ENABLE_UI}>:PkgConfig::GTK3>
    $<$<BOOL:${ENABLE_UI}>:PkgConfig::LIBSOUP>
    )

target_compile_definitions(mz800emu_options INTERFACE
    $<$<STREQUAL:Linux,${CMAKE_SYSTEM_NAME}>:LINUX>
    $<$<STREQUAL:Windows,${CMAKE_SYSTEM_NAME}>:WINDOWS>
    $<$<AND:$<STREQUAL:Windows,${CMAKE_SYSTEM_NAME}>,$<EQUAL:4,${CMAKE_SIZEOF_VOID_P}>>:WINDOWS_X86>
    $<$<AND:$<STREQUAL:Windows,${CMAKE_SYSTEM_NAME}>,$<EQUAL:8,${CMAKE_SIZEOF_VOID_P}>>:WINDOWS_X64>
    $<$<BOOL:${ENABLE_UI}>:ENABLE_UI>
    )

option(ENABLE_PEDANTIC "enable -pedantic" OFF)
option(ENABLE_WALL     "enable -Wall" OFF)
option(ENABLE_WERROR   "enable -Werror" OFF)

target_compile_options(mz800emu_options INTERFACE 
    $<$<BOOL:${ENABLE_PEDANTIC}>:-pedantic>
    $<$<BOOL:${ENABLE_WALL}>:-Wall>
    $<$<BOOL:${ENABLE_WERROR}>:-Werror>
    )
        

add_subdirectory(src)
