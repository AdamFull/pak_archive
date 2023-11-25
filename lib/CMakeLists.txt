cmake_minimum_required(VERSION 3.8)
project(libgpak LANGUAGES C)

option(BUILD_SHARED "Build as a shared library" OFF)

set(EXTERNALS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../external/")
add_subdirectory(${EXTERNALS_PATH} external)

file(GLOB_RECURSE GPAK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB_RECURSE GPAK_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

if(BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${GPAK_SOURCES} ${GPAK_HEADERS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DGPAK_EXPORTS -DGPAK_VERSION=1000)
else()
    add_library(${PROJECT_NAME} STATIC ${GPAK_SOURCES} ${GPAK_HEADERS})
endif()

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

target_include_directories(${PROJECT_NAME} PUBLIC 
"${CMAKE_CURRENT_SOURCE_DIR}"
"${EXTERNALS_PATH}libfmemopen"
)
target_link_libraries(${PROJECT_NAME} PRIVATE gpak_externals)