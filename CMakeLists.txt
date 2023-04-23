cmake_minimum_required(VERSION 3.8)

option(BUILD_PACKER_TOOL "Build gpak packer tool" OFF)
option(BUILD_TESTS "Build gpak tests" OFF)

set(PAK_ARCHIVE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_subdirectory(lib)

if (BUILD_PACKER_TOOL)
	add_subdirectory(packer_tool)
	target_include_directories(packer_tool PRIVATE ${PAK_ARCHIVE_INCLUDE_DIR})
endif (BUILD_PACKER_TOOL)

if (BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
	target_include_directories(lib_test PRIVATE ${PAK_ARCHIVE_INCLUDE_DIR})
endif (BUILD_TESTS)