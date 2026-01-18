cmake_minimum_required(VERSION 3.10)

project(AntTSPSolver VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include_directories(include)

set(SOURCES 
    src/main.cpp
)

add_executable(solver ${SOURCES})

target_link_libraries(solver PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(solver PRIVATE /W4)
else()
    target_compile_options(solver PRIVATE -Wall -Wextra -Wpedantic)
endif()