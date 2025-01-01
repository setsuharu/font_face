cmake_minimum_required(VERSION 3.22)
project(fontface)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


#file(GLOB SOURCE_FILES "src/*.cpp")

add_executable(${PROJECT_NAME} 
    src/bitmap/bitmap_string.cpp
    src/bitmap/bitmap.cpp
    src/font_face.cpp
    src/util.cpp
    src/main.cpp
)
target_include_directories(${PROJECT_NAME} PRIVATE "vendor/argparse/include")
target_include_directories(${PROJECT_NAME} PRIVATE "src/")