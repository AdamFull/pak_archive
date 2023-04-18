cmake_minimum_required(VERSION 3.8)
project(gpak_externals)

add_library(${PROJECT_NAME} gpakext.cpp)

if(WIN32)
add_subdirectory(libfmemopen)
target_link_libraries(${PROJECT_NAME} PUBLIC libfmemopen)
endif()

find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenSSL::Crypto)

find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ZLIB::ZLIB)

add_subdirectory("lz4/build/cmake/")
target_link_libraries(${PROJECT_NAME} PUBLIC  lz4_static)

add_subdirectory("zstd/build/cmake/")
target_link_libraries(${PROJECT_NAME} PUBLIC  libzstd_static)
