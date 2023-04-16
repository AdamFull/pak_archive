cmake_minimum_required(VERSION 3.8)
project(pak_archive LANGUAGES C)

option(BUILD_SHARED "Build as a shared library" OFF)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

if(BUILD_SHARED)
    add_library(pak_archive SHARED ${SOURCES} ${HEADERS})
    target_compile_definitions(pak_archive PRIVATE -DPAK_ARCHIVE_EXPORTS)
else()
    add_library(pak_archive STATIC ${SOURCES} ${HEADERS})
endif()

target_compile_features(pak_archive PRIVATE c_std_11)
set_target_properties(pak_archive PROPERTIES
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

target_include_directories(pak_archive PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})