cmake_minimum_required(VERSION 3.8)
project(lib_test LANGUAGES CXX)

find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(${PROJECT_NAME} PRIVATE libgpak GTest::GTest GTest::Main)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})