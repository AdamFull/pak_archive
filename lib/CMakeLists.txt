cmake_minimum_required(VERSION 3.8)
project(libgpak LANGUAGES C)

option(BUILD_SHARED "Build as a shared library" OFF)

set(EXTERNALS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../external/")
add_subdirectory(${EXTERNALS_PATH} external)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
set(SOURCES 
${SOURCES} 
"${EXTERNALS_PATH}lz4/lib/lz4file.c"
"${EXTERNALS_PATH}lz4/lib/lz4frame.c"
)

if(BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DGPAK_EXPORTS)
else()
    add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
endif()

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

target_include_directories(${PROJECT_NAME} PUBLIC 
"${CMAKE_CURRENT_SOURCE_DIR}"
"${EXTERNALS_PATH}lz4/lib"
"${EXTERNALS_PATH}zstd/lib"
"${EXTERNALS_PATH}libfmemopen"
)
target_link_libraries(${PROJECT_NAME} PRIVATE gpak_externals)