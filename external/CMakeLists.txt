cmake_minimum_required(VERSION 3.8)
project(gpak_externals)

find_package(zstd CONFIG REQUIRED)

add_library(${PROJECT_NAME} STATIC gpakext.cpp)

if(WIN32)
add_subdirectory(libfmemopen)
target_link_libraries(${PROJECT_NAME} PUBLIC libfmemopen)
endif()

find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} 
	PUBLIC ZLIB::ZLIB
	PRIVATE $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
)
