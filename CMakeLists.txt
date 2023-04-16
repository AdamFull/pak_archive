cmake_minimum_required(VERSION 3.8)

set(PAK_ARCHIVE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_subdirectory(lib)

add_subdirectory(test)
target_include_directories(lib_test PRIVATE ${PAK_ARCHIVE_INCLUDE_DIR})

add_subdirectory(packer_tool)
target_include_directories(packer_tool PRIVATE ${PAK_ARCHIVE_INCLUDE_DIR})